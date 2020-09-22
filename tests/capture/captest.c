#include "captest.h"

#include <fsm/alloc.h>
#include <fsm/capture.h>

#define FAIL(MSG)					\
	fprintf(stderr, "FAIL: %s:%d -- %s\n",	\
	    __FILE__, __LINE__, MSG);		\
	exit(EXIT_FAILURE)

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

	for (i = 0; i < capture_count; i++) {
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

	eo = calloc(1, sizeof(*eo));
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
	/* fprintf(stderr, "fsm_of_string: set opaque to %p for \"%s\"\n", */
	/*     (void *)eo, string); */

	return fsm;

cleanup:
	if (eo != NULL) { free(eo); }
	fsm_free(fsm);
	return NULL;
}

static struct fsm_options options;

static void captest_carryopaque(struct fsm *src_fsm,
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

	eo_dst = calloc(1, sizeof(*eo_dst));
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

		/* FIXME: freeing here leads to a use after free */
		/* f_free(opt->alloc, eo_old_dst); */
	}

	fsm_setopaque(dst_fsm, dst_state, eo_dst);

	/* union bits set in eo_src->ends into eo_dst->ends and free */
	for (i = 0; i < n; i++) {
		eo_src = fsm_getopaque(src_fsm, src_set[i]);
		if (eo_src == NULL) {
			continue;
		}
		if (log_level > 0) {
			fprintf(stderr, "carryopaque: dst %p <- src[%lu] %p\n",
			    (void *)eo_dst, i, (void *)eo_src);
		}
		assert(eo_src->tag == CAPTEST_END_OPAQUE_TAG);
		eo_dst->ends |= eo_src->ends;

		/* FIXME: freeing here leads to a use after free */
		/* f_free(opt->alloc, eo_src); */

		fsm_setopaque(src_fsm, src_set[i], NULL);
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
