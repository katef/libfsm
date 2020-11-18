#include "captest.h"

#include <fsm/alloc.h>
#include <fsm/capture.h>
#include <fsm/pred.h>

#if CAPTEST_RUN_SINGLE_LOG
#include <fsm/print.h>
#endif

#define FAIL(MSG)					\
	fprintf(stderr, "FAIL: %s:%d -- %s\n",	\
	    __FILE__, __LINE__, MSG);		\
	exit(EXIT_FAILURE)

/* Pool allocator for opaque data to eliminates inadvertent memory leaks.
 *
 * XXX - this isn't great, but it's quite hard to keep track of opaque
 *       data through the various fsm operations.
 *
 *       The right solution is probably to have each fsm keep an opaque
 *       "destructor" that it calls when the states of an FSM are
 *       destroyed.
 */
struct captest_opaque_pool;

enum { CAPTEST_OPAQUE_POOL_SIZE = 128 };
struct captest_opaque_pool {
	struct captest_opaque_pool *next;
	struct captest_end_opaque items[CAPTEST_OPAQUE_POOL_SIZE];
	unsigned count;
};

struct captest_opaque_pool *pool = NULL;

struct captest_end_opaque *
captest_new_opaque(void)
{
	struct captest_end_opaque *eo;

	if (pool == NULL || pool->count == CAPTEST_OPAQUE_POOL_SIZE) {
		struct captest_opaque_pool *new_pool;
		new_pool = calloc(1, sizeof *new_pool);

		if (new_pool == NULL) {
			return NULL;
		}

		new_pool->next = pool;
		new_pool->count = 0;
		pool = new_pool;
	}

	assert(pool != NULL);
	assert(pool->count < CAPTEST_OPAQUE_POOL_SIZE);

	eo = &pool->items[pool->count];
	pool->count++;

	return eo;
}

void
captest_free_all_end_opaques(void)
{
	struct captest_opaque_pool *curr;

	curr = pool;
	while (curr != NULL) {
		struct captest_opaque_pool *next;
		next = curr->next;

		free(curr);
		curr = next;
	}

	pool = NULL;
}

int
captest_getc(void *opaque)
{
	struct captest_input *input = opaque;
	int res = input->string[input->pos];
	input->pos++;
	return res == 0 ? EOF : res;
}

int
captest_run_single(const struct captest_single_fsm_test_info *info)
{
	size_t i;
	struct captest_input input;
	fsm_state_t end;
	int exec_res;
	struct fsm_capture got_captures[MAX_TEST_CAPTURES];
	struct fsm_capture exp_captures[MAX_TEST_CAPTURES];
	size_t capture_count = 0;
	struct fsm *fsm = captest_fsm_of_string(info->string, 0);

	input.string = info->string;
	input.pos = 0;

	if (fsm == NULL) {
		FAIL("fsm_of_string");
	}

	for (i = 0; i < MAX_TEST_CAPTURES; i++) {
		exp_captures[i].pos[0] = FSM_CAPTURE_NO_POS;
		exp_captures[i].pos[1] = FSM_CAPTURE_NO_POS;
	}

	for (i = 0; i < MAX_SINGLE_FSM_TEST_PATHS; i++) {
		const struct captest_single_fsm_test_path *path =
		    &info->paths[i];
		if (path->start == 0 && path->end == 0 && i > 0) {
			break;	/* end of list */
		}

		/* no zero-width captures */
		assert(path->end > path->start);

		if (!fsm_capture_set_path(fsm, i,
			path->start, path->end)) {
			fprintf(stderr,
			    "failed to set capture path %lu\n", i);
			FAIL("fsm_capture_set_path");
		}

		exp_captures[i].pos[0] = path->start;
		exp_captures[i].pos[1] = path->end;

		capture_count = i + 1;
	}

	{
		const unsigned count = fsm_countcaptures(fsm);
		const unsigned expected = capture_count;
		if (count != expected) {
			fprintf(stderr, "expected %u, got %u\n",
			    expected, count);
			FAIL("countcaptures");
		}
	}

#if CAPTEST_RUN_SINGLE_LOG
	fsm_print_fsm(stderr, fsm);
	fsm_capture_dump(stderr, "fsm", fsm);
#endif

	exec_res = fsm_exec(fsm, captest_getc, &input, &end, got_captures);
	if (exec_res != 1) { FAIL("exec_res"); }
	if (end != strlen(info->string)) { FAIL("exec end pos"); }

	{
		struct captest_end_opaque *eo = fsm_getopaque(fsm, end);
		assert(eo != NULL);
		assert(eo->tag == CAPTEST_END_OPAQUE_TAG);
		if (!(eo->ends & (1U << 0))) {
			FAIL("end ID not set in opaque");
		}
	}

	{
		fsm_end_id_t id_buf[1] = { ~0 };
		enum fsm_getendids_res gres;
		size_t written;
		if (1 != fsm_getendidcount(fsm, end)) {
			FAIL("did not have exactly one end ID");
		}

		gres = fsm_getendids(fsm, end, 1, id_buf, &written);
		if (gres != FSM_GETENDIDS_FOUND) {
			FAIL("failed to get end IDs");
		}

		if (0 != id_buf[0]) {
			FAIL("failed to get end ID of 0");
		}
	}

	for (i = 0; i < capture_count; i++) {
#if CAPTEST_RUN_SINGLE_LOG
		fprintf(stderr, "captest: capture %lu: exp (%ld, %ld), got (%ld, %ld)\n",
		    i, exp_captures[i].pos[0], exp_captures[i].pos[1],
		    got_captures[i].pos[0], got_captures[i].pos[1]);
#endif
		if (got_captures[i].pos[0] != exp_captures[i].pos[0]) {
			fprintf(stderr, "capture[%lu].pos[0]: exp %lu, got %lu\n",
			    i, exp_captures[i].pos[0],
			    got_captures[i].pos[0]);
			FAIL("capture mismatch");
		}
		if (got_captures[i].pos[1] != exp_captures[i].pos[1]) {
			fprintf(stderr, "capture[%lu].pos[1]: exp %lu, got %lu\n",
			    i, exp_captures[i].pos[1],
			    got_captures[i].pos[1]);
			FAIL("capture mismatch");
		}
	}

	fsm_free(fsm);
	captest_free_all_end_opaques();

	return 0;
}

struct fsm *
captest_fsm_of_string(const char *string, unsigned end_id)
{
	struct fsm *fsm = captest_fsm_with_options();
	const size_t length = strlen(string);
	size_t i;
	struct captest_end_opaque *eo = NULL;

	if (fsm == NULL) {
		return NULL;
	}

	eo = captest_new_opaque();
	if (eo == NULL) {
		goto cleanup;
	}

	eo->tag = CAPTEST_END_OPAQUE_TAG;

	/* set bit for end state */
	assert(end_id < 8*sizeof(eo->ends));
	eo->ends |= (1U << end_id);

	if (!fsm_addstate_bulk(fsm, length + 1)) {
		goto cleanup;
	}
	fsm_setstart(fsm, 0);

	for (i = 0; i < length; i++) {
		if (!fsm_addedge_literal(fsm, i, i + 1, string[i])) {
			goto cleanup;
		}
	}
	fsm_setend(fsm, length, 1);
	fsm_setopaque(fsm, length, eo);

	if (!fsm_setendid(fsm, end_id)) {
		goto cleanup;
	}

	return fsm;

cleanup:
	fsm_free(fsm);
	return NULL;
}

static struct fsm_options options;

static void captest_carryopaque(const struct fsm *src_fsm,
    const fsm_state_t *src_set, size_t n,
    struct fsm *dst_fsm, fsm_state_t dst_state)
{
	struct captest_end_opaque *eo_src = NULL;
	struct captest_end_opaque *eo_dst = NULL;
	struct captest_end_opaque *eo_old_dst = NULL;
	size_t i;
	const int log_level = 0;

	if (log_level > 0) {
		fprintf(stderr, "captest_carryopaque: src_fsm %p, src_set %p, n %lu, dst_fsm %p, dst_state %u\n",
		    (void *)src_fsm, (void *)src_set, n,
		    (void *)dst_fsm, dst_state);
	}

	eo_old_dst = fsm_getopaque(dst_fsm, dst_state);

	eo_dst = captest_new_opaque();
	/* FIXME: no way to handle an alloc error in carryopaque? */
	assert(eo_dst != NULL);
	eo_dst->tag = CAPTEST_END_OPAQUE_TAG;
	if (log_level > 0) {
		fprintf(stderr, "captest_carryopaque: new opaque %p (eo_dst)\n",
		    (void *)eo_dst);
	}

	if (eo_old_dst != NULL) {
		assert(eo_old_dst->tag == CAPTEST_END_OPAQUE_TAG);
		eo_dst->ends |= eo_old_dst->ends;
		if (log_level > 0) {
			fprintf(stderr, "carryopaque: old_dst (%d) ends 0x%x\n",
			    dst_state, eo_dst->ends);
		}
	}

	fsm_setopaque(dst_fsm, dst_state, eo_dst);

	/* union bits set in eo_src->ends into eo_dst->ends */
	for (i = 0; i < n; i++) {
		eo_src = fsm_getopaque(src_fsm, src_set[i]);
		if (eo_src == NULL) {
			continue;
		}
		if (log_level > 0) {
			fprintf(stderr, "carryopaque: dst %p (%d) <- src[%lu] (%d) %p\n",
			    (void *)eo_dst, dst_state, i, src_set[i],
			    (void *)eo_src);
		}
		assert(eo_src->tag == CAPTEST_END_OPAQUE_TAG);
		eo_dst->ends |= eo_src->ends;
		if (log_level > 0) {
			fprintf(stderr, "carryopaque: eo_src ends -> 0x%x\n",
			    eo_dst->ends);
		}
	}
}

struct fsm *
captest_fsm_with_options(void)
{
	struct fsm *fsm = NULL;
	if (options.carryopaque == NULL) { /* initialize */
		options.carryopaque = captest_carryopaque;
	}

	fsm = fsm_new(&options);
	return fsm;
}

