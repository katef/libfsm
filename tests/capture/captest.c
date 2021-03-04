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

	return 0;
}

struct fsm *
captest_fsm_of_string(const char *string, unsigned end_id)
{
	struct fsm *fsm = captest_fsm_with_options();
	const size_t length = strlen(string);
	size_t i;

	if (fsm == NULL) {
		return NULL;
	}

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

	if (!fsm_setendid(fsm, end_id)) {
		goto cleanup;
	}

	return fsm;

cleanup:
	fsm_free(fsm);
	return NULL;
}

static struct fsm_options options;

struct fsm *
captest_fsm_with_options(void)
{
	struct fsm *fsm = NULL;

	/* We currently don't need to set anything custom on this. */
	fsm = fsm_new(&options);
	return fsm;
}

int
captest_check_single_end_id(const struct fsm *fsm, fsm_state_t end_state,
    unsigned expected_end_id, const char **msg)
{
	fsm_end_id_t id_buf[1] = { ~0 };
	enum fsm_getendids_res gres;
	size_t written;
	const char *unused;

	if (msg == NULL) {
		msg = &unused;
	}

	if (1 != fsm_getendidcount(fsm, end_state)) {
		*msg = "did not have exactly one end ID";
		return 0;
	}

	gres = fsm_getendids(fsm, end_state, 1, id_buf, &written);
	if (gres != FSM_GETENDIDS_FOUND) {
		*msg = "failed to get end IDs";
		return 0;
	}

	if (expected_end_id != id_buf[0]) {
		*msg = "failed to get expected end ID";
		return 0;
	}

	return 1;
}
