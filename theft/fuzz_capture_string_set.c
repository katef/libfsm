#include "type_info_capture_string_set.h"

#include <fsm/capture.h>
#include <fsm/print.h>

#define LOG_LEVEL 2

static enum theft_trial_res
prop_css_invariants(struct theft *t, void *arg1);

static enum theft_trial_res
check_capstring_set(struct capture_env *env,
    const struct capture_string_set *css,
    const char *input, struct fsm_capture *captures);

static struct fsm *
build_capstring_dfa(const struct capstring *cs, uint8_t end_id);

struct check_env {
	struct fsm *combined;
	struct fsm **copies;
	const size_t *capture_bases;
	const unsigned *capture_counts;
	size_t copies_count;
	uint8_t letter_count;
	uint8_t string_maxlen;
	size_t checks;
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
test_css_invariants(theft_seed seed)
{
	enum theft_run_res res;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 2,
		.string_maxlen = 7,
	};


	seed = theft_seed_of_time();
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
			/* .enable = true, */
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

	struct fsm_capture captures[MAX_TOTAL_CAPTURES];
	const struct capture_string_set *css = arg1;
	return check_capstring_set(env, css, NULL, captures);
}

static enum theft_trial_res
check_capstring_set(struct capture_env *env,
    const struct capture_string_set *css,
    const char *input, struct fsm_capture *captures)
{
	const int verbosity = LOG_LEVEL;

	if (css->count == 0) {
		return THEFT_TRIAL_SKIP;
	}

	size_t capture_bases[MAX_CAPTURE_STRINGS] = { 0 };
	unsigned capture_counts[MAX_CAPTURE_STRINGS] = { 0 };
	struct fsm *fsm_copies[MAX_CAPTURE_STRINGS] = { NULL };
	struct fsm *combined = NULL;

	for (size_t cs_i = 0; cs_i < css->count; cs_i++) {
		const struct capstring *cs = &css->capture_strings[cs_i];

		for (size_t i = 0; i < cs->capture_count; i++) {
			/* if (cs->captures[i].length == 0) { */
			/* 	return THEFT_TRIAL_SKIP; */
			/* } */
		}

		struct fsm *dfa = build_capstring_dfa(cs, (uint8_t)cs_i);

		if (dfa == NULL) {
			return THEFT_TRIAL_ERROR;
		}

		const size_t capture_count = fsm_countcaptures(dfa);

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

		const size_t cp_capture_count = fsm_countcaptures(cp);
		if (verbosity > 2) {
			fprintf(stderr, "==== min(det(cp))\n");
			fsm_print_fsm(stderr, cp);
			fprintf(stderr, "capture_count: %lu\n", cp_capture_count);
		}

		if (cp_capture_count != capture_count) {
			fprintf(stderr, "expected %lu but got %lu captures for cp\n",
			    capture_count, cp_capture_count);
			return THEFT_TRIAL_FAIL;
		}

		if (cs_i == 0) {
			combined = dfa;
			/* fsm_capture_dump(stderr, "pre-union", combined); */
			capture_counts[0] = capture_count;
		} else {
			struct fsm_combine_info ci;
			combined = fsm_union(combined, dfa, &ci);
			if (combined == NULL) {
				return THEFT_TRIAL_ERROR;
			}

			/* FIXME: the way we track capture bases is problematic
			 * here, because everything before may get shifted --
			 * if _a is > 0, we need to increment everything up to
			 * now by that, right? */
			if (LOG_LEVEL > 2) {
				fprintf(stderr, "union'd, bases %u and %u, cs_i %lu\n",
				    ci.capture_base_a,
				    ci.capture_base_b, cs_i);
			}

			if (cs_i == 1) {
				capture_bases[0] = ci.capture_base_a;
			}
			capture_bases[cs_i] = ci.capture_base_b;
			capture_counts[cs_i] = cp_capture_count;

			if (ci.capture_base_a > 0 && cs_i > 1) {
				size_t i;
				for (i = 0; i < cs_i; i++) {
					capture_bases[i] += ci.capture_base_a;

					if (LOG_LEVEL > 2) {
						fprintf(stderr, " -- *** capture_bases[%lu] now %lu\n", i, capture_bases[i]);
					}
				}
			}

			if (capture_counts[cs_i] < capture_count) {
				fprintf(stderr, "FAIL: combined has %u captures but dfa has %zu\n",
				    capture_counts[cs_i], capture_count);
				return THEFT_TRIAL_FAIL;
			}

			if (verbosity > 2) {
				fsm_capture_dump(stderr, "unioned", combined);
				fsm_print_fsm(stderr, combined);
			}

		}

		if (verbosity > 2) {
			fprintf(stderr, "==== combined with %zu:\n", cs_i);
			fsm_print_fsm(stderr, combined);
		}
	}

	if (verbosity > 2) {
		fsm_capture_dump(stderr, "pre-det", combined);
		fsm_print_fsm(stderr, combined);
	}

	if (!fsm_determinise(combined)) {
		return THEFT_TRIAL_ERROR;
	}

	/* (do not minimise) */
	/* if (!fsm_minimise(combined)) { */
	/* 	return THEFT_TRIAL_ERROR; */
	/* } */

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
	};

	enum theft_trial_res res;

	if (input == NULL) {
		res = check_fsms_for_inputs(&check_env, captures);
		/* fprintf(stderr, "-- checked %zu\n", check_env.checks); */
	} else {
		res = check_fsms_for_single_input(&check_env, captures, input);
	}

	for (size_t cs_i = 0; cs_i < css->count; cs_i++) {
		fsm_free(fsm_copies[cs_i]);
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

	for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
		captures[i].pos[0] = (size_t)-3;
		captures[i].pos[1] = (size_t)-3;
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
			const size_t combined_capture_count = fsm_countcaptures(env->combined);
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
				captures2[i].pos[0] = (size_t)-2;
				captures2[i].pos[1] = (size_t)-2;
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

#if 0
		/* check it: if the combined one matches, then
		 * at least one of the original strings must match,
		 * and then check the captures. */
		struct css_input input = { .string = buf, };
		fsm_state_t end;
		assert(env->combined);

		for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
			captures[i].pos[0] = (size_t)-3;
			captures[i].pos[1] = (size_t)-3;
		}

		int exec_res = fsm_exec(env->combined, css_getc,
		    &input, &end, captures);
		if (exec_res < 0) {
			fprintf(stderr, "!!! '%s', res %d\n",
			    buf, exec_res);
		}

		assert(exec_res >= 0);
		if (exec_res == 1) {
			if (LOG_LEVEL > 0) {
				const size_t combined_capture_count = fsm_countcaptures(env->combined);
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
					captures2[i].pos[0] = (size_t)-2;
					captures2[i].pos[1] = (size_t)-2;
				}

				struct css_input input2 = { .string = buf, };
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
				}
			}

			if (!found) {
				fprintf(stderr, "combined matched but not originals\n");
				return THEFT_TRIAL_FAIL;
			}
		}
#endif
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
	const size_t combined_capture_count = fsm_countcaptures(env->combined);
	if (combined_capture_count == 0) {
		return true;	/* no captures */
	}

	const size_t capture_count = env->capture_counts[nth_fsm];
	const size_t base = env->capture_bases[nth_fsm];

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
	}

	for (size_t i = 0; i < MAX_TOTAL_CAPTURES; i++) {
		fprintf(stderr, "captures_combined[%zu]: (%ld, %ld)\n",
		    i, captures_combined[i].pos[0], captures_combined[i].pos[1]);
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

static struct fsm_options options;

static void css_carryopaque(struct fsm *src_fsm,
    const fsm_state_t *src_set, size_t n,
    struct fsm *dst_fsm, fsm_state_t dst_state)
{
	struct css_end_opaque *eo_src = NULL;
	struct css_end_opaque *eo_dst = NULL;
	struct css_end_opaque *eo_old_dst = NULL;
	size_t i;

	eo_old_dst = fsm_getopaque(dst_fsm, dst_state);

	eo_dst = calloc(1, sizeof(*eo_dst));
	assert(eo_dst != NULL);
	eo_dst->tag = CSS_END_OPAQUE_TAG;

	if (eo_old_dst != NULL) {
		assert(eo_old_dst->tag == CSS_END_OPAQUE_TAG);
		eo_dst->ends |= eo_old_dst->ends;
		/* FIXME: freeing here leads to a use after free */
		/* f_free(opt->alloc, eo_old_dst); */
	}

	fsm_setopaque(dst_fsm, dst_state, eo_dst);

	/* FIXME: set captures */

	/* union bits set in eo_src->ends into eo_dst->ends and free */
	for (i = 0; i < n; i++) {
		if (!fsm_isend(src_fsm, src_set[i])) {
			continue;
		}

		eo_src = fsm_getopaque(src_fsm, src_set[i]);
		if (eo_src == NULL) {
			continue;
		}
		assert(eo_src->tag == CSS_END_OPAQUE_TAG);
		eo_dst->ends |= eo_src->ends;

		/* FIXME: freeing here leads to a use after free */
		/* f_free(opt->alloc, eo_src); */

		fsm_setopaque(src_fsm, src_set[i], NULL);
	}
}

static struct fsm *
build_capstring_dfa(const struct capstring *cs, uint8_t end_id)
{
	struct fsm *fsm = NULL;
	size_t i;
	struct css_end_opaque *eo = NULL;
	const size_t length = strlen(cs->string);
	size_t states = 0;
	fsm_state_t state, prev;

	if (options.carryopaque == NULL) { /* initialize */
		options.carryopaque = css_carryopaque;
	}

	fsm = fsm_new(&options);
	if (fsm == NULL) { goto cleanup; }

	eo = calloc(1, sizeof(*eo));
	assert(eo != NULL);

	eo->tag = CSS_END_OPAQUE_TAG;

	/* set bit for end state */
	assert(end_id < 8*sizeof(eo->ends));
	eo->ends |= (1U << end_id);

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
	fsm_setopaque(fsm, state, eo);

	if (!fsm_determinise(fsm)) {
		goto cleanup;
	}

	if (!fsm_minimise(fsm)) {
		goto cleanup;
	}

	if (fsm_countcaptures(fsm) != cs->capture_count) {
		goto cleanup;
	}

	return fsm;

cleanup:
	free(eo);
	fsm_free(fsm);
	return NULL;
}

static bool
regression0(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 3,
		.capture_strings = {
			{
				.string = "",
				.capture_count = 0,
			},
			{
				.string = "",
				.capture_count = 0,
			},
			{
				.string = "aaa",
				.capture_count = 0,
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression1(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 1,
		.capture_strings = {
			{
				.string = "a",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression2(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	/* enough actions to force hash table growth */
	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "bbbbb",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
			{
				.string = "b*bba",
				.capture_count = 2,
				.captures = {
					{ 0, 2 },
					{ 1, 3 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression3(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "aaaaaaa",
				.capture_count = 2,
				.captures = {
					{ 0, 2 },
					{ 0, 0 },
				},
			},
			{
				.string = "d*a*bbd",
				.capture_count = 2,
				.captures = {
					{ 0, 0 },
					{ 0, 2 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression4(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	/* enough actions to force hash table growth */
	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "aa",
				.capture_count = 0,
			},
			{
				.string = "",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression5(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	/* enough actions to force hash table growth */
	const struct capture_string_set css = {
		.count = 3,
		.capture_strings = {
			{
				.string = "",
				.capture_count = 0,
			},
			{
				.string = "",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
			{
				.string = "aaaa",
				.capture_count = 1,
				.captures = {
					{ 0, 2 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression_fp(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

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

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression6(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	/* enough actions to force hash table growth */
	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{
				.string = "",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
			{
				.string = "a",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression7(theft_seed seed)
{
	(void)seed;

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

	const struct capture_string_set css = {
		.count = 3,
		.capture_strings = {
			{
				.string = "bb",
				.capture_count = 0,
			},
			{
				.string = "a",
				.capture_count = 0,
			},
			{
				.string = "b*a",
				.capture_count = 2,
				.captures = {
					{ 0, 0 },
					{ 0, 1 },
				},
			},
		},
	};

	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);
}

static bool
regression8(theft_seed seed)
{
	(void)seed; // 0x5d58a83011ff1d39

	struct capture_env env = {
		.tag = 'c',
		.letter_count = 3,
		.string_maxlen = 5,
	};

#if 0
	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{	/* "()aa" */
				.string = "aa",
				.capture_count = 1,
				.captures = {
					{ 0, 0 },
				},
			},
			{	/* "()a*(a)a()a" */
				.string = "a*aaa*",
				.capture_count = 3,
				.captures = {
					{ 0, 0 },
					{ 3, 0 },
					{ 1, 1 },
				},
			},
		},
	};
#else

	/* echo "bananas" | pcregrep --om-separator="#" -o1 -o2 ".*(an).*(ana).*" */
	/* echo "aa" | pcregrep --om-separator="#" -o1 -o2 "a*(a)()" */
	/* -> [(0,1); (-1,-1)] */

	const struct capture_string_set css = {
		.count = 2,
		.capture_strings = {
			{	/* "aa" */
				.string = "aa",
				.capture_count = 0,
			},
			{	/* "a*(a)()" */
				.string = "a*a",
				.capture_count = 2,
				.captures = {
					{ 1, 1 },
					{ 2, 0 },
				},
			},
		},
	};
#endif
	return THEFT_TRIAL_PASS ==
	    check_capstring_set(&env, &css, NULL, NULL);

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

	struct fsm_capture captures[MAX_TOTAL_CAPTURES];

	if (THEFT_TRIAL_PASS !=
	    check_capstring_set(&env, &css, "ab", captures)) {
		return false;
	}

	/* FIXME: check captures, need base info */
	fprintf(stderr, "%ld, %ld, %ld, %ld\n",
	    captures[0].pos[0], captures[0].pos[1],
	    captures[1].pos[0], captures[1].pos[1]);
	/* assert(captures[0].pos[0] ==  */

	return true;
}

void
register_test_capture_string_set(void)
{
	reg_test("capture_string_set_regression_false_positive", regression_fp);
	reg_test("capture_string_set_regression0", regression0);
	reg_test("capture_string_set_regression1", regression1);
	reg_test("capture_string_set_regression2", regression2);
	reg_test("capture_string_set_regression3", regression3);
	reg_test("capture_string_set_regression4", regression4);
	reg_test("capture_string_set_regression5", regression5);
	reg_test("capture_string_set_regression6", regression6);
	reg_test("capture_string_set_regression7", regression7);
	reg_test("capture_string_set_regression8", regression8);

	reg_test("capture_string_set_nonempty_regression1", regression_n_1);


	reg_test("capture_string_set_invariants",
	    test_css_invariants);
}
