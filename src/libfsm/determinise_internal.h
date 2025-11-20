#ifndef DETERMINISE_INTERNAL_H
#define DETERMINISE_INTERNAL_H

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/hash.h>
#include <adt/stateset.h>
#include <adt/internedstateset.h>
#include <adt/u64bitset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"
#include "eager_output.h"

#include <ctype.h>

#define DUMP_MAPPING 0
#define LOG_INPUT 0
#define LOG_DETERMINISE_CLOSURES 0
#define LOG_DETERMINISE_CAPTURES 0
#define LOG_DETERMINISE_STATE_SETS 0
#define LOG_SYMBOL_CLOSURE 0
#define LOG_AC 0
#define LOG_GROUPING 0
#define LOG_ANALYSIS_STATS 0
#define LOG_BUILD_REVERSE_MAPPING 0

#if LOG_DETERMINISE_CAPTURES || LOG_INPUT
#include <fsm/print.h>
#endif

/* Starting number of buckets for the map's
 * hash table. Must be a power of 2. */
#define DEF_MAP_BUCKET_CEIL 4

/* Starting array size for the cleanup vector. */
#define DEF_CVECT_CEIL 4

/*
 * This maps a DFA state onto its associated NFA symbol closure, such that an
 * existing DFA state may be found given any particular set of NFA states
 * forming a symbol closure.
 *
 * Since the same state sets tend to be constructed over and over during
 * determinization, they are interned -- constructing a set with the same
 * contents will instead get a reference to an existing instance.
 */
struct map {
	const struct fsm_alloc *alloc;

	size_t used;
	size_t count;

	/* These are allocated on demand, because they're pushed onto the
	 * stack by address, and *edges is used by address. */
	struct mapping {
		interned_state_set_id iss;
		size_t dfastate;
		struct edge_set *edges;
	} **buckets;
};

struct map_iter {
	const struct map *m;
	size_t i;
};

struct reverse_mapping {
	unsigned count;
	unsigned ceil;
	fsm_state_t *list;
};

struct det_copy_capture_actions_env {
#ifndef NDEBUG
	char tag;
#endif
	struct fsm *dst;
	struct reverse_mapping *reverse_mappings;
	bool ok;
};

#define MAPPINGSTACK_DEF_CEIL 16
struct mappingstack {
	const struct fsm_alloc *alloc;
	size_t ceil;
	size_t depth;
	struct mapping **s;
};

#define AC_NO_STATE ((fsm_state_t)-1)
#define DEF_ITER_CEIL 4
#define DEF_GROUP_CEIL 4
#define DEF_OUTPUT_CEIL 4
#define DEF_DST_CEIL 4

/* Mark an ID as a cached result ID (1) rather than a state ID (0). */
#define RESULT_BIT ((fsm_state_t)1U << (8U*sizeof(fsm_state_t) - 1))
#define DEF_PBUF_CEIL 4
#define DEF_GROUP_BUFFER_CEIL 4
#define DEF_TO_SET_CEIL 4
#define DEF_GROUP_GS 4
#define DEF_PAIR_CACHE_HTAB_BUCKET_COUNT 64
#define DEF_DST_TMP_CEIL 4
#define DEF_TO_SET_HTAB_BUCKET_COUNT 64

#define ID_WITH_SUFFIX(ID) ID &~ RESULT_BIT, ID & RESULT_BIT ? "_R" : "_s"

struct analyze_closures_env;	/* fwd ref */

static int
analyze_closures__pairwise_grouping(struct analyze_closures_env *env,
	interned_state_set_id iss_id);

static int
analyze_single_state(struct analyze_closures_env *env, fsm_state_t id);

static int
build_output_from_cached_analysis(struct analyze_closures_env *env, fsm_state_t cached_result_id);

static int
add_group_dst_info_to_buffer(struct analyze_closures_env *env,
	size_t dst_count, const fsm_state_t *dst,
	const uint64_t labels[256/64]);

static int
commit_buffered_result(struct analyze_closures_env *env, uint32_t *cache_result_id);

static int
combine_pair(struct analyze_closures_env *env, fsm_state_t pa, fsm_state_t pb,
    fsm_state_t *result_id);

static int
combine_result_pair_and_commit(struct analyze_closures_env *env,
    fsm_state_t result_id_a, fsm_state_t result_id_b,
    fsm_state_t *committed_result_id);

static int
analysis_cache_check_pair(struct analyze_closures_env *env, fsm_state_t pa, fsm_state_t pb,
    fsm_state_t *result_id);

static int
analysis_cache_check_single_state(struct analyze_closures_env *env, fsm_state_t id,
	fsm_state_t *result_id);

static int
analysis_cache_save_single_state(struct analyze_closures_env *env,
	fsm_state_t state_id, fsm_state_t result_id);

static int
analysis_cache_save_pair(struct analyze_closures_env *env,
	fsm_state_t a, fsm_state_t b, fsm_state_t result_id);

struct analyze_closures_env {
	const struct fsm_alloc *alloc;
	const struct fsm *fsm;
	struct interned_state_set_pool *issp;

	/* All sets of labels leading to states,
	 * stored in ascending order. */
	size_t group_ceil;
	size_t group_count;
	struct ac_group {
		fsm_state_t to;
		/* words_used & (1U << x) -> labels[x] has bits set */
		uint8_t words_used;
		uint64_t labels[256/64];
	} *groups;

	/* A collection of (label set -> interned state set) pairs. */
	size_t output_ceil;
	size_t output_count;
	struct ac_output {
		uint64_t labels[256/64];
		interned_state_set_id iss;
	} *outputs;

	struct clearing_vector {
		size_t ceil;
		size_t used;
		fsm_state_t *ids;
	} cvect;

	/* Pair of buffers used to store IDs as the list is combined
	 * and reduced, switching back and forth. */
	size_t pbuf_ceil;
	fsm_state_t *pbuf[2];

	/* Buffer for storing unique runs of ascending state IDs. */
	struct {
		size_t used;
		size_t ceil;
		fsm_state_t *buf;
	} to_sets;

	/* Hash table for finding offsets of existing runs of ascending
	 * state IDs, since they tend to be duplicated heavily. */
	struct to_set_htab {
		size_t bucket_count;
		size_t buckets_used;
		struct to_set_bucket {
			uint32_t count; /* count of 0 -> unused */
			uint32_t offset;
		} *buckets;
	} to_set_htab;

	/* Buffer used to accumulate destination state IDs, to avoid
	 * pointing into .to_sets when doing operations that could
	 * cause .to_sets.buf to grow/relocate. */
	struct {
		size_t used;
		size_t ceil;
		fsm_state_t *buf;
	} dst_tmp;

	/* Hash table for {id, id} -> result_id, where id is
	 * either an fsm_state_t or (result_id | RESULT_BIT).
	 *
	 * state IDs are always distinct and stored in ascending
	 * order, so a second ID of 0 marks an empty bucket.
	 * manages collisions with linear probing, grows when half full. */
	struct result_pair_htab {
		size_t bucket_count;
		size_t buckets_used;
		struct result_pair_bucket {
			enum result_pair_bucket_type {
				RPBT_UNUSED,
				RPBT_SINGLE_STATE,
				RPBT_PAIR,
			} t;
			fsm_state_t ids[2]; /* hash key */
			fsm_state_t result_id; /* < .groups.used */
		} *buckets;
	} htab;

	/* Cached results from analysing a state ID or pair of IDs. */
	struct {
		size_t used;
		size_t ceil;
		struct analysis_result {
			uint32_t count;
			struct result_entry {
				uint32_t to_set_offset;
				uint32_t to_set_count;
				uint8_t words_used;
				uint64_t labels[256/64];
			} entries[];
		} **rs;

		/* Buffer for building up the current result. */
		struct result_buffer {
			uint32_t used;
			uint32_t ceil;
			struct result_entry *entries;
		} buffer;
	} results;

	size_t count_single;
	size_t usec_single;
	size_t count_pair;
	size_t usec_pair;

	/* Buffer for building up destination state set. */
	size_t dst_ceil;
	fsm_state_t *dst;
};

static int
analyze_closures__init_groups(struct analyze_closures_env *env);

static int
analyze_closures__save_output(struct analyze_closures_env *env,
    const uint64_t labels[256/64], interned_state_set_id iss);

static int
analyze_closures__grow_groups(struct analyze_closures_env *env);

static int
analyze_closures__grow_dst(struct analyze_closures_env *env);

static int
analyze_closures__grow_outputs(struct analyze_closures_env *env);

static int
map_add(struct map *map,
	fsm_state_t dfastate, interned_state_set_id iss, struct mapping **new_mapping);

static int
map_find(const struct map *map, interned_state_set_id iss,
	struct mapping **mapping);

static void
map_free(struct map *map);

static struct mapping *
map_first(const struct map *map, struct map_iter *iter);

static struct mapping *
map_next(struct map_iter *iter);

static int
add_reverse_mapping(const struct fsm_alloc *alloc,
	struct reverse_mapping *reverse_mappings,
	fsm_state_t dfastate, fsm_state_t nfa_state);

static int
det_copy_capture_actions(struct reverse_mapping *reverse_mappings,
	struct fsm *dst, struct fsm *src);

static int
grow_map(struct map *map);

static int
remap_capture_actions(struct map *map, struct interned_state_set_pool *issp,
	struct fsm *dst_dfa, struct fsm *src_nfa);

static int
remap_eager_outputs(const struct map *map, struct interned_state_set_pool *issp,
	struct fsm *dst_dfa, const struct fsm *src_nfa);

static struct mappingstack *
stack_init(const struct fsm_alloc *alloc);

static void
stack_free(struct mappingstack *stack);

static int
stack_push(struct mappingstack *stack, struct mapping *item);

static struct mapping *
stack_pop(struct mappingstack *stack);

#endif
