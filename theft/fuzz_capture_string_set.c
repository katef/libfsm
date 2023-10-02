#include "type_info_capture_string_set.h"

#include <fsm/capture.h>
#include <fsm/print.h>

#define LOG_LEVEL 0

#define INIT_CAPTURES ((size_t)-2)

static enum theft_trial_res
prop_css_invariants(struct theft *t, void *arg1);

static enum theft_trial_res
check_capstring_set(struct capture_env *env,
    const struct capture_string_set *css,
    const char *input, bool check_others_for_fps,
    struct fsm_combined_base_pair *capture_bases,
    struct fsm_capture *captures);

static struct fsm *
build_capstring_dfa(const struct capstring *cs, uint8_t end_id);

struct check_env {
	struct fsm *combined;
	struct fsm **copies;
	const struct fsm_combined_base_pair *capture_bases;
	const unsigned *capture_counts;
	size_t copies_count;
	uint8_t letter_count;
	uint8_t string_maxlen;
	size_t checks;
	bool check_others_for_fps;
};

struct css_input {
	const char *string;
	size_t pos;
};

static int
css_getc(void *opaque);

static enum theft_trial_res
check_fsms_for_single_input(struct check_env *env, struct fsm_capture *captures,
    const char *input_str);

static enum theft_trial_res
check_fsms_for_inputs(struct check_env *env, struct fsm_capture *captures);

static enum theft_trial_res
check_iter(struct check_env *env,
    char *buf, size_t offset, struct fsm_capture *captures);

static bool
compare_captures(const struct check_env *env,
    const struct fsm_capture *captures_combined,
    size_t nth_fsm, const struct fsm_capture *captures);

static bool
compare_endids(const struct check_env *env, fsm_state_t combined_end,
    size_t nth_fsm, fsm_state_t nth_end);

static bool
check_captures_for_false_positives(const struct check_env *env,
    const struct fsm_capture *captures_combined, size_t nth_fsm);

static bool
test_css_invariants(theft_seed seed)
{
	enum theft_run_res res;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 8,
	};


	seed = theft_seed_of_time();
	/* seed = 0x2eb4e4ca6df59761ULL; */

	fprintf(stderr, "==== seed %lu\n", seed);

	/* theft_generate(stdout, seed, &type_info_capture_string_set, &env); */
	/* exit(EXIT_SUCCESS); */

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_css_invariants,
		.type_info = { &type_info_capture_string_set, },
		.hooks = {
			.trial_pre = theft_hook_first_fail_halt,
			/* .trial_post = repeat_with_verbose, */
			.env = &env,
		},
		.seed = seed,
		.trials = 100000,
		.fork = {
			.enable = true,
			.timeout = 10000,
		},
	};

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));

	return res == THEFT_RUN_PASS;
}

static enum theft_trial_res
prop_css_invariants(struct theft *t, void *arg1)
{
	/* build DFAs for every string (alphanumeric literals or '*'),
	 * save match groups, then combine copies of them and check that
	 * the combined version behaves the same as matching each in
	 * sequence for all possible strings letter_count x string_maxlen.
	 *
	 * optionally skip them all if the combined one doesn't match;
	 * that mostly checks fsm_union, not captures. */
	struct capture_env *env = theft_hook_get_env(t);
	assert(env && env->tag == 'c');

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	const struct capture_string_set *css = arg1;
	return check_capstring_set(env, css, NULL, false,
	    capture_bases, captures);
}

static enum theft_trial_res
check_capstring_set(struct capture_env *env,
    const struct capture_string_set *css,
    const char *input, bool check_others_for_fps,
    struct fsm_combined_base_pair *capture_bases,
    struct fsm_capture *captures)
{
	const int verbosity = LOG_LEVEL;

	if (css->count == 0) {
		return THEFT_TRIAL_SKIP;
	}

	unsigned capture_counts[MAX_CAPTURE_STRINGS] = { 0 };
	struct fsm *fsm_copies[MAX_CAPTURE_STRINGS] = { NULL };

	struct fsm *fsms[MAX_CAPTURE_STRINGS] = { NULL };
	struct fsm *combined = NULL;
	size_t combined_capture_count, total_captures = 0;

	assert(capture_bases != NULL);

	for (size_t cs_i = 0; cs_i < css->count; cs_i++) {
		const struct capstring *cs = &css->capture_strings[cs_i];
		struct fsm *dfa = build_capstring_dfa(cs, (uint8_t)cs_i);

		if (dfa == NULL) {
			return THEFT_TRIAL_ERROR;
		}

		const size_t capture_count = fsm_capture_ceiling(dfa);

		if (verbosity > 2) {
			fprintf(stderr, "==== cs '%s'\n", cs->string);
			fsm_print_fsm(stderr, dfa);
		}

		struct fsm *cp = fsm_clone(dfa);
		if (cp == NULL) {
			return THEFT_TRIAL_ERROR;
		}
		assert(cp != NULL);
		fsm_copies[cs_i] = cp;

		const size_t cp_capture_count = fsm_capture_ceiling(cp);
		if (verbosity > 2) {
			fprintf(stderr, "==== min(det(cp))\n");
			fsm_print_fsm(stderr, cp);
			fsm_capture_dump(stderr, "min(det(cp))", cp);
			fprintf(stderr, "capture_count: %lu\n", cp_capture_count);
		}

		if (cp_capture_count != capture_count) {
			fprintf(stderr, "expected %lu but got %lu captures for cp\n",
			    capture_count, cp_capture_count);
			return THEFT_TRIAL_FAIL;
		}

		fsms[cs_i] = dfa;
		capture_counts[cs_i] = capture_count;
	}

	combined = fsm_union_array(css->count, fsms, capture_bases);
	if (combined == NULL) {
		fprintf(stderr, "FAIL: combined is NULL\n");
		return THEFT_TRIAL_FAIL;
	}

	combined_capture_count = fsm_capture_ceiling(combined);
	for (size_t cs_i = 0; cs_i < css->count; cs_i++) {
		total_captures += capture_counts[cs_i];
	}
	if (total_captures != combined_capture_count) {
		fprintf(stderr, "FAIL: total_captures %zu, combined_cc %zu\n",
		    total_captures, combined_capture_count);
		return THEFT_TRIAL_FAIL;
	}


	if (verbosity > 2) {
		fprintf(stderr, "==== combined, pre-det\n");
		fsm_print_fsm(stderr, combined);
		fsm_capture_dump(stderr, "combined, pre-det", combined);
	}

	if (!fsm_determinise(combined)) {
		return THEFT_TRIAL_ERROR;
	}

	/* should be a no-op */
	if (!fsm_complement(combined)) {
		return THEFT_TRIAL_ERROR;
	}
	if (!fsm_complement(combined)) {
		return THEFT_TRIAL_ERROR;
	}

	if (verbosity > 2) {
		fprintf(stderr, "==== det(combined)\n");
		fsm_print_fsm(stderr, combined);
		fsm_capture_dump(stderr, "det(combined)", combined);
	}

	struct check_env check_env = {
		.combined = combined,
		.copies = fsm_copies,
		.copies_count = css->count,
		.capture_bases = capture_bases,
		.capture_counts = capture_counts,
		.letter_count = env->letter_count,
		.string_maxlen = env->string_maxlen,
		.check_others_for_fps = check_others_for_fps,
	};

	enum theft_trial_res res;

	if (input == NULL) {
		res = check_fsms_for_inputs(&check_env, captures);
		/* fprintf(stderr, "-- checked %zu\n", check_env.checks); */
	} else {
		if (verbosity > 2) {
			fprintf(stderr, "===== checking for single input string '%s'\n", input);
		}
		res = check_fsms_for_single_input(&check_env, captures, input);
	}

	for (size_t cs_i = 0; cs_i < css->count; cs_i++) {
		if (fsm_copies[cs_i] != NULL) {
			fsm_free(fsm_copies[cs_i]);
		}
	}
	fsm_free(combined);

	return res;
}

static enum theft_trial_res
check_fsms_for_single_input(struct check_env *env, struct fsm_capture *captures,
	const char *input_str)
{
	/* check it: if the combined one matches, then
	 * at least one of the original strings must match,
	 * and then check the captures. */
	struct css_input input = { .string = input_str, };
	fsm_state_t end;
	assert(env->combined);
	assert(captures != NULL);

	for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
		captures[i].pos[0] = INIT_CAPTURES;
		captures[i].pos[1] = INIT_CAPTURES;
	}

	if (LOG_LEVEL > 1) {
		fprintf(stderr, "=== executing env->combined\n");
	}

	int exec_res = fsm_exec(env->combined, css_getc,
	    &input, &end, captures);
	if (exec_res < 0) {
		fprintf(stderr, "!!! '%s', res %d\n",
		    input_str, exec_res);
	}

	assert(exec_res >= 0);
	if (exec_res == 1) {
		if (LOG_LEVEL > 0) {
			const size_t combined_capture_count = fsm_capture_ceiling(env->combined);
			for (size_t i = 0; i < combined_capture_count; i++) {
				fprintf(stderr, "capture[%zu/%zu]: (%ld, %ld)\n",
				    i, combined_capture_count,
				    captures[i].pos[0], captures[i].pos[1]);
			}
		}

		bool found = false;
		for (size_t i = 0; i < env->copies_count; i++) {
			struct fsm_capture captures2[MAX_TOTAL_CAPTURES];
			for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
				captures2[i].pos[0] = INIT_CAPTURES;
				captures2[i].pos[1] = INIT_CAPTURES;
			}

			struct css_input input2 = { .string = input_str, };
			fsm_state_t end2;
			if (env->copies[i] == NULL) {
				fprintf(stderr, "env->copies[%zu]: null\n", i);
			}

			if (LOG_LEVEL > 1) {
				fprintf(stderr, "=== executing env->copies[%lu]\n", i);
			}

			assert(env->copies[i] != NULL);
			int exec_res2 = fsm_exec(env->copies[i],
			    css_getc, &input2, &end2, captures2);
			if (exec_res2 == 1) {
				found = true;
				if (!compare_captures(env, captures,
					i, captures2)) {
					return THEFT_TRIAL_FAIL;
				}

				if (!compare_endids(env, end, i, end2)) {
					return THEFT_TRIAL_FAIL;
				}
			} else if (exec_res2 == 0 && env->check_others_for_fps) {
				if (!check_captures_for_false_positives(env,
					captures, i)) {
					return THEFT_TRIAL_FAIL;
				}
			}
		}

		if (!found) {
			fprintf(stderr, "combined matched but not originals\n");
			return THEFT_TRIAL_FAIL;
		}
	}
	return THEFT_TRIAL_PASS;
}

static enum theft_trial_res
check_fsms_for_inputs(struct check_env *env, struct fsm_capture *captures)
{
	char buf[MAX_CAPTURE_STRING_LENGTH] = { 0 };

	/* exhaustively check inputs up to env->string_maxlen */
	return check_iter(env, buf, 0, captures);
}

static int
css_getc(void *opaque)
{
	struct css_input *input = opaque;
	int res = input->string[input->pos];
	input->pos++;
	return res == 0 ? EOF : res;
}

static enum theft_trial_res
check_iter(struct check_env *env,
    char *buf, size_t offset, struct fsm_capture *captures)
{
	const int verbosity = LOG_LEVEL;

	if (verbosity > 2) {
		fprintf(stderr, "buf -- [%zu]: '%s'\n",
		    offset, buf);
	}

	if (offset == env->string_maxlen) {
		return THEFT_TRIAL_PASS;
	} else {

		if (verbosity > 1) {
			fprintf(stderr, "buf -- [%zu]: '%s' (executing)\n",
			    offset, buf);
		}

		enum theft_trial_res res;
		res = check_fsms_for_single_input(env, captures, buf);
		if (res != THEFT_TRIAL_PASS) {
			return res;
		}
	}

	env->checks++;

	for (uint8_t i = 0; i < env->letter_count; i++) {
		buf[offset] = 'a' + i;
		buf[offset + 1] = '\0';
		enum theft_trial_res res = check_iter(env,
		    buf, offset + 1, captures);
		if (res != THEFT_TRIAL_PASS) {
			return res;
		}
	}

	return THEFT_TRIAL_PASS;
}

static bool
compare_captures(const struct check_env *env,
    const struct fsm_capture *captures_combined,
    size_t nth_fsm, const struct fsm_capture *captures)
{
	const size_t combined_capture_count = fsm_capture_ceiling(env->combined);
	if (combined_capture_count == 0) {
		return true;	/* no captures */
	}

	const size_t capture_count = env->capture_counts[nth_fsm];
	const size_t base = env->capture_bases[nth_fsm].capture;

	if (combined_capture_count < capture_count) {
		fprintf(stderr, "expected %lu captures, got %lu\n",
		    capture_count, combined_capture_count);
		return false;
	}

#if LOG_LEVEL > 1
	fprintf(stderr, "compare_captures: combined_capture_count %zu, capture_count %lu\n",
	    combined_capture_count, capture_count);

	for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
		fprintf(stderr, "captures[%zu]: (%ld, %ld)\n",
		    i, captures[i].pos[0], captures[i].pos[1]);
		if (captures[i].pos[0] == (size_t)INIT_CAPTURES) { break; }
	}

	for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
		fprintf(stderr, "captures_combined[%zu]: (%ld, %ld)\n",
		    i, captures_combined[i].pos[0], captures_combined[i].pos[1]);
		if (captures_combined[i].pos[0] == INIT_CAPTURES) { break; }
	}
#endif

	for (size_t i = 0; i < capture_count; i++) {
		if (LOG_LEVEL > 0) {
			fprintf(stderr, "cc[%zu/%zu]: (%ld, %ld) <-> (%ld, %ld) (base %lu)\n",
			    i, capture_count,
			    captures[i].pos[0], captures[i].pos[1],
			    captures_combined[i + base].pos[0],
			    captures_combined[i + base].pos[1], base);
		}

		for (size_t p = 0; p < 2; p++) {
			/* fprintf(stderr, "?? captures[%zu].pos[%lu] = %zu\n", i, p, captures[i].pos[p]); */
			/* fprintf(stderr, "?? captures_combined[%zu + %lu].pos[%lu] = %zu\n", i, base, p, captures_combined[i + base].pos[p]); */

			if (captures[i].pos[p] != captures_combined[i + base].pos[p]) {
				fprintf(stderr, "bad capture %zu: exp (%ld - %ld), got (%ld - %ld)\n",
				    i,
				    captures[i].pos[0], captures[i].pos[1],
				    captures_combined[i + base].pos[0],
				    captures_combined[i + base].pos[1]);

				return false;

				/* if (captures[i].pos[0] != FSM_CAPTURE_NO_POS */
				/*     && captures[i].pos[1] != FSM_CAPTURE_NO_POS) { */
				/* 	return false; */
				/* } */
			}

			if (0) {
				fprintf(stderr, "matched capture %zu: exp (%ld - %ld), got (%ld - %ld)\n",
				    i,
				    captures[i].pos[0], captures[i].pos[1],
				    captures_combined[i + base].pos[0],
				    captures_combined[i + base].pos[1]);
			}
		}
	}

	if (combined_capture_count == 0) {
		fprintf(stderr, "Zero captures...\n");
	}

	if (LOG_LEVEL > 0) {
		fprintf(stderr, "%zu captures...matched\n", combined_capture_count);
	}
	return true;
}

static bool
check_captures_for_false_positives(const struct check_env *env,
    const struct fsm_capture *captures_combined, size_t nth_fsm)
{
	(void)env;
	(void)captures_combined;
	(void)nth_fsm;

	const size_t capture_count = env->capture_counts[nth_fsm];
	const size_t base = env->capture_bases[nth_fsm].capture;

	for (size_t i = 0; i < capture_count; i++) {
		const struct fsm_capture *c = &captures_combined[i + base];
		if (c->pos[0] != FSM_CAPTURE_NO_POS) {
			fprintf(stderr, "false positive[0], expected %ld got %ld\n",
			    FSM_CAPTURE_NO_POS, c->pos[0]);
			return false;
		}
		if (c->pos[1] != FSM_CAPTURE_NO_POS) {
			fprintf(stderr, "false positive[1], expected %ld got %ld\n",
			    FSM_CAPTURE_NO_POS, c->pos[1]);
			return false;
		}
	}

	return true;
}

/* If the nth FSM matches, then that FSM's ending state's end ID should
 * be among the end IDs from the combined FSM's ending state. */
static bool
compare_endids(const struct check_env *env, fsm_state_t combined_end,
    size_t nth_fsm, fsm_state_t nth_end)
{
	fsm_end_id_t combined_end_buf[MAX_CAPTURE_STRINGS];
	size_t combined_written;
	fsm_end_id_t nth_end_buf[4];
	size_t nth_written;
	size_t i;
	enum fsm_getendids_res gres;

	gres = fsm_getendids(env->combined, combined_end,
	    sizeof(combined_end_buf)/sizeof(combined_end_buf[0]),
	    combined_end_buf, &combined_written);
	if (gres != FSM_GETENDIDS_FOUND) {
		fprintf(stderr, "FAIL: fsm_getendids failed for env->combined\n");
		return false;
	}

	gres = fsm_getendids(env->copies[nth_fsm], nth_end,
	    sizeof(nth_end_buf)/sizeof(nth_end_buf[0]),
	    nth_end_buf, &nth_written);
	if (gres != FSM_GETENDIDS_FOUND) {
		fprintf(stderr, "FAIL: fsm_getendids failed for env->nth[%zu]\n", nth_fsm);
		return false;
	}

	if (nth_written != 1) {
		fprintf(stderr, "FAIL: fsm_getendids returned %zu for nth_written, expected exactly 1\n",
		    nth_written);
		return false;
	}

	for (i = 0; i < combined_written; i++) {
		if (combined_end_buf[i] == nth_end_buf[0]) {
			return true; /* found! */
		}
	}

	fprintf(stderr, "FAIL: endid %u for nth_fsm %zu not found among env->combined's %zu end IDs\n",
	    nth_end_buf[0], nth_fsm, combined_written);
	return false;
}

static struct fsm_options options;

static struct fsm *
build_capstring_dfa(const struct capstring *cs, uint8_t end_id)
{
	struct fsm *fsm = NULL;
	size_t i;
	const size_t length = strlen(cs->string);
	size_t states = 0;
	fsm_state_t state, prev;

	fsm = fsm_new(&options);
	if (fsm == NULL) { goto cleanup; }

	if (!fsm_addstate(fsm, &state)) {
		goto cleanup;
	}
	fsm_setstart(fsm, state);
	prev = state;

	i = 0;
	while (i < length) {
		const char c = cs->string[i];
		assert(c != '*');

		if (!fsm_addstate(fsm, &state)) {
			goto cleanup;
		}
		states++;

		if (!fsm_addedge_literal(fsm, prev, state, c)) {
			goto cleanup;
		}

		if (i + 1 < length && cs->string[i + 1] == '*') {
			if (!fsm_addedge_epsilon(fsm, prev, state)) {
				goto cleanup;
			}
			if (!fsm_addedge_literal(fsm, state, state, c)) {
				goto cleanup;
			}
			i++;
		}

		prev = state;
		i++;
	}

	for (i = 0; i < cs->capture_count; i++) {
		const fsm_state_t start = cs->captures[i].offset;
		const fsm_state_t end = start + cs->captures[i].length;

		if (!fsm_capture_set_path(fsm, i, start, end)) {
			goto cleanup;
		}
	}

	fsm_setend(fsm, state, 1);

	if (!fsm_setendid(fsm, end_id)) {
		goto cleanup;
	}

	if (!fsm_determinise(fsm)) {
		goto cleanup;
	}

	if (!fsm_minimise(fsm)) {
		goto cleanup;
	}

	if (fsm_capture_ceiling(fsm) != cs->capture_count) {
		goto cleanup;
	}

	return fsm;

cleanup:
	fsm_free(fsm);
	return NULL;
}

/* Get capture POS for the NTH capture for the DFA with BASE_ID in the
 * original input struct; this compensates for merging being able to
 * reorder the original input DFAs.
 *
 * Note that checking this only makes sense when check_capstring_set is
 * called with a fixed input string. */
#define CAP(BASE_ID, NTH, POS)					\
	(captures[capture_bases[BASE_ID].capture + NTH].pos[POS])

static bool
regression_fp1(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "abc",
				.capture_count = 2,
				.captures = {
					{ 0, 3 },
					{ 3, 0 },
				},
			},
			{
				.string = "ab*c",
				.capture_count = 2,
				.captures = {
					{ 0, 3 },
					{ 3, 0 },
				},
			},
		},
	};

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, NULL, false,
		capture_bases, captures)) {
		return false;
	}
	return true;
}

static bool
regression_fp2(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	/* Setting this to true will cause it to fail, because
	 * "aaa" doesn't match the second DFA, but it currently
	 * still sets its capture group.
	 *
	 * Once information mapping end states to capture groups
	 * is available, fsm_exec can use it to clear any false
	 * positives. */
	const bool check_others = false;

	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "aaa",
				.capture_count = 0,
			},
			{
				.string = "aa",
				.capture_count = 1,
				.captures = {
					{ 0, 1 },
				},
			},
		},
	};

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, "aaa", check_others,
		capture_bases, captures)) {
		return false;
	}

	assert(CAP(1, 0, 0) == 0);
	assert(CAP(1, 0, 1) == 1);
	return true;
}

static bool
regression_n_1(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{	/* a*(b) */
				.string = "a*b",
				.capture_count = 1,
				.captures = {
					{ 1, 1 },
				},
			},
			{	/* (a)b* */
				.string = "ab*",
				.capture_count = 1,
				.captures = {
					{ 0, 1 },
				},
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, "ab", false,
		capture_bases, captures)) {
		return false;
	}

	assert(CAP(0, 0, 0) == 1);
	assert(CAP(0, 0, 1) == 2);
	assert(CAP(1, 0, 0) == 0);
	assert(CAP(1, 0, 1) == 2);
	return true;
}

static bool
regression_n_2(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 4,
		.capture_strings = {
			{
				.string = "a",
				.capture_count = 0,
			},
			{
				.string = "a*ab*ab",
				.capture_count = 1,
				.captures = {
					{ 0, 4 },
				},
			},
			{
				.string = "bb",
				.capture_count = 0,
			},
			{
				.string = "ba",
				.capture_count = 0,
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, NULL, false,
		capture_bases, captures)) {
		return false;
	}
	return true;
}

static bool
regression_n_3(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 1,
		.capture_strings = {
			{
				.string = "a*b*abb",
				.capture_count = 1,
				.captures = {
					{ 0, 3 },
				},
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, "aabb", false,
		capture_bases, captures)) {
		return false;
	}

	assert(CAP(0, 0, 0) == 0);
	assert(CAP(0, 0, 1) == 2);
	return true;

}

static bool
regression_n_4(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	/* Seed 0xe43c1e1972671223 */
	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "aa",
				.capture_count = 0,
			},
			{	/* (a*b*a)bb */
				.string = "a*b*abb",
				.capture_count = 1,
				.captures = {
					{ 0, 3 },
				},
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, "aabb", false,
		capture_bases, captures)) {
		return false;
	}

	assert(CAP(1, 0, 0) == 0);
	assert(CAP(1, 0, 1) == 2);
	return true;
}

static bool
regression_n_5(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	/* Seed 0x806c3f5579183bc8 */
	const struct capture_string_set css = {
		.count = 1,
		.capture_strings = {
			{	/* (a*b*a)(a)b */
				.string = "a*b*aab",
				.capture_count = 2,
				.captures = {
					{ 0, 3 },
					{ 3, 1 },
				},
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, "aaab", false,
		capture_bases, captures)) {
		return false;
	}

	assert(CAP(0, 0, 0) == 0);
	assert(CAP(0, 0, 1) == 3);
	assert(CAP(0, 1, 0) == 1);
	assert(CAP(0, 1, 1) == 3);
	return true;
}

static bool
regression_n_6(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	/* Seed 0xd54b2c439441e9d6 */
	const struct capture_string_set css = {
		.count = 1,
		.capture_strings = {
			{
				.string = "a*b*a*",
				.capture_count = 0,
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, NULL, false,
		capture_bases, captures)) {
		return false;
	}

	return true;
}

static bool
regression_n_7(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 5,
	};

	/* Seed 0xd54b2c439441e9d6 */
	const struct capture_string_set css = {
		.count = 4,
		.capture_strings = {
			{
				.string = "",
				.capture_count = 0,
			},
			{
				.string = "a*",
				.capture_count = 0,
			},
			{
				.string = "",
				.capture_count = 0,
			},
			{
				.string = "",
				.capture_count = 0,
			},
		},
	};

	struct fsm_combined_base_pair capture_bases[MAX_CAPTURE_STRINGS];
	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, NULL, false,
		capture_bases, captures)) {
		return false;
	}

	return true;
}

void
register_test_capture_string_set(void)
{
	reg_test("capture_string_set_regression_false_positive", regression_fp1);
	reg_test("capture_string_set_regression_false_positive2", regression_fp2);

	reg_test("capture_string_set_regression1", regression_n_1);
	reg_test("capture_string_set_regression2", regression_n_2);
	reg_test("capture_string_set_regression3", regression_n_3);
	reg_test("capture_string_set_regression4", regression_n_4);
	reg_test("capture_string_set_regression5", regression_n_5);
	reg_test("capture_string_set_regression6", regression_n_6);
	reg_test("capture_string_set_regression7", regression_n_7);

	reg_test("capture_string_set_invariants",
	    test_css_invariants);
}
