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

enum run_mode {
	MODE_DEFAULT,
	MODE_SHUFFLE_MINIMISE,
	MODE_ALL_PRINT_FUNCTIONS,
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
