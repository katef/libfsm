#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/cost.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <re/re.h>

/* 10 seconds */
#define TIMEOUT_USEC (10ULL * 1000 * 1000)

enum run_mode {
	MODE_DEFAULT,
	MODE_SHUFFLE_MINIMISE,
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

static const struct fsm_options opt;

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
	fsm = re_comp(RE_PCRE, scanner_next, &s, &opt, RE_MULTI, &err);
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
		assert(!"timeout");
	}

	return fsm;
}

static int
codegen(const struct fsm *fsm)
{
	FILE *dev_null = fopen("/dev/null", "w");
	assert(dev_null != NULL);
	fsm_print_c(dev_null, fsm);
	fclose(dev_null);
	return 1;
}

static int
build_and_codegen(const char *pattern)
{
	struct fsm *fsm = build(pattern);
	if (fsm == NULL) {
		return EXIT_SUCCESS;
	}

	if (!codegen(fsm)) {
		return EXIT_SUCCESS;
	}

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

#define MAX_SHUFFLE 100

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

	fsm = re_comp(RE_PCRE, scanner_next, &s, &opt, RE_MULTI, &err);

	if (fsm == NULL) {
		/* ignore invalid regexp syntax, etc. */
		return EXIT_SUCCESS;
	}

	if (!fsm_determinise(fsm)) {
		return EXIT_FAILURE;
	}

	const size_t det_state_count = fsm_countstates(fsm);
	const size_t shuffle_limit = det_state_count > MAX_SHUFFLE
	    ? MAX_SHUFFLE : det_state_count;

	bool has_expected_state_count = false;
	size_t expected_state_count;

	/* Repeatedly copy the fsm, shuffle it different ways,
	 * minimise, and check if the resulting state count matches. */
	for (unsigned s_i = 0; s_i < shuffle_limit; s_i++) {
		struct fsm *cp = fsm_clone(fsm);
		if (cp == NULL) {
			return EXIT_FAILURE;
		}

		fsm_shuffle(cp, s_i);

		if (!fsm_minimise(cp)) {
			return EXIT_FAILURE;
		}

		const size_t cp_state_count = fsm_countstates(cp);
		if (has_expected_state_count) {
			if (cp_state_count != expected_state_count) {
				fprintf(stderr, "%s: failure for seed %u, regex '%s', exp %zu, got %zu\n",
				    __func__, s_i, pattern, expected_state_count, cp_state_count);
			}
		} else {
			expected_state_count = cp_state_count;
			has_expected_state_count = true;
		}

		fsm_free(cp);
	}

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

#define MAX_FUZZER_DATA (64 * 1024)
static uint8_t data_buf[MAX_FUZZER_DATA + 1];

static enum run_mode
get_run_mode(void)
{
	const char *mode = getenv("MODE");
	if (mode != NULL) {
		switch (mode[0]) {
		case 'm': return MODE_SHUFFLE_MINIMISE;
		default:
			break;
		}
	}

	return MODE_DEFAULT;
}

int
harness_fuzzer_target(const uint8_t *data, size_t size)
{
	enum run_mode run_mode = get_run_mode();

	/* Ensure that input is '\0'-terminated. */
	if (size > MAX_FUZZER_DATA) {
		size = MAX_FUZZER_DATA;
	}
	memcpy(data_buf, data, size);

	const char *pattern = (const char *)data_buf;

	switch (run_mode) {
	case MODE_DEFAULT:
		return build_and_codegen(pattern);
	case MODE_SHUFFLE_MINIMISE:
		return shuffle_minimise(pattern);
	}
}
