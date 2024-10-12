#include <sys/time.h>
#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/cost.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <re/re.h>

#include "../src/libfsm/minimise_test_oracle.h"

/* 10 seconds */
#define TIMEOUT_USEC (10ULL * 1000 * 1000)

static bool verbosity_checked = false;
static bool verbose = false;

#define LOG(...)					\
	do {						\
		if (verbose) {				\
			fprintf(stderr, __VA_ARGS__);	\
		}					\
	} while (0)					\

enum run_mode {
	MODE_DEFAULT,
	MODE_SHUFFLE_MINIMISE,
	MODE_ALL_PRINT_FUNCTIONS,
	MODE_EAGER_OUTPUT,
};


/* This stuff will already exist elsewhere once other branches are merged. */
#if 1
static void
time_get(struct timeval *tv)
{
	if (0 != gettimeofday(tv, NULL)) {
		assert(!"gettimeofday");
	}
}

static size_t
time_diff_usec(const struct timeval *pre, const struct timeval *post)
{
	return 1000000*(post->tv_sec - pre->tv_sec)
	    + (post->tv_usec - pre->tv_usec);
}

struct scanner {
	const uint8_t *str;
	size_t size;
	size_t offset;
};

static int
scanner_next(void *opaque)
{
	struct scanner *s;
	unsigned char c;

	s = opaque;

	if (s->offset == s->size) {
		return EOF;
	}

	c = s->str[s->offset];
	s->offset++;

	return (int) c;
}
#endif

static struct fsm *
build(const char *pattern)
{
	assert(pattern != NULL);

	struct timeval pre, post;
	size_t delta_usec, total_usec = 0;
	struct re_err err;
	struct fsm *fsm;
	const size_t length = strlen(pattern);

	struct scanner s = {
		.str    = (const uint8_t *)pattern,
		.size   = length,
		.offset = 0
	};

	time_get(&pre);
	fsm = re_comp(RE_PCRE, scanner_next, &s, NULL, RE_MULTI, &err);
	time_get(&post);
	delta_usec = time_diff_usec(&pre, &post);
	total_usec += delta_usec;

	if (fsm == NULL) {
		return NULL;
	}

	time_get(&pre);
	if (!fsm_determinise(fsm)) {
		return NULL;
	}
	time_get(&post);
	delta_usec = time_diff_usec(&pre, &post);
	total_usec += delta_usec;

	time_get(&pre);
	if (!fsm_minimise(fsm)) {
		return NULL;
	}
	time_get(&post);
	delta_usec = time_diff_usec(&pre, &post);
	total_usec += delta_usec;

	if (total_usec > TIMEOUT_USEC) {
#ifndef EXPENSIVE_CHECKS
		assert(!"timeout");
#else
		fprintf(stderr, "exiting zero due to timeout under EXPENSIVE_CHECKS\n");
		exit(0);
#endif
	}

	return fsm;
}

static int
codegen(const struct fsm *fsm, enum fsm_io io_mode)
{
	const struct fsm_options opt = {
		.io = io_mode,
	};

	FILE *dev_null = fopen("/dev/null", "w");
	assert(dev_null != NULL);
	fsm_print(dev_null, fsm, &opt, NULL, FSM_PRINT_C);
	fclose(dev_null);
	return 1;
}

static int
build_and_codegen(const char *pattern, enum fsm_io io_mode)
{
	struct fsm *fsm = build(pattern);
	if (fsm == NULL) {
		return EXIT_SUCCESS;
	}

	if (!codegen(fsm, io_mode)) {
		return EXIT_SUCCESS;
	}

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

#define DEF_MAX_SHUFFLE 10
#define DEF_MAX_MINIMISE_ORACLE_STATE_COUNT 1000

static int
shuffle_minimise(const char *pattern)
{
	assert(pattern != NULL);

	struct re_err err;
	struct fsm *fsm;
	const size_t length = strlen(pattern);

	struct scanner s = {
		.str    = (const uint8_t *)pattern,
		.size   = length,
		.offset = 0
	};

	fsm = re_comp(RE_PCRE, scanner_next, &s, NULL, RE_MULTI, &err);

	if (fsm == NULL) {
		/* ignore invalid regexp syntax, etc. */
		return EXIT_SUCCESS;
	}

	if (!fsm_determinise(fsm)) {
		return EXIT_FAILURE;
	}

	const long trim_res = fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE, NULL);
	if (trim_res == -1) {
		return EXIT_FAILURE;
	}

	const size_t det_state_count = fsm_countstates(fsm);
	const char *max_shuffle_str = getenv("MAX_SHUFFLE");
	const size_t max_shuffle = (max_shuffle_str == NULL
	    ? DEF_MAX_SHUFFLE : atoi(max_shuffle_str));
	const size_t shuffle_limit = det_state_count > max_shuffle
	    ? max_shuffle : det_state_count;

	/* This can become very slow when the state count is large. */
	const char *max_minimise_oracle_state_count_str = getenv("MAX_MINIMISE_ORACLE");
	const size_t max_minimise_oracle_state_count =
	    (max_minimise_oracle_state_count_str == NULL
		? DEF_MAX_MINIMISE_ORACLE_STATE_COUNT
		: atoi(max_minimise_oracle_state_count_str));
	if (det_state_count > max_minimise_oracle_state_count) {
		/* Too many states, skip. This can become VERY
		 * slow as the state count gets large. */
		fsm_free(fsm);
		return EXIT_SUCCESS;
	}

	struct fsm *oracle_min = fsm_minimise_test_oracle(fsm);
	if (oracle_min == NULL) {
		return EXIT_SUCCESS; /* ignore malloc failure */
	}

	const size_t expected_state_count = fsm_countstates(oracle_min);

	/* Repeatedly copy the fsm, shuffle it different ways,
	 * minimise, and check if the resulting state count matches
	 * the result from the test oracle. */
	for (unsigned s_i = 0; s_i < shuffle_limit; s_i++) {
		struct fsm *cp = fsm_clone(fsm);
		if (cp == NULL) {
			return EXIT_FAILURE;
		}

		const size_t cp_state_count_before = fsm_countstates(cp);
		if (!fsm_shuffle(cp, s_i)) {
			return EXIT_FAILURE;
		}
		const size_t cp_state_count = fsm_countstates(cp);
		assert(cp_state_count == cp_state_count_before);

		if (!fsm_minimise(cp)) {
			return EXIT_FAILURE;
		}
		const size_t cp_state_count_min = fsm_countstates(cp);

		if (cp_state_count_min != expected_state_count) {
			fprintf(stderr, "%s: failure for seed %u, regex '%s', exp %zu, got %zu\n",
			    __func__, s_i, pattern, expected_state_count, cp_state_count);

			fprintf(stderr, "== original input:\n");
			fsm_print(stderr, fsm, NULL, NULL, FSM_PRINT_FSM);


			fprintf(stderr, "== expected:\n");
			fsm_print(stderr, oracle_min, NULL, NULL, FSM_PRINT_FSM);

			fprintf(stderr, "== got:\n");
			fsm_print(stderr, cp, NULL, NULL, FSM_PRINT_FSM);

			fsm_free(cp);
			fsm_free(oracle_min);
			fsm_free(fsm);
			assert(!"non-minimal result");
		}

		fsm_free(cp);
	}

	fsm_free(oracle_min);
	fsm_free(fsm);
	return EXIT_SUCCESS;
}

static int
fuzz_all_print_functions(FILE *f, const char *pattern, bool det, bool min, const enum fsm_io io_mode)
{
	assert(pattern != NULL);

	struct re_err err;
	struct fsm *fsm;
	const size_t length = strlen(pattern);

	struct scanner s = {
		.str    = (const uint8_t *)pattern,
		.size   = length,
		.offset = 0
	};

	const struct fsm_options opt = {
		.io = io_mode,
	};

	fsm = re_comp(RE_PCRE, scanner_next, &s, NULL, RE_MULTI, &err);
	if (fsm == NULL) {
		/* ignore invalid regexp syntax, etc. */
		return EXIT_SUCCESS;
	}

	if (det) {
		if (!fsm_determinise(fsm)) {
			assert(!"det failure");
			return EXIT_FAILURE;
		}

		if (min) {
			if (!fsm_minimise(fsm)) {
				assert(!"min failure");
				return EXIT_FAILURE;
			}
		}
	}

	/* if errno isn't zero already, I want to know why */
	assert(errno == 0);

	/* see if this triggers any asserts */
	int r = 0;

	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_AMD64_ATT);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_AMD64_GO);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_AMD64_NASM);

	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_API);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_AWK);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_C);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_DOT);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_FSM);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_GO);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_IR);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_IRJSON);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_JSON);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_LLVM);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_RUST);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_SH);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_VMC);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_VMDOT);

	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_VMOPS_C);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_VMOPS_H);
	r |= fsm_print(f, fsm, &opt, NULL, FSM_PRINT_VMOPS_MAIN);

	assert(r == 0 || errno != 0);

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

#define MAX_PATTERNS 4
struct eager_output_cb_info {
	size_t used;
	fsm_output_id_t ids[MAX_PATTERNS];
};

static void
reset_eager_output_info(struct eager_output_cb_info *info)
{
	info->used = 0;
}

struct feo_env {
	bool ok;
	size_t pattern_count;
	size_t fsm_count;
	size_t max_match_count;
	size_t max_steps;

	char *patterns[MAX_PATTERNS];
	struct fsm *fsms[MAX_PATTERNS];
	struct fsm *combined;

	/* which pattern is being used for generation, (size_t)-1 for combined */
	size_t current_pattern;

	struct eager_output_cb_info outputs;
	struct eager_output_cb_info outputs_combined;
};

void
append_eager_output_cb(fsm_output_id_t id, void *opaque)
{
	struct eager_output_cb_info *info = (struct eager_output_cb_info *)opaque;

	for (size_t i = 0; i < info->used; i++) {
		if (info->ids[i] == id) {
			return;	/* already present */
		}
	}

	assert(info->used < MAX_PATTERNS);
	info->ids[info->used++] = id;
}

static enum fsm_generate_matches_cb_res
gen_combined_check_individual_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque);

static enum fsm_generate_matches_cb_res
gen_individual_check_combined_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque);

#define DEF_MAX_STEPS 100000
#define DEF_MAX_MATCH_COUNT 1000

/* This isn't part of the public interface, per se. */
void
fsm_eager_output_dump(FILE *f, const struct fsm *fsm);

static int
fuzz_eager_output(const uint8_t *data, size_t size)
{
	struct feo_env env = {
		.ok = true,
		.pattern_count = 0,
		.max_steps = DEF_MAX_STEPS,
		.max_match_count = DEF_MAX_MATCH_COUNT,
	};

	{
		const char *steps = getenv("STEPS");
		const char *matches = getenv("MATCHES");
		if (steps != NULL) {
			env.max_steps = strtoul(steps, NULL, 10);
			assert(env.max_steps > 0);
		}
		if (matches != NULL) {
			env.max_match_count = strtoul(matches, NULL, 10);
			assert(env.max_match_count > 0);
		}
	}

	int ret = 0;

	size_t max_pattern_length = 0;

	/* chop data into a series of patterns */
	{
		size_t prev = 0;
		size_t offset = 0;

		/* Patterns with lots of '.' can take a while to determinise.
		 * That slows down fuzzer coverage, but isn't interesting here. */
		size_t dots = 0;

		while (offset < size && env.pattern_count < MAX_PATTERNS) {
#define MAX_DOTS 4
			if (data[offset] == '.') { dots++; }

			if (data[offset] == '\0' || data[offset] == '\n' || offset == size - 1) {
				size_t len = offset - prev;

				if (dots > MAX_DOTS) {
					/* ignored */
					prev = offset;
				} else if (len > 0) {
					char *pattern = malloc(len + 1);
					assert(pattern != NULL);

					memcpy(pattern, &data[prev], len);
					if (len > 0 && pattern[len] == '\n') {
						len--; /* drop trailing newline */
					}
					pattern[len] = '\0';
                                        bool keep = true;

                                        if (len > 0) {
                                            for (size_t i = 0; i < len - 1; i++) {
                                                if (pattern[i] == '\\' && pattern[i + 1] == 'x') {
                                                    /* ignore unhandled parser errors from "\x", see #386 */
                                                    keep = false;
                                                }
                                            }
                                        }

                                        if (keep) {
                                            env.patterns[env.pattern_count++] = pattern;

                                            if (len > max_pattern_length) {
						max_pattern_length = len;
                                            }
                                        } else {
                                            free(pattern);
                                        }
					prev = offset;
					dots = 0;
				}
			}

			offset++;
		}
	}

	enum re_is_anchored_res anchorage[MAX_PATTERNS] = {0};

	/* for each pattern, attempt to compile to a DFA */
	for (size_t p_i = 0; p_i < env.pattern_count; p_i++) {
		const char *p = env.patterns[p_i];

		enum re_is_anchored_res a = re_is_anchored(RE_PCRE, fsm_sgetc, &p, 0, NULL);
		if (a == RE_IS_ANCHORED_ERROR) {
			continue; /* unsupported regex */
		}

		p = env.patterns[p_i];
		struct fsm *fsm = re_comp(RE_PCRE, fsm_sgetc, &p, NULL, 0, NULL);

		LOG("%s: pattern %zd: '%s' => %p\n", __func__, p_i, env.patterns[p_i], (void *)fsm);

		if (fsm == NULL) {
			continue; /* invalid regex */
		}

		const fsm_output_id_t endid = (fsm_output_id_t)p_i;
		ret = fsm_seteageroutputonends(fsm, endid);
		assert(ret == 1);

		if (verbose) {
			fprintf(stderr, "==== pattern %zd, pre det\n", p_i);
			fsm_dump(stderr, fsm);
			fsm_eager_output_dump(stderr, fsm);
			fprintf(stderr, "====\n");

			fsm_state_t c = fsm_countstates(fsm);
			for (fsm_state_t i = 0; i < c; i++) {
				fprintf(stderr, "-- %d: end? %d\n", i, fsm_isend(fsm, i));
			}
		}

		ret = fsm_determinise(fsm);
		assert(ret == 1);

		ret = fsm_minimise(fsm);
		assert(ret == 1);

		fsm_state_t start;
		if (!fsm_getstart(fsm, &start)) {
			fsm_free(fsm);
			continue;
		}

		if (verbose) {
			fprintf(stderr, "==== pattern %zd, post det\n", p_i);
			fsm_dump(stderr, fsm);
			fsm_eager_output_dump(stderr, fsm);
			fprintf(stderr, "====\n");

			fsm_state_t c = fsm_countstates(fsm);
			for (fsm_state_t i = 0; i < c; i++) {
				fprintf(stderr, "-- %d: end? %d\n", i, fsm_isend(fsm, i));
			}
		}

		fsm_eager_output_set_cb(fsm, append_eager_output_cb, &env.outputs);
		env.fsms[env.fsm_count++] = fsm;
	}

	/* don't bother checking combined behavior unless there's multiple DFAs */
	if (env.fsm_count < 2) { goto cleanup; }

	/* copy and combine fsms into one DFA */
	{
		size_t used = 0;
		struct fsm_union_entry entries[MAX_PATTERNS] = {0};

		for (size_t i = 0; i < env.fsm_count; i++) {
			/* there can be gaps, fsms[] lines up with patterns[] */
			if (env.fsms[i] == NULL) { continue; }

			fsm_state_t start;
			if (!fsm_getstart(env.fsms[i], &start)) {
				assert(!"hit");
			}

			struct fsm *cp = fsm_clone(env.fsms[i]);
			assert(cp != NULL);

			if (verbose) {
				fprintf(stderr, "==== cp %zd\n", i);
				fsm_dump(stderr, cp);
				fsm_eager_output_dump(stderr, cp);
				fprintf(stderr, "====\n");

				fsm_state_t c = fsm_countstates(cp);
				for (fsm_state_t i = 0; i < c; i++) {
					fprintf(stderr, "-- %d: end? %d\n", i, fsm_isend(cp, i));
				}
			}

			entries[used].fsm = cp;
			entries[used].anchored_start = anchorage[i] & RE_IS_ANCHORED_START;
			entries[used].anchored_end = anchorage[i] & RE_IS_ANCHORED_END;
			used++;
		}

		if (used == 0) {
			goto cleanup; /* nothing to do */
		}

		/* consumes entries[] */
		struct fsm *fsm = fsm_union_repeated_pattern_group(used, entries, NULL);
		assert(fsm != NULL);

		if (verbose) {
			fprintf(stderr, "==== combined (pre-det)\n");
			fsm_dump(stderr, fsm);
			fsm_eager_output_dump(stderr, fsm);
			fprintf(stderr, "====\n");
		}

		if (!fsm_determinise(fsm)) {
			assert(!"failed to determinise");
		}

		if (!fsm_minimise(fsm)) {
			assert(!"failed to minimise");
		}

		LOG("%s: combined state_count %d\n", __func__, fsm_countstates(fsm));
		env.combined = fsm;
		/* fsm_eager_output_set_cb(fsm, append_eager_output_cb, &env.outputs_combined); */

		if (verbose) {
			fprintf(stderr, "==== combined\n");
			fsm_dump(stderr, env.combined);
			fsm_eager_output_dump(stderr, env.combined);
			fprintf(stderr, "====\n");
		}

	}

	/* Use fsm_generate_matches to check for matches that got lost
	 * and false positives introduced while combining the DFAs.
	 * Use the combined DFA to generate matches, check that the
	 * match behavior agrees with the individual DFA copies. */
	env.current_pattern = (size_t)-1;
	if (!fsm_generate_matches(env.combined, max_pattern_length, gen_combined_check_individual_cb, &env)) {
		goto cleanup;
	}

	if (!env.ok) { goto cleanup; }

	/* Likewise, use every individual DFA to generate matches and */
	/* check behavior against the combined DFA. */
	for (size_t i = 0; i < env.pattern_count; i++) {
		env.current_pattern = i;
		if (!fsm_generate_matches(env.combined, max_pattern_length, gen_individual_check_combined_cb, &env)) {
			goto cleanup;
		}
	}

	ret = env.ok ? EXIT_SUCCESS : EXIT_FAILURE;
cleanup:
	for (size_t i = 0; i < MAX_PATTERNS; i++) {
		if (env.patterns[i] != NULL) {
			free(env.patterns[i]);
			env.patterns[i] = NULL;
		}
		if (env.fsms[i] != NULL) {
			fsm_free(env.fsms[i]);
		}
	}
	if (env.combined != NULL) {
		fsm_free(env.combined);
	}

	return ret;
}

static int
cmp_output_id(const void *pa, const void *pb)
{
	const fsm_output_id_t a = *(fsm_output_id_t *)pa;
	const fsm_output_id_t b = *(fsm_output_id_t *)pb;
	return a < b ? -1 : a > b ? 1 : 0;
}

static bool
match_input_get_eager_outputs(struct fsm *fsm, const char *input, size_t input_length,
    struct eager_output_cb_info *dst)
{
	(void)input_length;
	fsm_state_t end;

	reset_eager_output_info(dst);

	fsm_eager_output_set_cb(fsm, append_eager_output_cb, dst);
	const int ret = fsm_exec(fsm, fsm_sgetc, &input, &end, NULL);
	if (ret == 0) {
		return false; /* no match */
	} else {
		assert(ret == 1); /* match */
	}

	/* sort the IDs, to make comparison cheaper */
	qsort(dst->ids, dst->used, sizeof(dst->ids[0]), cmp_output_id);
	return true;	/* match */
}

/* For a given matching input generated by the combined DFA, check that
 * only the expected individual source DFAs match. */
static enum fsm_generate_matches_cb_res
gen_combined_check_individual_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	(void)fsm;
	(void)depth;
	(void)end_state;

	struct feo_env *env = opaque;
	assert(env->current_pattern == (size_t)-1);

	if (match_count > env->max_match_count) { return FSM_GENERATE_MATCHES_CB_RES_HALT; }
	if (steps > env->max_steps) { return FSM_GENERATE_MATCHES_CB_RES_HALT; }

	/* execute, to set eager outputs */
	if (!match_input_get_eager_outputs(env->combined, input, input_length, &env->outputs_combined)) {
		env->ok = false;
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	size_t individual_outputs_used = 0;
	fsm_output_id_t individual_outputs[MAX_PATTERNS];

	for (size_t i = 0; i < env->pattern_count; i++) {
		struct fsm *fsm = env->fsms[i];
		if (fsm == NULL) { continue; }

		if (!match_input_get_eager_outputs(fsm, input, input_length, &env->outputs)) {
			env->ok = false;
			return FSM_GENERATE_MATCHES_CB_RES_HALT;
		}

		if (env->outputs.used > 0) {
			assert(env->outputs.used == 1);
			individual_outputs[individual_outputs_used++] = env->outputs.ids[0];
		}
	}

	bool match = true;
	if (env->outputs_combined.used != individual_outputs_used) {
		match = false;
	}

	for (size_t cmb_i = 0; cmb_i < env->outputs_combined.used; cmb_i++) {
		const fsm_output_id_t cur = env->outputs_combined.ids[cmb_i];
		assert(env->fsms[cmb_i] != NULL);
		bool found = false;
		for (size_t i = 0; i < individual_outputs_used; i++) {
			if (individual_outputs[i] == cur) {
				found = true;
				break;
			}
		}
		if (!found) {
			match = false;
			break;
		}
	}

	if (!match) {
		fprintf(stderr, "%s: combined <-> individual mismatch for input '%s'(%zd)!\n", __func__, input, input_length);

		fprintf(stderr, "-- combined: %zu IDs:", env->outputs_combined.used);
		for (size_t cmb_i = 0; cmb_i < env->outputs_combined.used; cmb_i++) {
			fprintf(stderr, " %d", env->outputs_combined.ids[cmb_i]);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "-- individiual: %zu IDs:", individual_outputs_used);
		for (size_t i = 0; i < individual_outputs_used; i++) {
			fprintf(stderr, " %d", individual_outputs[i]);
		}
		fprintf(stderr, "\n");
		goto fail;
	}

	return FSM_GENERATE_MATCHES_CB_RES_CONTINUE;

fail:
	env->ok = false;
	return FSM_GENERATE_MATCHES_CB_RES_HALT;
}

/* For a given matching input generated by one of the source DFAs, check that
 * the combined DFA also matches, and that the only other source DFAs that match
 * are ones that should according to the combined DFA. */
static enum fsm_generate_matches_cb_res
gen_individual_check_combined_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	(void)fsm;
	(void)depth;
	(void)end_state;

	struct feo_env *env = opaque;
	assert(env->current_pattern < env->pattern_count);
	if (match_count > env->max_match_count) { return FSM_GENERATE_MATCHES_CB_RES_HALT; }
	if (steps > env->max_steps) { return FSM_GENERATE_MATCHES_CB_RES_HALT; }

	struct fsm *cur_fsm = env->fsms[env->current_pattern];
	if (cur_fsm == NULL) { return FSM_GENERATE_MATCHES_CB_RES_CONTINUE; }

	/* execute, to set eager outputs */
	if (!match_input_get_eager_outputs(cur_fsm, input, input_length, &env->outputs)) {
		goto fail;
	}
	if (!match_input_get_eager_outputs(env->combined, input, input_length, &env->outputs_combined)) {
		goto fail;
	}

	assert(env->outputs.used == 1);

	bool found = false;
	for (size_t i = 0; i < env->outputs_combined.used; i++) {
		if (env->outputs_combined.ids[i] == env->outputs.ids[0]) {
			found = true;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "%s: combined <-> individual mismatch for input '%s'(%zd)!\n", __func__, input, input_length);

		fprintf(stderr, "-- combined: %zu IDs:", env->outputs_combined.used);
		for (size_t cmb_i = 0; cmb_i < env->outputs_combined.used; cmb_i++) {
			fprintf(stderr, " %d", env->outputs_combined.ids[cmb_i]);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "-- pattern %zd: %zu IDs:", env->current_pattern, env->outputs.used);
		for (size_t i = 0; i < env->outputs.used; i++) {
			fprintf(stderr, " %d", env->outputs.ids[i]);
		}
		fprintf(stderr, "\n");
		goto fail;
	}

	return FSM_GENERATE_MATCHES_CB_RES_CONTINUE;

fail:
	env->ok = false;
	return FSM_GENERATE_MATCHES_CB_RES_HALT;
}
#undef MAX_PATTERNS

#define MAX_FUZZER_DATA (64 * 1024)
static uint8_t data_buf[MAX_FUZZER_DATA + 1];

static enum run_mode
get_run_mode(void)
{
	const char *mode = getenv("MODE");
	if (mode == NULL) {
		return MODE_DEFAULT;
	}

	switch (mode[0]) {
	case 'm': return MODE_SHUFFLE_MINIMISE;
	case 'p': return MODE_ALL_PRINT_FUNCTIONS;
	case 'E': return MODE_EAGER_OUTPUT;
	case 'd':
	default:
		return MODE_DEFAULT;
	}
}

static FILE *dev_null = NULL;

int
harness_fuzzer_target(const uint8_t *data, size_t size)
{
	if (size < 1) {
		return EXIT_SUCCESS;
	}

	if (!verbosity_checked) {
		verbosity_checked = true;
		verbose = getenv("VERBOSE") != NULL;
	}

	/* Ensure that input is '\0'-terminated. */
	if (size > MAX_FUZZER_DATA) {
		size = MAX_FUZZER_DATA;
	}
	memcpy(data_buf, data, size);

	const char *pattern = (const char *)data_buf;

	switch (get_run_mode()) {
	case MODE_DEFAULT: {
		const uint8_t b0 = data_buf[0];
		const enum fsm_io io_mode = (b0 >> 2) % 3;

		return build_and_codegen(pattern, io_mode);
	}

	case MODE_SHUFFLE_MINIMISE:
		return shuffle_minimise(pattern);

	case MODE_EAGER_OUTPUT:
		return fuzz_eager_output(data, size);

	case MODE_ALL_PRINT_FUNCTIONS:
	{
		if (dev_null == NULL) {
			dev_null = fopen("/dev/null", "w");
			assert(dev_null != NULL);
		}

		const uint8_t b0 = data_buf[0];
		const bool det = b0 & 0x1;
		const bool min = b0 & 0x2;
		const enum fsm_io io_mode = (b0 >> 2) % 3;

		const char *shifted_pattern = (const char *)&data_buf[1];
		int res = fuzz_all_print_functions(dev_null, shifted_pattern, det, min, io_mode);
		return res;
	}
	}

	assert(!"unreached");
}
