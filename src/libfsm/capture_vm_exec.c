/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "capture_vm.h"
#include "capture_vm_program.h"
#include "capture_vm_log.h"

#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>

#include <fsm/capture.h>

/* for EXPENSIVE_CHECKS and TRACK_TIMES */
#include "internal.h"

#if EXPENSIVE_CHECKS
#include <adt/hash.h>
#endif

/* Special handling for a path node that has a long prefix of all 0
 * bits, as is common when the regex is unanchored at the start. */
#define USE_COLLAPSED_ZERO_PREFIX 1

/* Special out-of-range NONE values. */
#define NO_POS ((uint32_t)-1)
#define NO_ID  ((uint32_t)-1)
#define COLLAPSED_ZERO_PREFIX_ID ((uint32_t)-2)
#define NO_POS_SIZE_T ((size_t)-1)

/* Max number of bits each path link can store.
 * This value cannot be changed without reworking the data structures. */
#define PATH_LINK_BITS 32

/* This enables extra debugging/testing output */
#ifndef TESTING_OPTIONS
#define TESTING_OPTIONS 0
#endif

/* Write the solution to stdout (used for testing). */
#define LOG_SOLUTION_TO_STDOUT (0 || TESTING_OPTIONS)

/* Enable extra fields for debugging/performance tuning, most notably
 * a 'uniq_id' field that helps to see the various execution paths. */
#define CAPVM_STATS (0 || TESTING_OPTIONS)
#define CAPVM_PATH_STATS (0 && CAPVM_STATS)

/* This may no longer be necessary after further work on path handling. */
#define ALLOW_TABLE_RESIZING 1
#define ALLOW_PATH_TABLE_RESIZING (1 || ALLOW_TABLE_RESIZING)
#define ALLOW_THREAD_TABLE_RESIZING (0 || ALLOW_TABLE_RESIZING)

/* Set to non-zero to trap runaway path table growth */
#define PATH_TABLE_CEIL_LIMIT 0

/* Specialized logging that can be scraped to reconstruct non-interleaved
 * execution paths per thread. */
#define LOG_EXECUTION 0
#define LOG_EXECUTION_FILE stderr
#if LOG_EXECUTION

#if CAPVM_STATS == 0
#error CAPVM_STATS must be 1 for uniq_id
#endif

/* Various execution log messages, in an easily scraped format */
#define LOG_EXEC_OP(UNIQ_ID, INPUT_POS, OP_ID, OP_NAME)	\
	fprintf(LOG_EXECUTION_FILE,			\
	    "LOG_EXEC OP %u %u %u %s\n",		\
	    UNIQ_ID, INPUT_POS, OP_ID, OP_NAME)

#define LOG_EXEC_CHAR(UNIQ_ID, CHAR)		\
	fprintf(LOG_EXECUTION_FILE,		\
	    "LOG_EXEC CHAR %u %c 0x%02x\n", UNIQ_ID, isprint(CHAR) ? CHAR : '.', CHAR)

#define LOG_EXEC_HALT(UNIQ_ID)			\
	fprintf(LOG_EXECUTION_FILE,		\
	    "LOG_EXEC HALT %u\n", UNIQ_ID)

#define LOG_EXEC_PATH_FIND_SOLUTION(UNIQ_ID, BIT)		\
	fprintf(LOG_EXECUTION_FILE,				\
	    "LOG_EXEC PATH_FIND_SOLUTION %u %u\n", UNIQ_ID, BIT)

#define LOG_EXEC_PATH_SAVE_CAPTURES(UNIQ_ID, BIT)		\
	fprintf(LOG_EXECUTION_FILE,				\
	    "LOG_EXEC PATH_SAVE_CAPTURES %u %u\n", UNIQ_ID, BIT)

#define LOG_EXEC_SPLIT(PARENT_UNIQ_ID, CHILD_UNIQ_ID)		\
	fprintf(LOG_EXECUTION_FILE,				\
	    "LOG_EXEC SPLIT %u %u\n", PARENT_UNIQ_ID, CHILD_UNIQ_ID)
#else
#define LOG_EXEC_OP(UNIQ_ID, INPUT_POS, OP_ID, OP_NAME) /* no-op */
#define LOG_EXEC_CHAR(UNIQ_ID, CHAR)			/* no-op */
#define LOG_EXEC_HALT(UNIQ_ID)				/* no-op */
#define LOG_EXEC_PATH_FIND_SOLUTION(UNIQ_ID, BIT)	/* no-op */
#define LOG_EXEC_PATH_SAVE_CAPTURES(UNIQ_ID, BIT)	/* no-op */
#define LOG_EXEC_SPLIT(PARENT_UNIQ_ID, CHILD_UNIQ_ID)	/* no-op */
#endif

/* Bitset backed by an array of 32-bit words */
#define GET_BIT32(BITARRAY, BIT) (BITARRAY[BIT/32] & ((uint32_t)1 << (BIT & 31)))
#define SET_BIT32(BITARRAY, BIT) (BITARRAY[BIT/32] |= ((uint32_t)1 << (BIT & 31)))

static const char *
op_name[] = {
	[CAPVM_OP_CHAR] = "CHAR",
	[CAPVM_OP_CHARCLASS] = "CHARCLASS",
	[CAPVM_OP_MATCH] = "MATCH",
	[CAPVM_OP_JMP] = "JMP",
	[CAPVM_OP_JMP_ONCE] = "JMP_ONCE",
	[CAPVM_OP_SPLIT] = "SPLIT",
	[CAPVM_OP_SAVE] = "SAVE",
	[CAPVM_OP_ANCHOR] = "ANCHOR",
};

enum pair_id { PAIR_ID_CURRENT = 0, PAIR_ID_NEXT = 1 };

struct capvm {
	const struct capvm_program *p;
	const uint8_t *input;
	const uint32_t input_len;
	struct fsm_capture *capture_buf;
	const size_t capture_buf_length;
	size_t step_limit;

#if CAPVM_STATS
	uint32_t uniq_id_counter;
#endif

	/* Two stacks, used to track which execution instruction should
	 * be advanced next. The current stack is
	 * run_stacks[PAIR_ID_CURRENT], run_stacks[PAIR_ID_NEXT] is the
	 * stack for the next input position, and when the current stack
	 * is completed the next stack is copied over (and reversed).
	 * Same with run_stacks_h, the height for each stack, and the
	 * other fields with [2] below. */
	uint32_t *run_stacks[2];
	uint32_t run_stacks_h[2];

	/* Similarly, two columns of bits and two arrays of path_info
	 * node IDs and uniq_ids for the execution at a particular
	 * opcode. */
	uint32_t *evaluated[2];
	uint32_t *path_info_heads[2];
#if CAPVM_STATS
	uint32_t *uniq_ids[2];
#endif

	struct capvm_thread_stats {
		uint32_t live;
		uint32_t max_live;
	} threads;

	/* Pool of nodes for linked lists of path segments. */
	struct capvm_path_info_pool {
		uint32_t ceil;
		uint32_t live;
		uint32_t max_live;
		uint32_t freelist_head;
		struct capvm_path_info {
			union {
				struct capvm_path_freelist_link {
					uint16_t refcount; /* == 0 */
					uint32_t freelist;
				} freelist_node;
				struct capvm_path_info_link {
					uint16_t refcount; /* > 0, sticky at UINT16_MAX? */
					uint8_t used;      /* .bits used, <= PATH_LINK_BITS */
					uint32_t bits;
					uint32_t offset;
					/* Linked list to earlier path nodes, with common
					 * nodes shared until paths diverge.
					 *
					 * This can be either a valid path node ID, NO_ID
					 * for end of list, or COLLAPSED_ZERO_PREFIX_ID
					 * to indicate that the node is preceded by
					 * (offset) zero bits. */
					uint32_t backlink;
#if CAPVM_PATH_STATS
					uint32_t bits_added_per_input_character;
#endif
				} path;
			} u;
		} *pool;
	} paths;

	struct capvm_solution_info {
		uint32_t best_path_id;
#if CAPVM_STATS
		uint32_t best_path_uniq_id;
#endif
		uint32_t zeros_evaluated_up_to;
	} solution;

	struct {
		size_t steps;
#if CAPVM_STATS
		uint32_t matches;
		uint32_t path_prefixes_shared;
		uint32_t collapsed_zero_prefixes;
#endif
#if CAPVM_PATH_STATS
		uint32_t max_bits_added_per_input_character;
		uint32_t max_path_length_memory;
#endif
	} stats;

	enum fsm_capvm_program_exec_res res;
};

/* Type identifier macros */
#define IS_THREAD_FREELIST(T) (T->u.thread.path_info_head == NO_ID)
#define IS_PATH_FREELIST(P) (P->u.path.refcount == 0)
#define IS_PATH_NODE(P) (P->u.path.refcount > 0 && P->u.path.used <= PATH_LINK_BITS)

static void
release_path_info_link(struct capvm *vm, uint32_t *pi_id);

static void
dump_path_table(FILE *f, const struct capvm *vm);

static void
set_max_threads_live(struct capvm *vm, uint32_t new_max_live)
{
	vm->threads.max_live = new_max_live;
	if (LOG_CAPVM >= 6) {
		LOG(0, "==== new vm->threads.max_live: %u\n", vm->threads.max_live);
		dump_path_table(stderr, vm);
	}
}


/***********************
 * path_info functions *
 ***********************/

static void
set_max_paths_live(struct capvm *vm)
{
	vm->paths.max_live = vm->paths.live;
	if (LOG_CAPVM >= 6) {
		LOG(0, "==== new vm->paths.max_live: %u\n", vm->paths.max_live);
		dump_path_table(stderr, vm);
	}
}

static uint32_t
get_path_node_refcount(const struct capvm *vm, uint32_t p_id)
{
	assert(p_id < vm->paths.ceil);
	const struct capvm_path_info *pi = &vm->paths.pool[p_id];
	if (IS_PATH_FREELIST(pi)) {
		return pi->u.freelist_node.refcount;
	} else {
		assert(IS_PATH_NODE(pi));
		return pi->u.path.refcount;
	}
}

static void
inc_path_node_refcount(struct capvm *vm, uint32_t p_id)
{
	/* TODO: sticky refcount handling is not currently implemented */
	if (p_id == COLLAPSED_ZERO_PREFIX_ID) { return; }
	assert(p_id < vm->paths.ceil);
	struct capvm_path_info *pi = &vm->paths.pool[p_id];
	assert(IS_PATH_NODE(pi));
	LOG(5, "%s: p_id %u: refcnt %u -> %u\n",
	    __func__, p_id, pi->u.path.refcount, pi->u.path.refcount + 1);
	pi->u.path.refcount++;
}

static uint32_t
get_path_node_offset(const struct capvm *vm, uint32_t p_id)
{
	assert(p_id < vm->paths.ceil);
	const struct capvm_path_info *pi = &vm->paths.pool[p_id];
	assert(IS_PATH_NODE(pi));
	return pi->u.path.offset;
}

static uint32_t
get_path_node_backlink(const struct capvm *vm, uint32_t p_id)
{
	assert(p_id < vm->paths.ceil);
	const struct capvm_path_info *pi = &vm->paths.pool[p_id];
	if (IS_PATH_FREELIST(pi)) {
		return pi->u.freelist_node.freelist;
	} else {
		assert(IS_PATH_NODE(pi));
		return pi->u.path.backlink;
	}
}

static void
set_path_node_backlink(struct capvm *vm, uint32_t p_id, uint32_t backlink)
{
	assert(p_id < vm->paths.ceil);
	assert(backlink < vm->paths.ceil || (backlink == NO_ID || backlink == COLLAPSED_ZERO_PREFIX_ID));
	struct capvm_path_info *pi = &vm->paths.pool[p_id];
	assert(IS_PATH_NODE(pi));
	pi->u.path.backlink = backlink;
}

static void
dump_path_table(FILE *f, const struct capvm *vm)
{
	fprintf(f, "=== path table, %u/%u live\n",
	    vm->paths.live, vm->paths.ceil);
	for (uint32_t i = 0; i < vm->paths.ceil; i++) {
		struct capvm_path_info *pi = &vm->paths.pool[i];
		if (IS_PATH_FREELIST(pi)) {
			if (LOG_CAPVM >= 5) {
				fprintf(f, "paths[%u]: freelist -> %d\n",
				    i, (int)pi->u.freelist_node.freelist);
			}
		} else {
			assert(IS_PATH_NODE(pi));
			fprintf(f, "paths[%u]: refcount %u, used %u, bits 0x%08x, offset %u, backlink %d%s\n",
			    i, pi->u.path.refcount, pi->u.path.used, pi->u.path.bits,
			    pi->u.path.offset, (int)pi->u.path.backlink,
			    pi->u.path.backlink == COLLAPSED_ZERO_PREFIX_ID
			    ? " (collapsed zero prefix)"
			    : pi->u.path.backlink == NO_ID
			    ? " (none)"
			    : " (link)");
		}
	}
}

static void
check_path_table(const struct capvm *vm)
{
#if EXPENSIVE_CHECKS
	uint32_t *refcounts = calloc(vm->paths.ceil, sizeof(refcounts[0]));
	assert(refcounts);

	if (LOG_CAPVM >= 4) {
		dump_path_table(stderr, vm);
	}

	LOG(4, "%s: stack heights %u, %u\n", __func__,
	    vm->run_stacks_h[PAIR_ID_CURRENT], vm->run_stacks_h[PAIR_ID_NEXT]);

	for (uint32_t pair_id = 0; pair_id < 2; pair_id++) {
		for (uint32_t h = 0; h < vm->run_stacks_h[pair_id]; h++) {
			const uint32_t op_id = vm->run_stacks[pair_id][h];
			if (op_id == NO_ID) { continue; }
#if CAPVM_STATS
			const uint32_t uniq_id = vm->uniq_ids[pair_id][op_id];
#else
			const uint32_t uniq_id = 0;
#endif

			LOG(4, "%s: run_stacks[%u][%u/%u]: op_id %u (uniq_id %u) -> path_info_head %u\n",
			    __func__, pair_id, h, vm->run_stacks_h[pair_id], op_id,
			    uniq_id, vm->path_info_heads[pair_id][op_id]);
			if (op_id == NO_ID) { continue; }
			const uint32_t p_id = vm->path_info_heads[pair_id][op_id];
			if (p_id != NO_ID) {
				refcounts[p_id]++;
			}
		}
	}

	for (uint32_t p_id = 0; p_id < vm->paths.ceil; p_id++) {
		const struct capvm_path_info *pi = &vm->paths.pool[p_id];
		if (IS_PATH_FREELIST(pi)) {
			continue;
		}
		const uint32_t backlink = get_path_node_backlink(vm, p_id);
		if (backlink != NO_ID && backlink != COLLAPSED_ZERO_PREFIX_ID) {
			assert(backlink < vm->paths.ceil);
			refcounts[backlink]++;
		}
	}

	if (vm->solution.best_path_id != NO_ID) {
		assert(vm->solution.best_path_id < vm->paths.ceil);
		refcounts[vm->solution.best_path_id]++;
	}

	for (uint32_t p_id = 0; p_id < vm->paths.ceil; p_id++) {
		const struct capvm_path_info *pi = &vm->paths.pool[p_id];
		if (IS_PATH_FREELIST(pi)) { continue; }
		bool ok;
		const uint32_t refcount = get_path_node_refcount(vm, p_id);
		ok = refcounts[p_id] == refcount;

		if (!ok) {
			dump_path_table(stderr, vm);

			fprintf(stderr, "BAD REFCOUNT: pi[%u], expected %u, got %u\n",
			    p_id, refcounts[p_id], refcount);
			assert(ok);
		}
	}

	free(refcounts);
	LOG(6, "%s: passed\n", __func__);
#else
	(void)vm;
#endif
}

static bool
reserve_path_info_link(struct capvm *vm, uint32_t *pi_id)
{
	if (vm->paths.live == vm->paths.ceil) {
#if ALLOW_PATH_TABLE_RESIZING
		if (LOG_CAPVM >= 4) {
			fprintf(stderr, "\n");
			dump_path_table(stderr, vm);
			check_path_table(vm);
			fprintf(stderr, "\n");
		}

		const uint32_t nceil = 2*vm->paths.ceil;
		LOG(1, "%s: growing path table %u -> %u\n",
		    __func__, vm->paths.ceil, nceil);

		/* This can legitimitely be reached with very long inputs, but
		 * if PATH_TABLE_CEIL_LIMIT is non-zero and this is hit then
		 * it's most likely a sign of an infinite loop. */
		if (PATH_TABLE_CEIL_LIMIT != 0 && nceil > PATH_TABLE_CEIL_LIMIT) {
			assert(!"reached PATH_TABLE_CEIL_LIMIT");
		}

		assert(nceil > vm->paths.ceil);
		struct capvm_path_info *npool = realloc(vm->paths.pool,
		    nceil * sizeof(npool[0]));
		if (npool == NULL) {
			return false;
		}

		for (size_t i = vm->paths.ceil; i < nceil; i++) {
			npool[i].u.freelist_node.refcount = 0;
			npool[i].u.freelist_node.freelist = i + 1;
		}
		npool[nceil - 1].u.freelist_node.refcount = 0;
		npool[nceil - 1].u.freelist_node.freelist = NO_POS;
		vm->paths.freelist_head = vm->paths.ceil;
		vm->paths.ceil = nceil;
		vm->paths.pool = npool;
#else
		assert(!"shouldn't need to grow path pool");
#endif
	}

	assert(vm->paths.live < vm->paths.ceil);
	assert(vm->paths.freelist_head != NO_POS);

	*pi_id = vm->paths.freelist_head;
	LOG(3, "%s: returning %u\n", __func__, *pi_id);
	return true;
}

/* Release a reference to a path_info_link. Consume the argument.
 * If the reference count reaches 0, repool the node and release
 * its backlink. */
static void
release_path_info_link(struct capvm *vm, uint32_t *pi_id)
{
#define LOG_RELEASE_PI 0
	size_t count = 0;
	assert(pi_id != NULL);
	uint32_t cur_id = *pi_id;
	LOG(4 - LOG_RELEASE_PI, "%s: pi_id %u\n", __func__, cur_id);
	*pi_id = NO_ID;

	while (cur_id != NO_ID) {
		struct capvm_path_info *pi = &vm->paths.pool[cur_id];
		uint32_t refcount = get_path_node_refcount(vm, cur_id);
		LOG(4 - LOG_RELEASE_PI, "-- checking path_info[%u]: refcount %u\n",
		    cur_id, refcount);
		assert(refcount > 0);
		LOG(4 - LOG_RELEASE_PI, "release: pi[%u] refcount %u -> %u\n",
		    cur_id, refcount, refcount - 1);

		const uint32_t backlink = get_path_node_backlink(vm, cur_id);
		assert(IS_PATH_NODE(pi));
		pi->u.path.refcount--;
		refcount = pi->u.path.refcount;

		if (refcount > 0) {
			break;
		}

		count++;
		LOG(3 - LOG_RELEASE_PI, "-- repooling path_info %u, now %u live\n",
		    cur_id, vm->paths.live - 1);
		LOG(3 - LOG_RELEASE_PI, "-- backlink: %d\n", backlink);

		pi->u.freelist_node.freelist = vm->paths.freelist_head;
		vm->paths.freelist_head = cur_id;
		assert(vm->paths.live > 0);
		vm->paths.live--;

		cur_id = backlink;
		if (cur_id == COLLAPSED_ZERO_PREFIX_ID) {
			break;
		}
	}
}

static void
print_path(FILE *f, const struct capvm *vm, uint32_t p_id)
{
	if (p_id == NO_ID) {
		fprintf(f, "/0");
		return;
	}

	/* reverse links to the root node */
	uint32_t zero_prefix = 0;
	uint32_t next = NO_ID;
	uint32_t first = NO_ID;
	uint32_t prev;

	while (p_id != NO_ID) {
		assert(p_id < vm->paths.ceil);
		struct capvm_path_info *pi = &vm->paths.pool[p_id];
		assert(!IS_PATH_FREELIST(pi));

		uint32_t bl;
		assert(IS_PATH_NODE(pi));
		bl = pi->u.path.backlink;
		pi->u.path.backlink = next;

		if (bl == NO_ID) {
			prev = bl;
			first = p_id;
			break;
		} else if (bl == COLLAPSED_ZERO_PREFIX_ID) {
			prev = bl;
			first = p_id;
			zero_prefix = pi->u.path.offset;
			break;
		}

		next = p_id;
		p_id = bl;
	}

	if (zero_prefix > 0) {
		fprintf(f, "0/%u", zero_prefix);
	}

	/* iterate forward, printing and restoring link order */
	p_id = first;
	while (p_id != NO_ID) {
		assert(p_id < vm->paths.ceil);
		struct capvm_path_info *pi = &vm->paths.pool[p_id];
		assert(!IS_PATH_FREELIST(pi));

		uint32_t fl;	/* now a forward link */
		assert(IS_PATH_NODE(pi));
		fl = pi->u.path.backlink;
		pi->u.path.backlink = prev;
		prev = p_id;
		fprintf(f, "%s%08x/%u", prev == NO_ID ? "" : " ",
		    pi->u.path.bits, pi->u.path.used);

		p_id = fl;
	}
}

#if EXPENSIVE_CHECKS
SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
#endif
static int
cmp_paths(struct capvm *vm, uint32_t p_a, uint32_t p_b)
{
#if EXPENSIVE_CHECKS
	/* When EXPENSIVE_CHECKS is set, walk the chains
	 * before and after and compare incremental hashing of node IDs,
	 * to ensure the chains are restored properly. */
	uint64_t hash_a_before = 0;
	uint64_t hash_b_before = 0;
#endif

#define LOG_CMP_PATHS 0
	LOG(3 - LOG_CMP_PATHS, "%s: p_a %d, p_b %d\n", __func__, p_a, p_b);

	if (p_a == NO_ID) {
		return p_b == NO_ID ? 0 : -1;
	} else if (p_b == NO_ID) {
		return 1;
	}

	assert(p_a != p_b);

	if (LOG_CAPVM >= 5) {
		fprintf(stderr, "A: ");
		print_path(stderr, vm, p_a);
		fprintf(stderr, "\n");

		fprintf(stderr, "B: ");
		print_path(stderr, vm, p_b);
		fprintf(stderr, "\n");
	}

	/* walk both paths backward until they reach a beginning
	 * or the common prefix node, reversing links along the
	 * way, then compare forward and restore link order. */
	uint32_t link_a = p_a;
	uint32_t link_b = p_b;

	uint32_t fwd_a = NO_ID;
	uint32_t fwd_b = NO_ID;

	/* Walk both paths backward, individually until reaching a
	 * common offset, then back until reaching a common prefix
	 * (including the start). While iterating backward, replace
	 * the .backlink field with a forward link, which will be
	 * reverted when iterating forward and comparing from the
	 * common prefix. */
	bool common_prefix_found = false;
	uint32_t first_a = NO_ID;
	uint32_t first_b = NO_ID;
	uint32_t common_prefix_link; /* can be NO_ID */

#if EXPENSIVE_CHECKS
	uint32_t hash_step = 0;	/* added so ordering matters */
	while (link_a != NO_ID) {
		assert(link_a < vm->paths.ceil);
		const uint32_t prev = get_path_node_backlink(vm, link_a);
		hash_a_before += hash_id(link_a + hash_step);
		link_a = prev;
		hash_step++;
	}
	hash_step = 0;
	while (link_b != NO_ID) {
		assert(link_b < vm->paths.ceil);
		const uint32_t prev = get_path_node_backlink(vm, link_b);
		hash_b_before += hash_id(link_b + hash_step);
		link_b = prev;
		hash_step++;
	}

	link_a = p_a;
	link_b = p_b;
#endif

	while (!common_prefix_found) {
		assert(link_a != NO_ID);
		assert(link_b != NO_ID);
		assert(link_a < vm->paths.ceil);
		assert(link_b < vm->paths.ceil);

		const uint32_t prev_a = get_path_node_backlink(vm, link_a);
		const uint32_t prev_b = get_path_node_backlink(vm, link_b);
		const uint32_t offset_a = get_path_node_offset(vm, link_a);
		const uint32_t offset_b = get_path_node_offset(vm, link_b);
		const uint32_t backlink_a = get_path_node_backlink(vm, link_a);
		const uint32_t backlink_b = get_path_node_backlink(vm, link_b);

		LOG(3 - LOG_CMP_PATHS,
		    "%s: backward loop: link_a %d (offset %u, prev %d), link_b %d (offset %u, prev %d)\n",
		    __func__, link_a, offset_a, prev_a, link_b, offset_b, prev_b);

		assert((offset_a & 31) == 0); /* multiple of 32 */
		assert((offset_b & 31) == 0); /* multiple of 32 */
		if (offset_a > offset_b) {
			LOG(3 - LOG_CMP_PATHS, "%s: backward loop: a longer than b\n", __func__);
			set_path_node_backlink(vm, link_a, fwd_a);
			fwd_a = link_a;
			link_a = prev_a;
		} else if (offset_b > offset_a) {
			LOG(3 - LOG_CMP_PATHS, "%s: backward loop: b longer than a\n", __func__);
			set_path_node_backlink(vm, link_b, fwd_b);
			fwd_b = link_b;
			link_b = prev_b;
		} else {
			assert(offset_b == offset_a);
			LOG(3 - LOG_CMP_PATHS, "%s: backward loop: comparing backlinks: a: %d, b: %d\n",
			    __func__, backlink_a, backlink_b);
			assert(fwd_a != link_a);
			set_path_node_backlink(vm, link_a, fwd_a);
			assert(fwd_b != link_b);
			set_path_node_backlink(vm, link_b, fwd_b);

			if (prev_a == prev_b) {
				/* if == NO_ID, empty prefix */
				common_prefix_found = true;
				common_prefix_link = prev_a;
				first_a = link_a;
				first_b = link_b;

				LOG(3 - LOG_CMP_PATHS, "%s: backward loop: common_prefix_found: %d\n",
				    __func__, common_prefix_link);
			} else {
				fwd_a = link_a;
				fwd_b = link_b;

				link_a = prev_a;
				link_b = prev_b;
			}
		}
	}

	assert(first_a != NO_ID);
	assert(first_b != NO_ID);
	link_a = first_a;
	link_b = first_b;

	bool cmp_done = false;
	int res;
	bool done_restoring_link_order = false;
	uint32_t prev_a = common_prefix_link;
	uint32_t prev_b = common_prefix_link;
	while (!done_restoring_link_order) {
		LOG(3 - LOG_CMP_PATHS,
		    "%s: fwd loop, link_a %d, link_b %d, cmp_done %d\n",
		    __func__, link_a, link_b, cmp_done);
		if (!cmp_done) {
			if (link_a == NO_ID) { /* b is longer */
				cmp_done = true;
				if (link_b == NO_ID) {
					res = 0;
					LOG(3 - LOG_CMP_PATHS,
					    "%s: fwd loop, equal length, res %d\n", __func__, res);
				} else {
					res = -1;
					LOG(3 - LOG_CMP_PATHS,
					    "%s: fwd loop, b is longer, res %d\n", __func__, res);
				}
			} else if (link_b == NO_ID) { /* a is longer */
				cmp_done = true;
				res = 1;
				LOG(3 - LOG_CMP_PATHS,
				    "%s: fwd loop, a is longer, res %d\n", __func__, res);
			} else {
				assert(link_a < vm->paths.ceil);
				assert(link_b < vm->paths.ceil);
				struct capvm_path_info *pi_a = &vm->paths.pool[link_a];
				struct capvm_path_info *pi_b = &vm->paths.pool[link_b];

				const uint32_t offset_a = get_path_node_offset(vm, link_a);
				const uint32_t offset_b = get_path_node_offset(vm, link_b);

				assert(offset_a == offset_b);

				if (pi_a->u.path.bits != pi_b->u.path.bits) {
					res = pi_a->u.path.bits < pi_b->u.path.bits ? -1 : 1;
					cmp_done = true;
					LOG(3 - LOG_CMP_PATHS,
					    "%s: fwd loop, different bits (0x%08x, 0x%08x) => res %d\n",
					    __func__, pi_a->u.path.bits, pi_b->u.path.bits, res);
				}
			}
		}

		/* Check if both have reached the original head node. */
		if (link_a == NO_ID && link_b == NO_ID) {
			done_restoring_link_order = true;
			LOG(3 - LOG_CMP_PATHS,
			    "%s: fwd loop: reached end of both paths, prev_a %d (p_a %d), prev_b %d (p_b %d)\n",
			    __func__, prev_a, p_a, prev_b, p_b);
			assert(prev_a == p_a);
			assert(prev_b == p_b);
		}

		/* Whether or not comparison has finished, iterate forward,
		 * restoring forward links. */
		if (link_a != NO_ID) {
			assert(link_a < vm->paths.ceil);
			const uint32_t fwd_a = get_path_node_backlink(vm, link_a);
			LOG(3 - LOG_CMP_PATHS, "%s: fwd loop: link_a %d, fwd_a %d\n",
			    __func__, link_a, fwd_a);
			assert(fwd_a != link_a);

			LOG(3 - LOG_CMP_PATHS,
			    "%s: fwd loop, restoring a's backlink: pi[%u].backlink <- %d\n",
			    __func__, link_a, prev_a);
			set_path_node_backlink(vm, link_a, prev_a);
			prev_a = link_a;
			link_a = fwd_a;
		}

		if (link_b != NO_ID) {
			assert(link_b < vm->paths.ceil);
			const uint32_t fwd_b = get_path_node_backlink(vm, link_b);
			LOG(3 - LOG_CMP_PATHS, "%s: fwd loop: link_b %d, fwd_b %d\n",
			    __func__, link_b, fwd_b);
			assert(fwd_b != link_b);

			LOG(3 - LOG_CMP_PATHS,
			    "%s: fwd loop, restoring b's backlink: pi[%u].backlink <- %d\n",
			    __func__, link_b, prev_b);
			set_path_node_backlink(vm, link_b, prev_b);
			prev_b = link_b;
			link_b = fwd_b;
		}
	}

	LOG(3 - LOG_CMP_PATHS, "%s: res %d\n", __func__, res);

#if EXPENSIVE_CHECKS
	uint64_t hash_a_after = 0;
	uint64_t hash_b_after = 0;
	hash_step = 0;
	link_a = p_a;
	while (link_a != NO_ID) {
		assert(link_a < vm->paths.ceil);
		const uint32_t prev = get_path_node_backlink(vm, link_a);
		hash_a_after += hash_id(link_a + hash_step);
		link_a = prev;
		hash_step++;
	}
	link_b = p_b;
	hash_step = 0;
	while (link_b != NO_ID) {
		assert(link_b < vm->paths.ceil);
		const uint32_t prev = get_path_node_backlink(vm, link_b);
		hash_b_after += hash_id(link_b + hash_step);
		link_b = prev;
		hash_step++;
	}

	assert(hash_a_after == hash_a_before);
	assert(hash_b_after == hash_b_before);
#endif

	return res;
#undef LOG_CMP_PATHS
}

static void
handle_possible_matching_path(struct capvm *vm, uint32_t path_info_head, uint32_t uniq_id);

static bool
copy_path_info(struct capvm *vm, uint32_t path_info_head,
	uint32_t *new_path_info_head);

static bool
extend_path_info(struct capvm *vm, uint32_t path_info_head, bool greedy, uint32_t uniq_id,
	uint32_t *new_path_info_head);

/* Push the next execution step onto the stack, if it hasn't already
 * been run by a greedier path. Calling this hands off ownership of the
 * path_info_head, so it is released if execution will not be resumed
 * later. */
static void
schedule_possible_next_step(struct capvm *vm, enum pair_id pair_id,
    uint32_t input_pos, uint32_t op_id,
    uint32_t path_info_head, uint32_t uniq_id)
{
	assert(path_info_head != NO_ID);
	uint32_t *stack = vm->run_stacks[pair_id];
	uint32_t *stack_h = &vm->run_stacks_h[pair_id];
	assert((*stack_h) < vm->p->used);

	/* If that instruction has already been evaluated, skip the
	 * redundant execution by a less greedy path. */
	const uint32_t *evaluated = vm->evaluated[pair_id];
	const bool already_evaluated = GET_BIT32(evaluated, op_id) != 0;
	LOG(3, "%s: pair_id %u, input_pos %u, op_id %u, path_info_head %u, uniq_id %u, already_evaluated %d, stack_h %u\n",
	    __func__, pair_id, input_pos, op_id, path_info_head, uniq_id, already_evaluated, *stack_h);

	if (already_evaluated) {
		LOG_EXEC_HALT(uniq_id);
		release_path_info_link(vm, &path_info_head);
		assert(vm->threads.live > 0);
		vm->threads.live--;
	} else {

		/* If the work being scheduled by the current greediest
		 * thread pre-empts work scheduled by a less greedy
		 * thread, release that thread's path link and clear its
		 * op ID on the run stack.
		 *
		 * TODO: Ideally, avoid the linear scan here, but the
		 * total stack height is bounded by the generated program size
		 * and should be fairly small in practice. Wait to change this
		 * untill there are benchmarks in place showing it's necessary.
		 *
		 * An extra two bits per opcode (one for each stack) could
		 * be used to track whether the stack already contains
		 * op_id, so the linear scan could be avoided except when
		 * actually necessary. */
		uint32_t cur_pih = vm->path_info_heads[pair_id][op_id];
		if (cur_pih != NO_ID) {
			release_path_info_link(vm, &cur_pih);
			vm->path_info_heads[pair_id][op_id] = NO_ID;
			const size_t h = *stack_h;
			for (size_t i = 0; i < h; i++) {
				if (stack[i] == op_id) {
					stack[i] = NO_ID;
					vm->threads.live--;
				}
			}
		}
		stack[(*stack_h)++] = op_id;
		vm->path_info_heads[pair_id][op_id] = path_info_head;
#if CAPVM_STATS
		vm->uniq_ids[pair_id][op_id] = uniq_id;
#endif

		if (*stack_h > vm->threads.max_live) {
			vm->threads.max_live = *stack_h;
			if (LOG_CAPVM >= 6) {
				LOG(0, "==== new vm->threads.max_live: %u\n", vm->threads.max_live);
				dump_path_table(stderr, vm);
			}
		}
	}
}

static void
eval_vm_advance_greediest(struct capvm *vm, uint32_t input_pos,
    uint32_t path_info_head, uint32_t uniq_id, uint32_t op_id)
{
	LOG(5, "%s: input_pos %u, input_len %u, op_id %u, threads_live %u\n",
	    __func__, input_pos, vm->input_len, op_id, vm->threads.live);

	if (vm->stats.steps == vm->step_limit) {
		LOG(1, "%s: halting, steps == step_limit %zu\n",
		    __func__, vm->step_limit);
		return;
	}

	assert(op_id < vm->p->used);

	const struct capvm_opcode *op = &vm->p->ops[op_id];

	LOG(2, "%s: op_id[%u]: input_pos %u, path_info_head %u, uniq_id %u, op %s\n",
	    __func__, op_id, input_pos, path_info_head, uniq_id, op_name[op->t]);
	LOG_EXEC_OP(uniq_id, input_pos, op_id, op_name[op->t]);

	vm->stats.steps++;
	if (vm->stats.steps == vm->step_limit) {
		/* TODO: Set some sort of STEP_LIMIT_REACHED error. */
		return;
	}

	switch (op->t) {
	case CAPVM_OP_CHAR:
		if (input_pos == vm->input_len) {
			goto halt_thread; /* past end of input */
		}

		LOG(3, "OP_CHAR: input_pos %u, exp char '%c', got '%c'\n",
		    input_pos, op->u.chr, vm->input[input_pos]);

		if (vm->input[input_pos] != op->u.chr) {
			goto halt_thread; /* character mismatch */
		}
		LOG_EXEC_CHAR(uniq_id, vm->input[input_pos]);

		schedule_possible_next_step(vm, PAIR_ID_NEXT, input_pos + 1, op_id + 1,
		    path_info_head, uniq_id);
		break;

	case CAPVM_OP_CHARCLASS:
	{
		if (input_pos == vm->input_len) {
			goto halt_thread; /* past end of input */
		}

		const uint8_t c = vm->input[input_pos];
		const uint32_t cc_id = op->u.charclass_id;
		assert(cc_id < vm->p->char_classes.count);
		const struct capvm_char_class *class = &vm->p->char_classes.sets[cc_id];

		if (!(class->octets[c/64] & ((uint64_t)1 << (c&63)))) {
			goto halt_thread; /* character not in class */
		}
		LOG_EXEC_CHAR(uniq_id, vm->input[input_pos]);

		schedule_possible_next_step(vm, PAIR_ID_NEXT, input_pos + 1, op_id + 1,
		    path_info_head, uniq_id);
		break;
	}

	case CAPVM_OP_MATCH:
		if (input_pos == vm->input_len) {
			handle_possible_matching_path(vm, path_info_head, uniq_id);
		} else if (vm->input_len > 0 && input_pos == vm->input_len - 1
		    && vm->input[input_pos] == '\n') {
			LOG(3, "OP_MATCH: special case for trailing newline\n");
			handle_possible_matching_path(vm, path_info_head, uniq_id);
		}
		goto halt_thread;

	case CAPVM_OP_JMP:
		schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op->u.jmp,
		    path_info_head, uniq_id);
		break;

	case CAPVM_OP_JMP_ONCE:
	{
		/* If the destination for this jump has already been visited
		 * without advancing input, then skip the jump. This is necessary
		 * for edge cases like the first branch in `^(^|.$)*`, which would
		 * otherwise have a backward jump to before the first case, due to
		 * the repetition, and would effectively be treated as an infinite
		 * loop and ignored, leading to incorrect match bounds for "x".
		 *
		 * Replaying the capture path does not track what has been evaluated,
		 * so this needs to record the branch in the path. This will make
		 * repetition more expensive in some cases, but compilation could
		 * emit a JMP when it's safe to do so. */
		const bool greedy = GET_BIT32(vm->evaluated[PAIR_ID_CURRENT], op->u.jmp_once);
		if (greedy) {
			/* non-greedy branch -- fall through */
			uint32_t new_path_info_head = NO_ID;
			if (!extend_path_info(vm, path_info_head, 0, uniq_id, &new_path_info_head)) {
				release_path_info_link(vm, &path_info_head);
				goto alloc_error;
			}
			schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op_id + 1,
			    new_path_info_head, uniq_id);
		} else {
			/* greedy branch -- loop back and potentially match more */
			uint32_t new_path_info_head = NO_ID;
			if (!extend_path_info(vm, path_info_head, 1, uniq_id, &new_path_info_head)) {
				release_path_info_link(vm, &path_info_head);
				goto alloc_error;
			}
			schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op->u.jmp_once,
			    new_path_info_head, uniq_id);
		}
		break;
	}

	case CAPVM_OP_SPLIT:
	{
		const uint32_t dst_cont = op->u.split.cont;
		const uint32_t dst_new = op->u.split.new;

		/* destinations must be in range and not self-referential */
		assert(dst_cont < vm->p->used);
		assert(dst_new < vm->p->used);
		assert(dst_cont != op_id);
		assert(dst_new != op_id);

		uint32_t new_path_info_head;
		if (!copy_path_info(vm, path_info_head, &new_path_info_head)) {
			goto alloc_error;
		}

		/* cont is the greedy branch */
		if (!extend_path_info(vm, path_info_head, 1, uniq_id, &path_info_head)) {
			release_path_info_link(vm, &path_info_head);
			goto alloc_error;
		}

		/* new is the non-greedy branch */
		if (!extend_path_info(vm, new_path_info_head, 0, uniq_id, &new_path_info_head)) {
			release_path_info_link(vm, &path_info_head);
			goto alloc_error;
		}

#if CAPVM_STATS
		const uint32_t new_uniq_id = ++vm->uniq_id_counter;
#else
		const uint32_t new_uniq_id = 0;
#endif

		vm->threads.live++;
		if (vm->threads.live > vm->threads.max_live) {
			set_max_threads_live(vm, vm->threads.live);
		}

		/* Push the split.new destination, and then the
		 * split.cont destination on top of it, so that the
		 * greedier .cont branch will be fully evaluated
		 * first. */
		schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, dst_new,
		    new_path_info_head, new_uniq_id);
		schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, dst_cont,
		    path_info_head, uniq_id);
		LOG_EXEC_SPLIT(uniq_id, new_uniq_id);

	break;
	}

	case CAPVM_OP_SAVE:
		/* no-op, during this stage */
		schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op_id + 1,
		    path_info_head, uniq_id);
		break;

	case CAPVM_OP_ANCHOR:
		if (op->u.anchor == CAPVM_ANCHOR_START) {
			LOG(3, "%s: ^ anchor\n", __func__);
			/* ignore a trailing newline, because PCRE does,
			 * even after a $ anchor. */
			if (input_pos == 0
			    && vm->input_len == 1
			    && vm->input[0] == '\n') {
				/* allowed */
				LOG(3, "%s: special case: ^ ignoring trailing newline\n", __func__);
				schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op_id + 1,
				    path_info_head, uniq_id);
				break;
			} else if (input_pos == 1
			    && vm->input_len == 1
			    && vm->input[0] == '\n') {
				/* allowed */
			} else if (input_pos != 0) { goto halt_thread; }
		} else {
			assert(op->u.anchor == CAPVM_ANCHOR_END);
			LOG(3, "%s: $ anchor: input_len %u, input_pos %u\n",
			    __func__, vm->input_len, input_pos);

			/* ignore a trailing newline, because PCRE does */
			if (vm->input_len > 0 && input_pos == vm->input_len - 1) {
				if (vm->input[input_pos] != '\n') {
					goto halt_thread;
				}
				LOG(3, "%s: special case: $ allowing trailing newline\n", __func__);
				schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op_id + 1,
				    path_info_head, uniq_id);
				break;
			} else if (input_pos != vm->input_len) {
				goto halt_thread;
			}
		}

		schedule_possible_next_step(vm, PAIR_ID_CURRENT, input_pos, op_id + 1,
		    path_info_head, uniq_id);
		break;

	default:
		assert(!"unreachable");
		return;
	}

	if (EXPENSIVE_CHECKS) { /* postcondition */
		check_path_table(vm);
	}

	/* FIXME: Check the cleanup logic here. */
	return;

halt_thread:
	/* do not push further execution on the run stack */
	LOG_EXEC_HALT(uniq_id);

	release_path_info_link(vm, &path_info_head);
	assert(vm->threads.live > 0);
	vm->threads.live--;
	return;

alloc_error:
	release_path_info_link(vm, &path_info_head);
	vm->res = FSM_CAPVM_PROGRAM_EXEC_ERROR_ALLOC;
}

static void
handle_possible_matching_path(struct capvm *vm, uint32_t pi_id, uint32_t uniq_id)
{
	LOG(3, "\n%s: HIT, pi_id %u, uniq_id %u\n", __func__, pi_id, uniq_id);

	if (LOG_CAPVM >= 3) {
		LOG(3, "--- current_live: %u, max_live: %u\n",
		    vm->threads.live, vm->threads.max_live);
		dump_path_table(stderr, vm);
		LOG(3, "=====\n");
	}

#if CAPVM_STATS
	vm->stats.matches++;
#endif

	assert(pi_id < vm->paths.ceil);

	if (vm->solution.best_path_id == NO_ID) {
		struct capvm_path_info *pi = &vm->paths.pool[pi_id];
		assert(!IS_PATH_FREELIST(pi));
		if (LOG_CAPVM >= 5) {
			const uint32_t refcount = get_path_node_refcount(vm, pi_id);
			LOG(5, "MATCH: pi_id %u refcount %u -> %u\n",
			    pi_id, refcount, refcount + 1);
		}
		inc_path_node_refcount(vm, pi_id);
		vm->solution.best_path_id = pi_id;
#if CAPVM_STATS
		vm->solution.best_path_uniq_id = uniq_id;
#endif
		LOG(3, "MATCH: saved current best solution path (pi_id %u)\n", pi_id);
	} else {
		/* Compare path info and only keep the path associated
		 * with the greediest match so far. */
		const int res = cmp_paths(vm, pi_id, vm->solution.best_path_id);
		if (res > 0) {
			/* replace current best solution */
			struct capvm_path_info *pi = &vm->paths.pool[pi_id];
			assert(!IS_PATH_FREELIST(pi));
			if (LOG_CAPVM >= 5) {
				const uint32_t refcount = get_path_node_refcount(vm, pi_id);
				LOG(5, "MATCH: pi_id %u refcount %u -> %u\n",
				    pi_id, refcount, refcount + 1);
			}
			inc_path_node_refcount(vm, pi_id);

			LOG(3, "MATCH: replacing current best solution path %u with %u\n",
			    vm->solution.best_path_id, pi_id);

			release_path_info_link(vm, &vm->solution.best_path_id);
			vm->solution.best_path_id = pi_id;
#if CAPVM_STATS
			vm->solution.best_path_uniq_id = uniq_id;
#endif
		} else {
			/* keep the current best solution */
			LOG(3, "MATCH: ignoring new solution path %u, keeping %u\n",
			    pi_id, vm->solution.best_path_id);
		}
	}
}

static bool
eval_vm(struct capvm *vm)
{
	uint32_t i_i;

	/* init the path_info_heads tables to NO_ID, except for cell 0
	 * in next, which contains the starting point. */
	for (size_t op_i = 0; op_i < vm->p->used; op_i++) {
		vm->path_info_heads[PAIR_ID_CURRENT][op_i] = NO_ID;
#if CAPVM_STATS
		vm->uniq_ids[PAIR_ID_CURRENT][op_i] = NO_ID;
#endif
	}
	for (size_t op_i = 1; op_i < vm->p->used; op_i++) {
		vm->path_info_heads[PAIR_ID_NEXT][op_i] = NO_ID;
#if CAPVM_STATS
		vm->uniq_ids[PAIR_ID_NEXT][op_i] = NO_ID;
#endif
	}

	for (i_i = 0; i_i <= vm->input_len; i_i++) {
		if (vm->threads.live == 0
		    || vm->stats.steps == vm->step_limit) {
			LOG(3, "%s: breaking, live %u, steps %zu/%zd\n",
			    __func__, vm->threads.live, vm->stats.steps, vm->step_limit);
			break;
		}
		LOG(3, "\n###### i_i %u\n", i_i);

		LOG(4, "-- clearing evaluated\n");
		const size_t evaluated_bit_words = vm->p->used/32 + 1;
		for (size_t i = 0; i < evaluated_bit_words; i++) {
			vm->evaluated[PAIR_ID_CURRENT][i] = 0;
			vm->evaluated[PAIR_ID_NEXT][i] = 0;
		}

		uint32_t *stack_h = &vm->run_stacks_h[PAIR_ID_CURRENT];
		uint32_t *run_stack = vm->run_stacks[PAIR_ID_CURRENT];

		/* Copy everything from the next run stack to the
		 * current. Copy in reverse, so items that were pushed
		 * earlier by greedier paths end up on the top of the
		 * stack and evalated first, preserving greedy
		 * ordering. */
		{
			const uint32_t next_stack_h = vm->run_stacks_h[PAIR_ID_NEXT];
			const uint32_t *next_stack = vm->run_stacks[PAIR_ID_NEXT];
			uint32_t *next_path_info_heads = vm->path_info_heads[PAIR_ID_NEXT];
			uint32_t *cur_path_info_heads = vm->path_info_heads[PAIR_ID_CURRENT];

			uint32_t discarded = 0;
			for (size_t i = 0; i < next_stack_h; i++) {
				const uint32_t op_id = next_stack[i];
				if (op_id == NO_ID) {
					assert(!"unreachable");
					discarded++;
					continue;
				}

				cur_path_info_heads[op_id] = next_path_info_heads[op_id];
				LOG(3, "%s: run_stack[%zd] <- %u, path_info_head %u\n",
				    __func__, i, op_id, cur_path_info_heads[op_id]);
				assert(next_path_info_heads[op_id] < vm->paths.ceil);
				next_path_info_heads[op_id] = NO_ID; /* move reference */
#if CAPVM_STATS
				vm->uniq_ids[PAIR_ID_CURRENT][op_id] =
				    vm->uniq_ids[PAIR_ID_NEXT][op_id];
#endif
				run_stack[next_stack_h - i - 1 - discarded] = op_id;
			}
			*stack_h = next_stack_h - discarded;
			vm->run_stacks_h[PAIR_ID_NEXT] = 0;

#if CAPVM_PATH_STATS
			/* reset counters */
			for (size_t i = 0; i < vm->paths.ceil; i++) {
				struct capvm_path_info *pi = &vm->paths.pool[i];
				if (IS_PATH_NODE(pi)) {
					pi->u.path.bits_added_per_input_character = 0;
				}
			}
#endif
		}

		uint32_t *path_info_heads = vm->path_info_heads[PAIR_ID_CURRENT];
		while (vm->run_stacks_h[PAIR_ID_CURRENT] > 0) {
			/* Do this here, before popping, so that the reference
			 * on the stack can be counted properly. */
			if (EXPENSIVE_CHECKS) {
				check_path_table(vm);
			}

			const uint32_t h = --(*stack_h);
			assert(h < vm->p->used);
			const uint32_t op_id = run_stack[h];
			LOG(4, "%s: popped op_id %d off stack\n", __func__, op_id);
			if (op_id == NO_ID) {
				LOG(4, "%s: ignoring halted pending execution\n", __func__);
				continue;
			}
			assert(op_id < vm->p->used);

			if (GET_BIT32(vm->evaluated[PAIR_ID_CURRENT], op_id)) {
				LOG(2, "%s: evaluated[current] already set for op_id %u (popped off stack), skipping\n",
				    __func__, op_id);
				assert(!"unreachable");
				continue;
			}

			LOG(4, "%s: setting evaluated[current] for op_id %u (popped off stack)\n", __func__, op_id);
			SET_BIT32(vm->evaluated[PAIR_ID_CURRENT], op_id);

			const uint32_t path_info_head = path_info_heads[op_id];
			LOG(4, "%s: op_id %d's path_info_head: %d\n", __func__, op_id, path_info_head);
			path_info_heads[op_id] = NO_ID;


#if CAPVM_STATS
			const uint32_t uniq_id = vm->uniq_ids[PAIR_ID_CURRENT][op_id];
			assert(uniq_id != NO_ID);
#else
			const uint32_t uniq_id = 0;
#endif
			eval_vm_advance_greediest(vm, i_i, path_info_head, uniq_id, op_id);
		}


#if CAPVM_PATH_STATS
		uint32_t max_path_bits_added = 0;
		for (size_t i = 0; i < vm->paths.ceil; i++) {
			const struct capvm_path_info *pi = &vm->paths.pool[i];
			if (IS_PATH_NODE(pi)) {
				if (pi->u.path.bits_added_per_input_character > max_path_bits_added) {
					max_path_bits_added = pi->u.path.bits_added_per_input_character;
				}
			}
		}
		LOG(2, "%s: input_i %u: max_path_bits_added: %u\n",
		    __func__, i_i, max_path_bits_added);
		if (max_path_bits_added > vm->stats.max_bits_added_per_input_character) {
			vm->stats.max_bits_added_per_input_character = max_path_bits_added;
		}

		if (CAPVM_PATH_STATS > 1) {
			dump_path_table(stderr, vm);
		}
#endif
	}

	return vm->solution.best_path_id != NO_ID;
}

static bool
copy_path_info(struct capvm *vm, uint32_t path_info_head,
    uint32_t *new_path_info_head)
{
	if (!reserve_path_info_link(vm, new_path_info_head)) {
		return false;
	}

	assert(path_info_head != NO_ID);
	assert(path_info_head < vm->paths.ceil);
	assert(*new_path_info_head < vm->paths.ceil);
	assert(*new_path_info_head != path_info_head);

	/* Since this is the path head, it can never be a collapsed
	 * zero prefix node. */
	const struct capvm_path_info *pi = &vm->paths.pool[path_info_head];
	assert(IS_PATH_NODE(pi));

	struct capvm_path_info *npi = &vm->paths.pool[*new_path_info_head];
	assert(IS_PATH_FREELIST(npi));

	/* unlink from freelist */
	vm->paths.freelist_head = npi->u.freelist_node.freelist;
	vm->paths.live++;
	if (vm->paths.live > vm->paths.max_live) {
		set_max_paths_live(vm);
	}

	*npi = (struct capvm_path_info){
		.u.path = {
			.refcount = 1,
			.used = pi->u.path.used,
			.bits = pi->u.path.bits,
			.offset = pi->u.path.offset,
			.backlink = pi->u.path.backlink,
		}
	};
	if (pi->u.path.backlink != NO_ID) {
		inc_path_node_refcount(vm, pi->u.path.backlink);
	}
	return true;
}

#if CAPVM_PATH_STATS
static void
update_max_path_length_memory(struct capvm *vm, const struct capvm_path_info *pi)
{
	const uint32_t len = pi->u.path.used +
	    (pi->u.path.backlink == COLLAPSED_ZERO_PREFIX_ID
		? 0		/* not actually stored, so don't count it */
		: pi->u.path.offset);

	if (len > vm->stats.max_path_length_memory) {
		vm->stats.max_path_length_memory = len;
	}
}
#endif

static bool
extend_path_info(struct capvm *vm, uint32_t pi_id, bool greedy, uint32_t uniq_id,
    uint32_t *new_path_info_head)
{
	assert(pi_id < vm->paths.ceil);
	struct capvm_path_info *pi = &vm->paths.pool[pi_id];
	assert(IS_PATH_NODE(pi));

	(void)uniq_id;
	LOG_EXEC_PATH_FIND_SOLUTION(uniq_id, greedy);

#define LOG_EPI 0
	LOG(5 - LOG_EPI, "%s: pi_id %u, greedy %d\n",
	    __func__, pi_id, greedy);


	if (pi->u.path.used == PATH_LINK_BITS) { /* full */
		uint32_t npi_id;
		if (!reserve_path_info_link(vm, &npi_id)) {
			assert(!"alloc fail");
		}
		pi = &vm->paths.pool[pi_id]; /* refresh stale pointer */
		LOG(5 - LOG_EPI, "%s: new head at %u (%u is full)\n", __func__, npi_id, pi_id);
		assert(npi_id < vm->paths.ceil);
		struct capvm_path_info *npi = &vm->paths.pool[npi_id];
		vm->paths.freelist_head = npi->u.freelist_node.freelist;
		vm->paths.live++;
		if (vm->paths.live > vm->paths.max_live) {
			set_max_paths_live(vm);
		}

		LOG(5 - LOG_EPI, "%s: npi_id %u refcount 1 (new link)\n",
		    __func__, npi_id);
		npi->u.path.refcount = 1;
		npi->u.path.offset = pi->u.path.offset + pi->u.path.used;
		npi->u.path.bits = (greedy ? ((uint32_t)1 << 31) : 0);
		LOG(5 - LOG_EPI, "%s: bits after: 0x%08x\n", __func__, npi->u.path.bits);
		npi->u.path.used = 1;

#if CAPVM_PATH_STATS
		npi->u.path.bits_added_per_input_character = pi->u.path.bits_added_per_input_character + 1;
#endif

		/* If the path node is full of zero bits and it's either at the start,
		 * or its backlink is a COLLAPSED_ZERO_PREFIX_ID, then extend the
		 * backlink to a collapsed run of zeroes. The node's offset field
		 * indicates the prefix length. Long prefixes of zero bits tend to
		 * occur with an unanchored start loop. */
		if (pi->u.path.bits == (uint32_t)0 && USE_COLLAPSED_ZERO_PREFIX
		    && (pi->u.path.offset == 0 || pi->u.path.backlink == COLLAPSED_ZERO_PREFIX_ID)) {
			release_path_info_link(vm, &pi_id);
			pi_id = COLLAPSED_ZERO_PREFIX_ID;

#if CAPVM_STATS
			vm->stats.collapsed_zero_prefixes++;
#endif
		} else {
			/* Check if there's an existing full path node with
			 * exactly the same bits. If so, link backward to that
			 * and free the old full one, rather than saving it as
			 * a duplicate. */
			const uint32_t old_path_bits = pi->u.path.bits;
			const uint32_t old_path_offset = pi->u.path.offset;
			const uint32_t old_path_backlink = pi->u.path.backlink;

			for (uint32_t epi_id = 0; epi_id < vm->paths.ceil; epi_id++) {
				if (epi_id == pi_id) { continue; }
				struct capvm_path_info *epi = &vm->paths.pool[epi_id];
				if (IS_PATH_FREELIST(epi)) {
					continue;
				}

				assert(IS_PATH_NODE(epi));
				if (epi->u.path.used == PATH_LINK_BITS
				    && epi->u.path.bits == old_path_bits
				    && epi->u.path.offset == old_path_offset
				    && epi->u.path.backlink == old_path_backlink) {

					if (LOG_CAPVM >= 4 || 1) {
						const uint32_t refcount = get_path_node_refcount(vm, epi_id);
						LOG(4 - LOG_EPI, "%s: pi[%u] refcount %u -> %u (reusing identical path backlink %u instead of %u)\n",
						    __func__, epi_id, refcount, refcount + 1,
						    epi_id, pi_id);
					}
					inc_path_node_refcount(vm, epi_id);
					release_path_info_link(vm, &pi_id);
					pi_id = epi_id;
#if CAPVM_STATS
					vm->stats.path_prefixes_shared++;
#endif
					break;
				}
			}
		}

		assert(IS_PATH_NODE(npi));
		npi->u.path.backlink = pi_id;
		/* transfer pi_id's reference to npi_id */
		*new_path_info_head = npi_id;

#if CAPVM_PATH_STATS
		update_max_path_length_memory(vm, npi);
#endif

		return true;
	} else {
		assert(IS_PATH_NODE(pi));
		assert(pi->u.path.used < PATH_LINK_BITS);

		LOG(5 - LOG_EPI, "%s: appending to head node %u, %u -> %u used\n",
		    __func__, pi_id, pi->u.path.used, pi->u.path.used + 1);
		assert(pi->u.path.used < PATH_LINK_BITS);
		if (greedy) {
			LOG(5 - LOG_EPI, "%s: bits before: 0x%08x (greedy: %d)\n",
			    __func__, pi->u.path.bits, greedy);
			pi->u.path.bits |= (uint32_t)1 << (31 - pi->u.path.used);
			LOG(5 - LOG_EPI, "%s: bits after: 0x%08x\n",
			    __func__, pi->u.path.bits);
		}
		pi->u.path.used++;
#if CAPVM_PATH_STATS
		pi->u.path.bits_added_per_input_character++;
#endif

#if CAPVM_PATH_STATS
		update_max_path_length_memory(vm, pi);
#endif

		*new_path_info_head = pi_id;
		return true;
	}
#undef LOG_EPI
}

static void
populate_solution(struct capvm *vm)
{
	if (LOG_CAPVM >= 3) {
		fsm_capvm_program_dump(stderr, vm->p);
		LOG(0, "%s: best_path_id %d, tables:\n", __func__, vm->solution.best_path_id);
		dump_path_table(stderr, vm);
		check_path_table(vm);
		fprintf(stderr, "SOLUTION_PATH: ");
		print_path(stderr, vm, vm->solution.best_path_id);
		fprintf(stderr, "\n");
	}

#if CAPVM_PATH_STATS
	LOG(1, "%s: prog_size %u, max_path_length_memory %u (bits)\n",
	    __func__, vm->p->used, vm->stats.max_path_length_memory);
	const uint32_t uniq_id = vm->solution.best_path_uniq_id;
#else
	const uint32_t uniq_id = NO_ID;
#endif
	(void)uniq_id;

	/* Interpret the program again, but rather than using the input to
	 * drive execution, use the saved path for the primary solution. */

	/* Walk the solution path, reversing the edges temporarily so it
	 * can be executed start to finish, and truncate any bits appended
	 * after branches on the path. */
	assert(vm->solution.best_path_id != NO_ID);
	assert(vm->solution.best_path_id < vm->paths.ceil);

	uint32_t path_link = vm->solution.best_path_id;
	uint32_t next_link = NO_ID;
	uint32_t next_offset = NO_POS;
	uint32_t first_link = NO_ID;

	size_t split_count = 0;
	uint32_t zero_prefix_length = 0;

	if (LOG_CAPVM >= 3) {
		const struct capvm_path_info *pi = &vm->paths.pool[path_link];
		assert(!IS_PATH_FREELIST(pi));
		LOG(3, "%s: best_path %d, path_length %u\n",
		    __func__, vm->solution.best_path_id, pi->u.path.offset + pi->u.path.used);
		if (LOG_CAPVM > 4) {
			dump_path_table(stderr, vm);
		}
	}

	uint32_t prev;
	do {
		struct capvm_path_info *pi = &vm->paths.pool[path_link];
		assert(!IS_PATH_FREELIST(pi));
		const uint32_t prev_link = get_path_node_backlink(vm, path_link);

		if (LOG_CAPVM >= 3) {
			if (IS_PATH_NODE(pi)) {
				LOG(3, "%s (moving back), node %u: refcount %u, used %u, offset %u, backlink %d, bits '",
				    __func__, path_link, pi->u.path.refcount, pi->u.path.used,
				    pi->u.path.offset, pi->u.path.backlink);
				for (uint8_t i = 0; i < pi->u.path.used; i++) {
					const uint32_t bit = (pi->u.path.bits & ((uint32_t)1 << (31 - i)));
					LOG(3, "%c", bit ? '1' : '0');
				}
				LOG(3, "'\n");
			}
		}

		split_count += pi->u.path.used;

		if (next_link != NO_ID) {
			LOG(3, "-- setting backlink to %d\n", next_link);
			set_path_node_backlink(vm, path_link, next_link); /* point fwd */
		} else {
			LOG(3, "-- setting backlink to %d\n", NO_ID);
			set_path_node_backlink(vm, path_link, NO_ID); /* now EOL */
		}

		if (prev_link == NO_ID) {
			first_link = path_link;
			prev = prev_link;
		} else if (prev_link == COLLAPSED_ZERO_PREFIX_ID) {
			first_link = path_link;
			split_count += pi->u.path.offset;
			zero_prefix_length = pi->u.path.offset;
			prev = prev_link;
		}

		next_offset = get_path_node_offset(vm, path_link);
		next_link = path_link;
		assert(path_link != prev_link);
		path_link = prev_link;
	} while (path_link != NO_ID && path_link != COLLAPSED_ZERO_PREFIX_ID);

	/* iter forward */
	uint32_t cur = first_link;
	if (LOG_CAPVM >= 3) do {
		struct capvm_path_info *pi = &vm->paths.pool[cur];

		assert(IS_PATH_NODE(pi));
		LOG(3, "%s (moving fwd): node %u: refcount %u, used %u, offset %u, fwdlink %d, bits '",
		    __func__, cur, get_path_node_refcount(vm, cur),
		    pi->u.path.used,
		    get_path_node_offset(vm, cur),
		    get_path_node_backlink(vm, cur));
		for (uint8_t i = 0; i < pi->u.path.used; i++) {
			const uint32_t bit = (pi->u.path.bits & ((uint32_t)1 << (31 - i)));
			LOG(3, "%c", bit ? '1' : '0');
		}
		LOG(3, "'\n");

		const uint32_t next_cur = get_path_node_backlink(vm, cur);
		assert(cur != next_cur);
		cur = next_cur;	/* fwd link */
	} while (cur != NO_ID);

	/* evaluate program with forward path */
	LOG(3, "%s: split_count %zu\n", __func__, split_count);
	size_t split_i = 0;
	uint32_t prog_i = 0;
	uint32_t input_i = 0;
	size_t capture_lookup_steps = 0;
	bool done = false;

	/* This flag tracks whether an explicit newline was matched at
	 * the end of input. Normally a trailing newline is implicitly
	 * ignored in the bounds for captures, but when the regex
	 * matches a newline at the end, it must still be included. An
	 * example case where this matters is `^[^x]$` for "\n", because
	 * the character class matches the newline this should capture
	 * as (0,1). */
	bool explicitly_matched_nl_at_end = false;

	cur = first_link;
	while (split_i < split_count || !done) {
		assert(prog_i < vm->p->used);
		const uint32_t cur_prog_i = prog_i;
		const struct capvm_opcode *op = &vm->p->ops[cur_prog_i];
		LOG(3, "%s: i_i %u, p_i %u, s_i %zu/%zu, op %s\n",
		    __func__, input_i, cur_prog_i, split_i, split_count, op_name[op->t]);

		prog_i++;
		capture_lookup_steps++;
		switch (op->t) {
		case CAPVM_OP_CHAR:
			assert(input_i < vm->input_len);
			LOG(3, "OP_CHAR: input_i %u, exp char '%c', got '%c'\n",
			    input_i, op->u.chr, vm->input[input_i]);
			assert(vm->input[input_i] == op->u.chr);
			if (vm->input_len > 0
			    && input_i == vm->input_len - 1
			    && vm->input[input_i] == '\n') {
				explicitly_matched_nl_at_end = true;
			}
			input_i++;
			break;
		case CAPVM_OP_CHARCLASS:
			assert(input_i < vm->input_len);
			if (vm->input_len > 0
			    && input_i == vm->input_len - 1
			    && vm->input[input_i] == '\n') {
				explicitly_matched_nl_at_end = true;
			}
			input_i++;
			break;
		case CAPVM_OP_MATCH:
			LOG(2, "split_i %zu, split_count %zu\n", split_i, split_count);
			assert(split_i == split_count);
			done = true;
			break;
		case CAPVM_OP_JMP:
			prog_i = op->u.jmp;
			break;
		case CAPVM_OP_JMP_ONCE:
		{
			/* look at next bit of path and jmp or fall through */
			const uint32_t offset = get_path_node_offset(vm, cur);
			const struct capvm_path_info *pi = &vm->paths.pool[cur];

			assert(IS_PATH_NODE(pi));
			bool next_bit;
			LOG(3, "%s: OP_JMP_ONCE: split_i %zu, zpl %u, offset %u, pi->u.path.used %u\n",
			    __func__, split_i, zero_prefix_length, offset, pi->u.path.used);
			if (split_i < zero_prefix_length) {
				next_bit = 0;
			} else {
				assert(split_i >= offset &&
				    split_i <= offset + pi->u.path.used);
				const uint32_t shift = 31 - (split_i & 31);
				assert(shift < PATH_LINK_BITS);
				next_bit = (pi->u.path.bits & ((uint32_t)1 << shift)) != 0;
			}
			LOG(3, "jmp_once: next_bit %d\n", next_bit);
			LOG_EXEC_PATH_SAVE_CAPTURES(uniq_id, next_bit);
			if (next_bit) { /* greedy edge */
				prog_i = op->u.jmp_once;
			} else { /* non-greedy edge */
				/* fall through */
			}
			split_i++;
			if (split_i >= offset &&
			    split_i - offset == pi->u.path.used && split_i < split_count) {
				const uint32_t backlink = get_path_node_backlink(vm, cur);
				assert(backlink != NO_ID);
				cur = backlink;
			}
			LOG(3, "%s: prog_i now %u, split_i %zu/%zu\n",
			    __func__, prog_i, split_i, split_count);
			assert(split_i <= split_count);
			break;
		}
		case CAPVM_OP_SPLIT:
		{
			/* look at next bit of path and act accordingly */
			const uint32_t offset = get_path_node_offset(vm, cur);
			const struct capvm_path_info *pi = &vm->paths.pool[cur];

			const uint32_t dst_cont = op->u.split.cont;
			const uint32_t dst_new = op->u.split.new;

			assert(IS_PATH_NODE(pi));
			bool next_bit;
			LOG(3, "%s: OP_SPLIT_CONT: split_i %zu, zpl %u, offset %u, pi->u.path.used %u\n",
			    __func__, split_i, zero_prefix_length, offset, pi->u.path.used);
			if (split_i < zero_prefix_length) {
				next_bit = 0;
			} else {
				assert(split_i >= offset &&
				    split_i <= offset + pi->u.path.used);
				const uint32_t shift = 31 - (split_i & 31);
				assert(shift < PATH_LINK_BITS);
				next_bit = (pi->u.path.bits & ((uint32_t)1 << shift)) != 0;
			}
			LOG(3, "split: next_bit %d\n", next_bit);
			LOG_EXEC_PATH_SAVE_CAPTURES(uniq_id, next_bit);
			if (next_bit) { /* greedy edge */
				prog_i = dst_cont;
			} else { /* non-greedy edge */
				prog_i = dst_new;
			}
			split_i++;
			if (split_i >= offset &&
			    split_i - offset == pi->u.path.used && split_i < split_count) {
				const uint32_t backlink = get_path_node_backlink(vm, cur);
				assert(backlink != NO_ID);
				cur = backlink;
			}
			LOG(3, "%s: prog_i now %u, split_i %zu/%zu\n",
			    __func__, prog_i, split_i, split_count);
			assert(split_i <= split_count);

			break;
		}
		case CAPVM_OP_SAVE:
		{
			const unsigned capture_id = op->u.save/2;
			const bool is_end = (op->u.save & 1) == 1;

			LOG(5, "%s: input_i %u, save %d -> capture %d pos %d, cur value %zd, prev char 0x%02x\n",
			    __func__,
			    input_i, op->u.save,
			    capture_id, is_end,
			    vm->capture_buf[op->u.save/2].pos[op->u.save & 1],
			    input_i > 0 ? vm->input[input_i - 1] : 0xff);

			/* Special case to ignore a trailing
			 * newline when capturing, unless the
			 * newline was explicitly matched as the
			 * last character of input. */
			if (input_i > 0
			    && !explicitly_matched_nl_at_end
			    && input_i == vm->input_len
			    && vm->input[input_i - 1] == '\n') {
				LOG(3, "%s: updating capture[%u].pos[1] to ignore trailing '\\n' at %u\n",
				    __func__, capture_id, input_i);
				vm->capture_buf[capture_id].pos[is_end] = input_i - 1;
			} else {
				/* Save current position to appropriate capture buffer endpoint */
				vm->capture_buf[op->u.save/2].pos[op->u.save & 1] = input_i;
				LOG(3, "%s: saved capture[%d].pos[%d] <- %u\n",
				    __func__, op->u.save/2, op->u.save&1, input_i);
			}
			break;
		}
		case CAPVM_OP_ANCHOR:
			if (op->u.anchor == CAPVM_ANCHOR_START) {
				assert(input_i == 0
				    || (input_i == 1
					&& vm->input_len == 1
					&& vm->input[0] == '\n'));
			} else {
				assert(op->u.anchor == CAPVM_ANCHOR_END);
				LOG(3, "%s: $ anchor: input_len %u, input_i %u\n",
				    __func__, vm->input_len, input_i);

				if (vm->input_len > 0 && input_i == vm->input_len - 1) {
					/* special hack to not include trailing newline
					 * in match group zero */
					if (vm->p->capture_count > 0) {
						vm->capture_buf[0].pos[1] = input_i;
					}

					assert(vm->input[input_i] == '\n');
					input_i++;
				} else {
					assert(input_i == vm->input_len);
				}
			}
			break;

		default:
			assert(!"match fail");
		}
	}

	/* write solution into caller's buffers and print */
	if (LOG_SOLUTION_TO_STDOUT) {
		/* fprintf(stderr, "capture_count %u\n", vm->p->capture_count); */
		printf("HIT:");
		for (unsigned i = 0; i < vm->p->capture_count; i++) {
			printf(" %zd %zd",
			    vm->capture_buf[i].pos[0], vm->capture_buf[i].pos[1]);
		}
		printf("\n");
	}

	/* restore original link order */
	cur = first_link;
	do {
		struct capvm_path_info *pi = &vm->paths.pool[cur];
		assert(!IS_PATH_FREELIST(pi));
		const uint32_t backlink = get_path_node_backlink(vm, cur);

		LOG(3, "%s (moving fwd again): node %u: refcount %u, used %u, offset %u, fwdlink %d, bits '",
		    __func__, cur, get_path_node_refcount(vm, cur),
		    pi->u.path.used,
		    get_path_node_offset(vm, cur),
		    backlink);
		for (uint8_t i = 0; i < pi->u.path.used; i++) {
			const uint32_t bit = (pi->u.path.bits & ((uint32_t)1 << (31 - i)));
			LOG(3, "%c", (pi->u.path.bits & bit) ? '1' : '0');
		}
		LOG(3, "'\n");

		LOG(3, "-- setting node %u's backlink to %d\n", cur, prev);
		const uint32_t next = backlink;
		set_path_node_backlink(vm, cur, prev);

		prev = cur;
		cur = next; /* fwd link */
	} while (cur != NO_ID);
}

/* TODO: It should be possible to avoid dynamic allocation here
 * by calculating the max space needed upfront and passing in a
 * uint32_t or uint64_t-aligned array for working space. */

enum fsm_capvm_program_exec_res
fsm_capvm_program_exec(const struct capvm_program *program,
    const uint8_t *input, size_t length,
    struct fsm_capture *capture_buf, size_t capture_buf_length)
{
	assert(program != NULL);
	assert(input != NULL || length == 0);
	assert(capture_buf != NULL);

	const size_t thread_max = program->used;

	/* FIXME: The path node table can grow beyond this, but in
	 * practice will usually stay fairly small. The worst case
	 * should be decidable based on the compiled program and input
	 * length. */
#if ALLOW_PATH_TABLE_RESIZING
	const size_t path_info_max = thread_max;
#else
	const size_t path_info_max = 3 * thread_max;
#endif

	struct capvm_path_info *path_info_pool = malloc(path_info_max
	    * sizeof(path_info_pool[0]));
	if (path_info_pool == NULL) {
		return FSM_CAPVM_PROGRAM_EXEC_ERROR_ALLOC;
	}
	assert(path_info_pool != NULL);

	/* link path_info freelist */
	for (size_t i = 1; i < path_info_max - 1; i++) {
		struct capvm_path_info *pi = &path_info_pool[i];
		pi->u.freelist_node.refcount = 0;
		pi->u.freelist_node.freelist = i + 1;
	}
	struct capvm_path_info *piZ = &path_info_pool[path_info_max - 1];
	piZ->u.freelist_node.refcount = 0;
	piZ->u.freelist_node.freelist = NO_ID;

	/* init an empty path descriptor for initial execution */
	struct capvm_path_info *pi0 = &path_info_pool[0];
	pi0->u.path.refcount = 1;
	pi0->u.path.used = 0;
	pi0->u.path.bits = 0;
	pi0->u.path.offset = 0;
	pi0->u.path.backlink = NO_ID;

	uint32_t stack_a[thread_max];
	uint32_t stack_b[thread_max];

	const size_t evaluated_bit_words = program->used/32 + 1;
	uint32_t evaluated_a[evaluated_bit_words];
	uint32_t evaluated_b[evaluated_bit_words];
	uint32_t path_info_head_a[thread_max];
	uint32_t path_info_head_b[thread_max];
#if CAPVM_STATS
	uint32_t uniq_ids_a[thread_max];
	uint32_t uniq_ids_b[thread_max];
#endif

	assert(capture_buf_length >= program->capture_base + program->capture_count);

	struct fsm_capture *offset_capture_buf = &capture_buf[program->capture_base];

	struct capvm vm = {
		.res = FSM_CAPVM_PROGRAM_EXEC_NO_SOLUTION_FOUND,
		.p = program,
		.input = input,
		.input_len = length,
		.capture_buf = offset_capture_buf,
		.capture_buf_length = capture_buf_length,
		.step_limit = SIZE_MAX,
#if CAPVM_STATS
		.uniq_id_counter = 0,
#endif

		.run_stacks = { stack_a, stack_b },
		.evaluated = { evaluated_a, evaluated_b },
		.path_info_heads = { path_info_head_a, path_info_head_b },
#if CAPVM_STATS
		.uniq_ids = { uniq_ids_a, uniq_ids_b },
#endif

		.paths = {
			.ceil = path_info_max,
			.live = 1,
			.max_live = 1,
			.freelist_head = 1,
			.pool = path_info_pool,
		},
		.solution = {
			.best_path_id = NO_ID,
		},
	};

	/* enqueue execution at first opcode */
	vm.run_stacks[PAIR_ID_NEXT][0] = 0;
	vm.run_stacks_h[PAIR_ID_NEXT] = 1;
	vm.threads.live = 1;
	vm.threads.max_live = 1;
	vm.path_info_heads[PAIR_ID_NEXT][0] = 0;

#if CAPVM_STATS
	vm.uniq_ids[PAIR_ID_NEXT][0] = 0;
#endif

	INIT_TIMERS();
	TIME(&pre);
	if (eval_vm(&vm)) {
		assert(vm.threads.live == 0);
		assert(vm.paths.live > 0);

		populate_solution(&vm);
		release_path_info_link(&vm, &vm.solution.best_path_id);
		vm.res = FSM_CAPVM_PROGRAM_EXEC_SOLUTION_WRITTEN;

		/* TODO: This assert will not work if refcounts are
		 * sticky at the max value, but if the number of paths
		 * and threads is bounded then it shouldn't be possible
		 * to overflow the refcount anyway. If sticky refcounts
		 * are used then reaching one should probably set a
		 * flag, which would skip this assertion. */
		assert(vm.paths.live == 0);
	} else {
		assert(vm.res == FSM_CAPVM_PROGRAM_EXEC_NO_SOLUTION_FOUND);
	}

	TIME(&post);
	DIFF_MSEC(__func__, pre, post, NULL);

#if CAPVM_STATS
	LOG(2, "%s: %zu steps, max_threads %u, max_paths %u, matches %u, path_prefixes_shared %u, collapsed_zero_prefixes %u\n",
	    __func__, vm.stats.steps, vm.threads.max_live, vm.paths.max_live, vm.stats.matches,
	    vm.stats.path_prefixes_shared, vm.stats.collapsed_zero_prefixes);
#if CAPVM_PATH_STATS
	LOG(2, "%s: prog_size %u, max_path_length_memory %u (bits), input length %zu, max_paths * %zu bytes/path => %zu bytes\n",
	    __func__, vm.p->used, vm.stats.max_path_length_memory, length,
	    sizeof(vm.paths.pool[0]),
	    vm.paths.max_live * sizeof(vm.paths.pool[0]));
#endif
#endif

	free(vm.paths.pool);
	return vm.res;
}
