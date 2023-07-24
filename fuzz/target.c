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
#include <fsm/alloc.h>
#include <fsm/bool.h>
#include <fsm/cost.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <re/re.h>

#include "../src/libfsm/minimise_test_oracle.h"

/* for fsm_capture_dump */
/* FIXME: should this be a public interface? */
#include "../src/libfsm/capture.h"

/* Buffer for sanitized fuzzer input */
#define MAX_FUZZER_DATA (64 * 1024)
static uint8_t data_buf[MAX_FUZZER_DATA + 1];

/* Should fuzzer harness code be built that compares behavior
 * with PCRE? (Obviously, this depends on PCRE.) */
#ifndef CMP_PCRE
#define CMP_PCRE 0
#endif

#if CMP_PCRE
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

static int
compare_with_pcre(const char *pattern, struct fsm *fsm);
#endif

/* 10 seconds */
#define TIMEOUT_USEC (10ULL * 1000 * 1000)

/* for TRACK_TIMES and EXPENSIVE_CHECKS */
#include "../src/libfsm/internal.h"

enum run_mode {
	MODE_REGEX,
	MODE_REGEX_SINGLE_ONLY,
	MODE_REGEX_MULTI_ONLY,
	MODE_IDEMPOTENT_DET_MIN,
	MODE_SHUFFLE_MINIMISE,
	MODE_ALL_PRINT_FUNCTIONS,
};

static size_t
get_env_config(size_t default_value, const char *env_var_name);

/* TODO: These could be moved to a common file for test utils. */
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

/* This is used to track allocation during each fuzzer
 * run. Note that hwm is not reduced when memory is
 * free'd or realloc'd, because the size info is not
 * passed to those calls. */
#define MB(X) ((size_t)X * 1000 * 1000)
#define FH_ALLOCATOR_HWM_LIMIT (MB(50))
struct fh_allocator_stats {
	size_t hwm;		/* high water mark */
};

static void
fh_memory_hwm_limit_hook(const char *caller_name)
{
	/* It doesn't really help to exit here because libfuzzer will
	 * still treat it as a failure, but at least we can print a
	 * message about hitting the allocator limit and exit so we
	 * don't need to spend time investigating timeouts or ooms
	 * that are due to obvious resource exhaustion. */
	fprintf(stderr, "%s: hit FH_ALLOCATOR_HWM_LIMIT (%zu), exiting\n",
	    caller_name, FH_ALLOCATOR_HWM_LIMIT);
	exit(EXIT_SUCCESS);
}

static void
fh_free(void *opaque, void *p)
{
	(void)opaque;
	free(p);
}

static void *
fh_calloc(void *opaque, size_t n, size_t sz)
{
	struct fh_allocator_stats *stats = opaque;
	stats->hwm += sz;
	if (stats->hwm > FH_ALLOCATOR_HWM_LIMIT) {
		fh_memory_hwm_limit_hook(__func__);
		return NULL;
	}

	(void)opaque;
	return calloc(n, sz);
}

static void *
fh_malloc(void *opaque, size_t sz)
{
	struct fh_allocator_stats *stats = opaque;
	stats->hwm += sz;
	if (stats->hwm > FH_ALLOCATOR_HWM_LIMIT) {
		fh_memory_hwm_limit_hook(__func__);
		return NULL;
	}

	return malloc(sz);
}

static void *
fh_realloc(void *opaque, void *p, size_t sz)
{
	struct fh_allocator_stats *stats = opaque;
	stats->hwm += sz;
	if (stats->hwm > FH_ALLOCATOR_HWM_LIMIT) {
		fh_memory_hwm_limit_hook(__func__);
		return NULL;
	}

	return realloc(p, sz);
}

static struct fh_allocator_stats allocator_stats;

/* fuzzer harness allocators */
static struct fsm_alloc custom_allocators = {
	.free = fh_free,
	.calloc = fh_calloc,
	.malloc = fh_malloc,
	.realloc = fh_realloc,
	.opaque = &allocator_stats,
};

static const struct fsm_options fsm_options = {
	.group_edges = 1,	/* make output readable */
	.alloc = &custom_allocators,
};

static void
dump_pattern(const char *pattern)
{
	const size_t pattern_length = strlen(pattern);
	fprintf(stderr, "-- Pattern: %zu bytes\n", pattern_length);
	for (size_t i = 0; i < pattern_length; i++) {
		fprintf(stderr, " %02x", (uint8_t)pattern[i]);
		if ((i & 31) == 31) { fprintf(stderr, "\n"); }
	}
	if ((pattern_length & 31) != 31) {
		fprintf(stderr, "\n");
	}
	for (size_t i = 0; i < pattern_length; i++) {
		fprintf(stderr, "%c", isprint(pattern[i]) ? pattern[i] : '.');
		if ((i & 63) == 63) { fprintf(stderr, "\n"); }
	}
	fprintf(stderr, "\n");
}

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
	fsm = re_comp(RE_PCRE, scanner_next, &s, &fsm_options, RE_MULTI, &err);
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
		dump_pattern(pattern);
		assert(!"timeout");
#else
		(void)dump_pattern;
		fprintf(stderr, "exiting zero due to timeout under EXPENSIVE_CHECKS\n");
		exit(0);
#endif
	}

	return fsm;
}

static size_t
get_env_config(size_t default_value, const char *env_var_name)
{
	const char *s = getenv(env_var_name);
	if (s == NULL) {
		return default_value;
	} else {
		return strtoul(s, NULL, 10);
	}
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
build_and_check_single(const char *pattern)
{
	const int verbosity = get_env_config(0, "VERBOSITY");
	if (verbosity > 1) {
		fprintf(stderr, "pattern: \"%s\"\n", pattern);
	}

	INIT_TIMERS();
	TIME(&pre);
	struct fsm *fsm = build(pattern);
	if (fsm == NULL) {
		return EXIT_SUCCESS;
	}
	TIME(&post);
	DIFF_MSEC("build", pre, post, NULL);

	if (getenv("DUMP")) {
		fprintf(stderr,"==================================================\n");
		fsm_print_fsm(stderr, fsm);
		fprintf(stderr,"==================================================\n");
		fsm_capture_dump(stderr, "CAPTURE", fsm);
		fprintf(stderr,"==================================================\n");
	}

#if CMP_PCRE
	TIME(&pre);
	const int cmp_res = compare_with_pcre(pattern, fsm);
	TIME(&post);
	DIFF_MSEC("cmp", pre, post, NULL);
	if (!cmp_res) {
		fsm_free(fsm);
		return EXIT_SUCCESS;
	}
#endif

	TIME(&pre);
	const int codegen_res = codegen(fsm);
	TIME(&post);
	DIFF_MSEC("codegen", pre, post, NULL);
	if (!codegen_res) {
		return EXIT_SUCCESS;
	}

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

#define DEF_MAX_DEPTH 20
#define DEF_MAX_LENGTH 10
#define DEF_MAX_STEPS 10000
#define DEF_MAX_MATCH_COUNT 1000

#if CMP_PCRE
/* These two are only used with PCRE2 */
#define ANCHORED_PCRE 0
#define FUZZ_RE_MATCH_LIMIT 10000
#define FUZZ_RE_RECURSION_LIMIT 200
#define MAX_OVEC_SIZE 512

static pcre2_match_context *pcre2_mc = NULL;

struct cmp_pcre_env {
	int verbosity;
	const char *pattern;
	const struct fsm *fsm;
	pcre2_match_data *md;
	pcre2_code *p;

	struct fsm_capture *captures;
	size_t captures_length;

	size_t max_depth;
	size_t max_steps;
	size_t max_match_count;
};

struct test_pcre_match_info {
	int res;
	int pcre_error;
	size_t ovector[MAX_OVEC_SIZE];
};

static pcre2_code *
build_pcre2(const char *pattern, int verbosity)
{
	const uint32_t options = ANCHORED_PCRE ? PCRE2_ANCHORED : 0;
	int errorcode;
	PCRE2_SIZE erroffset = 0;
	pcre2_compile_context *cctx = NULL;

	/* Set match limits */
	if (pcre2_mc == NULL) {
		pcre2_mc = pcre2_match_context_create(NULL);
		assert(pcre2_mc != NULL);

		pcre2_set_match_limit(pcre2_mc, FUZZ_RE_MATCH_LIMIT);
		pcre2_set_recursion_limit(pcre2_mc, FUZZ_RE_RECURSION_LIMIT);
	}

	pcre2_code *p = pcre2_compile((const unsigned char *)pattern,
	    PCRE2_ZERO_TERMINATED,
	    options, &errorcode, &erroffset, cctx);
	if (verbosity > 0 && p == NULL && errorcode != 0) {
#define ERRSIZE 4096
		unsigned char errbuf[ERRSIZE] = {0};
		if (!pcre2_get_error_message(errorcode,
			errbuf, ERRSIZE)) {
			fprintf(stderr, "pcre2_get_error_message: failed\n");
		}
		fprintf(stderr, "pcre2_compile: error: %s\n", errbuf);
	}
	return p;
}

enum do_pcre_match_res {
	DO_PCRE_MATCH_HIT,
	DO_PCRE_MATCH_MISS,
	DO_PCRE_MATCH_SKIP, /* an exceptional case we don't care about */
	DO_PCRE_MATCH_ERROR = -1,
};
enum do_pcre_match_res
do_pcre_match(FILE *f, const pcre2_code *p, pcre2_match_data *md, int verbosity,
	const char *input, struct test_pcre_match_info *match_info)
{
#define MAX_BUF (64*1024)
	const size_t input_len = strlen(input);
	enum do_pcre_match_res mres;

	/* turn off the JIT because it can give inconsistent results while fuzzing */
	const uint32_t options = (ANCHORED_PCRE ? PCRE2_ANCHORED : 0)
	    | PCRE2_NO_JIT;

	assert(pcre2_mc != NULL);

	/* The value returned by pcre2_match() is one more than the
	 * highest numbered pair that has been set. */
	int res = pcre2_match(p, (const unsigned char *)input, input_len,
	    0, options, md, pcre2_mc);

	if (res == PCRE2_ERROR_NOMATCH || res == PCRE2_ERROR_PARTIAL) {
		if (f != NULL && verbosity > 1) {
			fprintf(f, " -- no match (%s)\n",
				res == PCRE2_ERROR_NOMATCH ? "NOMATCH"
			    : res == PCRE2_ERROR_PARTIAL ? "PARTIAL"
			    : "<unknown>");
		}
		mres = DO_PCRE_MATCH_MISS;
		goto cleanup;
	} else if (res == PCRE2_ERROR_MATCHLIMIT || res == PCRE2_ERROR_DEPTHLIMIT) {
		/* It's possible to exhaust PCRE's internal limits with pathologically
		 * nested regexes like "(((((((((^.)?)*)?)?)?)*)+)+)*$" and
		 * "((((((((akbzaabdcOaa)|((((b*))))?|.|.|.*|.|.))+)+)+$)*)?)" , but
		 * as long as they don't cause it to block for excessively long or
		 * exhaust resources that's fine. */
		if (f != NULL) {
			fprintf(f, " -- PCRE2_ERROR_MATCHLIMIT (returning SKIP)\n");
		}
		mres = DO_PCRE_MATCH_SKIP;
	} else if (res <= 0) {
		if (f != NULL) {
#define ERR_MAX 4096
			unsigned char err_buf[ERR_MAX];
			if (pcre2_get_error_message(res, err_buf, ERR_MAX)) {
				fprintf(f, " -- error %d: %s\n", res, err_buf);
			} else {
				fprintf(f, " -- error %d\n", res);
			}
#undef ERR_MAX
		}
		if (match_info != NULL) {
			match_info->pcre_error = res;
		}
		mres = DO_PCRE_MATCH_ERROR;
		goto cleanup;
	} else {
		const uint32_t ovc = pcre2_get_ovector_count(md);
		PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(md);
		assert(res >= 0);
		size_t ures = (size_t)res;
		assert(ovc > ures);

		assert(ovector[1] >= ovector[0]);
		const size_t mlen = ovector[1] - ovector[0];
		if (ANCHORED_PCRE && (ovector[0] != 0 || mlen != input_len)) {
			mres = DO_PCRE_MATCH_MISS;
			goto cleanup;
		}
		mres = DO_PCRE_MATCH_HIT;

		if (f != NULL && verbosity > 1) {
			for (size_t i = 0; i < ures; i++) {
				char buf[MAX_BUF] = { 0 };
				memcpy(buf, &input[ovector[2*i]],
				    ovector[2*i + 1U] - ovector[2*i]);
				fprintf(f, " -- %zu: \"%s\"\n", i, buf);
			}
		}

		if (match_info != NULL && res < MAX_OVEC_SIZE) {
			match_info->res = res;
			assert(res >= 0);
			const size_t ures = (size_t)res;

			for (size_t i = 0; i < 2*ures; i++) {
				match_info->ovector[i] = ovector[i];
			}
		}
	}

cleanup:
	return mres;
#undef MAX_BUF
}

static bool
exec_and_compare_captures(struct cmp_pcre_env *env,
	const char *input, size_t input_size,
	const struct test_pcre_match_info *match_info)
{
	bool matching = true;
	fsm_state_t end_state;
	const uint8_t *u8_input = (const uint8_t *)input;
	int res = fsm_exec_with_captures(env->fsm, u8_input, input_size,
	    &end_state, env->captures, env->captures_length);

	if (res < 0) {
		if (env->verbosity > 1) {
			fprintf(stderr, "got res of %d\n", res);
		}

		return false;
	}

	if (res > 0) {
		assert(match_info->res >= 0);
		const size_t ures = (size_t)match_info->res;

		if (env->verbosity > 1) {
			fprintf(stderr, "ures %zu\n", ures);
		}

		for (size_t i = 0; i < ures; i++) {
			if (env->verbosity > 1) {
				fprintf(stderr, "%zu/%zu: pcre [%ld, %ld] <-> libfsm [%ld, %ld]\n",
				    i, ures,
				    match_info->ovector[2*i], match_info->ovector[2*i + 1],
				    env->captures[i].pos[0], env->captures[i].pos[1]);
			}
			if ((match_info->ovector[2*i] != env->captures[i].pos[0])
			    || (match_info->ovector[2*i + 1] != env->captures[i].pos[1])) {
				matching = false;
			}
		}

		if (!matching) {
			for (size_t i = 0; i < ures; i++) {
				fprintf(stderr, "%zu/%zu: pcre [%ld, %ld] <-> libfsm [%ld, %ld]\n",
				    i, ures,
				    match_info->ovector[2*i], match_info->ovector[2*i + 1],
				    env->captures[i].pos[0], env->captures[i].pos[1]);
			}
		}
	}

	return matching;
}

static void
dump_pattern_and_input(const char *pattern, const char *input, size_t input_length)
{
	dump_pattern(pattern);

	fprintf(stderr, "-- Input: %zu bytes\n", input_length);
	for (size_t i = 0; i < input_length; i++) {
		fprintf(stderr, " %02x", (uint8_t)input[i]);
		if ((i & 31) == 31) { fprintf(stderr, "\n"); }
	}
	if ((input_length & 31) != 31) {
		fprintf(stderr, "\n");
	}
	for (size_t i = 0; i < input_length; i++) {
		fprintf(stderr, "%c", isprint(input[i]) ? input[i] : '.');
		if ((i & 63) == 63) { fprintf(stderr, "\n"); }
	}
	fprintf(stderr, "\n");
}

static enum fsm_generate_matches_cb_res
cmp_pcre_gen_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	struct cmp_pcre_env *env = opaque;
	assert(env != NULL);

	(void)fsm;
	(void)depth;
	(void)end_state;

	const size_t len = strlen(input);

	if (env->verbosity > 4) {
		fprintf(stderr, "%s: depth %zu/%zu, match_count %zu/%zu, steps %zu/%zu\n",
		    __func__,
		    depth, env->max_depth,
		    match_count, env->max_match_count,
		    steps, env->max_steps);
	}

	if (steps > env->max_steps) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	if (match_count > env->max_match_count) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	if (depth > env->max_depth) {
		return FSM_GENERATE_MATCHES_CB_RES_PRUNE;
	}

	/* Completely avoid exploring inputs with embedded 0x00 bytes. */
	if (input_length != len) {
		return FSM_GENERATE_MATCHES_CB_RES_PRUNE;
	}

	if (len > 0 && input[len - 1] == '\n') {
		/* These will need to be handled properly, but PCRE has
		 * special cases for '\n' handling. */
		/* fprintf(stderr, " -- skipping input ending with '\\n'.\n"); */
		return FSM_GENERATE_MATCHES_CB_RES_PRUNE;
	}

	struct test_pcre_match_info match_info = { .pcre_error = 0 };
	enum do_pcre_match_res mres = do_pcre_match(stderr,
	    env->p, env->md, env->verbosity, input, &match_info);
	switch (mres) {
	case DO_PCRE_MATCH_SKIP:
		break;
	case DO_PCRE_MATCH_MISS:
		dump_pattern_and_input(env->pattern, input, input_length);
		assert(!"matches libfsm but not with PCRE");
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	case DO_PCRE_MATCH_ERROR:
		fprintf(stderr, "FAIL: PCRE returned ERROR %d: pattern \"%s\"\n",
		    match_info.pcre_error, env->pattern);
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	case DO_PCRE_MATCH_HIT:
		break;	/* okay; continue below */
	}

	if (env->verbosity > 1) {
		fprintf(stderr, "-- comparing captures for pattern \"%s\", input \"%s\" (len %zu)\n",
		    env->pattern, input, len);
	}

	if (!exec_and_compare_captures(env, input, input_length, &match_info)) {
		if (env->verbosity > 1 || 1) {
			dump_pattern_and_input(env->pattern, input, input_length);
			fsm_print_fsm(stderr, env->fsm);
			fsm_capture_dump(stderr, "fsm", env->fsm);
		}
		assert(!"captures don't match");
	}

	return FSM_GENERATE_MATCHES_CB_RES_CONTINUE;
}

static int
compare_fixed_input(struct fsm *fsm, const char *pattern, const char *input, pcre2_match_data *md, pcre2_code *p)
{
	fsm_state_t end_state;
	const size_t capture_ceil = fsm_capture_ceiling(fsm);

	struct fsm_capture *captures = malloc(capture_ceil * sizeof(captures[0]));
	assert(captures != NULL);
	for (size_t i = 0; i < capture_ceil; i++) {
		/* clobber with meaningless but visually distinct values */
		captures[i].pos[0] = 88888888;
		captures[i].pos[1] = 99999999;
	};

	const uint8_t *u8_input = (const uint8_t *)input;
	const size_t input_len = strlen(input);
	const int libfsm_res = fsm_exec_with_captures(fsm, u8_input, input_len,
	    &end_state, captures, capture_ceil);

	const bool libfsm_matching = libfsm_res > 0;

	int res = 1;

	struct test_pcre_match_info match_info = { .pcre_error = 0 };
	enum do_pcre_match_res mres = do_pcre_match(stderr,
	    p, md, 0, input, &match_info);
	switch (mres) {
	case DO_PCRE_MATCH_SKIP:
		return 1;
	case DO_PCRE_MATCH_MISS:
		if (!libfsm_matching) {
			goto cleanup;
		}
		dump_pattern_and_input(pattern, input, 0);
		assert(!"matches libfsm but not with PCRE");
		return 0;
	case DO_PCRE_MATCH_ERROR:
		fprintf(stderr, "FAIL: PCRE returned ERROR %d: pattern \"%s\"\n",
		    match_info.pcre_error, pattern);
		return 0;
	case DO_PCRE_MATCH_HIT:
		if (!libfsm_matching) {
			dump_pattern_and_input(pattern, input, input_len);
			assert(!"matches PCRE but not libfsm");
			res = 0;
			goto cleanup;
		}

		const size_t ures = (size_t)match_info.res;
		if (ures > capture_ceil) {
			dump_pattern_and_input(pattern, input, 0);
			fprintf(stderr, "error: capture_ceil: %zu exceeded by ures: %zd\n",
			    capture_ceil, ures);
			assert(!"both PCRE and libfsm match but with different capture counts");
		}

		bool matching = true;
		for (size_t i = 0; i < ures; i++) {
			if ((match_info.ovector[2*i] != captures[i].pos[0])
			    || (match_info.ovector[2*i + 1] != captures[i].pos[1])) {
				matching = false;
			}
		}
		for (size_t i = 0; i < ures; i++) {
			if (!matching) {
				fprintf(stderr, "%zu/%zu: pcre [%ld, %ld] <-> libfsm [%ld, %ld]\n",
				    i, ures,
				    match_info.ovector[2*i], match_info.ovector[2*i + 1],
				    captures[i].pos[0], captures[i].pos[1]);
			}
		}

		if (!matching) {
			dump_pattern_and_input(pattern, input, 0);
			assert(!"both PCRE and libfsm match but with different captures");
		}

		goto cleanup; /* ok, both matched */
	}

	assert(!"unreachable");

cleanup:
	free(captures);
	return res;

}

static int
compare_with_pcre(const char *pattern, struct fsm *fsm)
{
	size_t verbosity = get_env_config(0, "VERBOSITY");
	size_t max_length = get_env_config(DEF_MAX_LENGTH, "MAX_LENGTH");
	size_t max_steps = get_env_config(DEF_MAX_STEPS, "MAX_STEPS");
	size_t max_depth = get_env_config(DEF_MAX_DEPTH, "MAX_DEPTH");
	size_t max_match_count = get_env_config(DEF_MAX_MATCH_COUNT, "MAX_MATCH_COUNT");
	int res = 1;

	pcre2_match_data *md;

	pcre2_code *p = build_pcre2(pattern, 0);
	if (p == NULL) {
		return 1;
	}

	md = pcre2_match_data_create(MAX_OVEC_SIZE, NULL);
	assert(md != NULL);

	/* Check the empty string and "\n", because PCRE has an awkward
	 * special case for "\n" that has complicated interactions
	 * with start and end anchoring. */
	if (!compare_fixed_input(fsm, pattern, "", md, p)
	    || !compare_fixed_input(fsm, pattern, "\n", md, p)) {
		pcre2_match_data_free(md);
		pcre2_code_free(p);
		return res;
	}

	struct fsm_capture captures[MAX_OVEC_SIZE/2] = { 0 };

	const size_t pattern_length = strlen(pattern);
	if (pattern_length >= max_length) {
		max_length = pattern_length + 1;
		static size_t max_max_length;
		if (max_length > max_max_length) {
			fprintf(stderr, "Note: increasing max_length to %zu\n",
			    pattern_length + 1);
			max_max_length = max_length;
			if (max_depth < max_length) {
				max_depth = max_length + 1;
			}
		}
	}

	struct cmp_pcre_env env = {
		.verbosity = (int)verbosity,
		.pattern = pattern,
		.fsm = fsm,
		.captures = captures,
		.captures_length = MAX_OVEC_SIZE/2,
		.md = md,
		.p = p,
		.max_steps = max_steps,
		.max_depth = max_depth,
		.max_match_count = max_match_count,
	};

	if (!fsm_generate_matches(fsm, max_length, cmp_pcre_gen_cb, &env)) {
		res = 0;
	}

	pcre2_match_data_free(md);
	pcre2_code_free(p);
	return res;
}
#endif

/* Note: combined_fsm and fsms[] are non-const because fsm_generate_matches
 * calls fsm_trim on them. */
static int
compare_separate_and_combined(int verbosity, size_t max_length, size_t count,
    struct fsm *combined_fsm, const struct fsm_combined_base_pair *bases,
    struct fsm **fsms);

static enum fsm_generate_matches_cb_res
cmp_separate_and_combined_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque);

static int
build_and_check_multi(const char *input)
{
	int res = EXIT_FAILURE;
	const int verbosity = get_env_config(0, "VERBOSITY");
#define MAX_PATTERNS 8
#define MAX_PATTERN_LEN 256
	char patterns[MAX_PATTERNS][MAX_PATTERN_LEN] = { 0 };
	size_t count = 0;
	const size_t len = strlen(input);
	size_t max_length = get_env_config(DEF_MAX_LENGTH, "MAX_LENGTH");
	INIT_TIMERS();

	/* if nonzero, apply a timeout to the combined FSM det/min below */
	const size_t timeout = get_env_config(0, "TIMEOUT");

	if (timeout > 0) {
		if (TRACK_TIMES == 0) {
			fprintf(stderr, "\n\n\n\n\nError: src/libfsm/internal.h:TRACK_TIMES needs to be nonzero for this use case, exiting.\n\n\n\n\n");
			exit(EXIT_FAILURE);
		} else {
			static bool printed_timeout_msg;
			if (!printed_timeout_msg) {
				fprintf(stderr, "Using timeout of %zu msec for fsm_determinise/fsm_minimise on combined FSM.\n",
				    timeout);
				printed_timeout_msg = true;
			}
		}
	}

	size_t i, j;
	for (i = 0, j = 0; i < len; i++) {
		const char c = input[i];
		if (c == '\n' || c == '\r') {
			if (j > max_length) {
				max_length = j;
			}
			count++;
			if (count == MAX_PATTERNS) {
				/* ignore: too many patterns */
				return EXIT_SUCCESS;
			}
			j = 0;
		} else {
			patterns[count][j] = c;
			j++;
			if (j == MAX_PATTERN_LEN) {
				/* ignore: pattern too long */
				return EXIT_SUCCESS;
			}
		}
	}
	if (j > 0) { count++; }

	if (count == 1) {
		/* multi mode with only one pattern is pointless */
		return EXIT_SUCCESS;
	}

	struct re_err err;
	const enum re_flags flags = 0;

	/* build each regex, combining them and keeping track of capture offsets */
	struct fsm *fsms[count];
	struct fsm *fsms_cp[count];
	struct fsm_combined_base_pair bases[count];
	struct fsm *combined_fsm = NULL;
	for (size_t i = 0; i < count; i++) {
		fsms[i] = NULL;
		fsms_cp[i] = NULL;

		bases[i].state = 0;
		bases[i].capture = 0;
	}

	/* compile each individually */
	/* FIXME: apply and check endids */
	for (size_t i = 0; i < count; i++) {
		if (verbosity > 1) {
			fprintf(stderr, "%s: compiling \"%s\"\n",
			    __func__, patterns[i]);
		}

		struct scanner s = {
			.str    = (const uint8_t *)patterns[i],
			.size   = strlen(patterns[i]),
		};

		struct fsm *fsm = re_comp(RE_PCRE, scanner_next, &s, &fsm_options, flags, &err);
		if (fsm == NULL) {
			res = EXIT_SUCCESS; /* invalid regex, so skip this batch */
			goto cleanup;
		}

		/* set endid to associate each FSM with its pattern */
		if (!fsm_setendid(fsm, (fsm_end_id_t)i)) {
			goto cleanup;
		}

		char label_buf[100];
		snprintf(label_buf, 100, "single_determisise_%zu", i);

		TIME(&pre);
		if (!fsm_determinise(fsm)) {
			goto cleanup;
		}
		TIME(&post);
		DIFF_MSEC(label_buf, pre, post, NULL);

		snprintf(label_buf, 100, "single_minimise_%zu", i);
		TIME(&pre);
		if (!fsm_minimise(fsm)) {
			goto cleanup;
		}
		TIME(&post);
		DIFF_MSEC(label_buf, pre, post, NULL);

		if (verbosity > 4) {
			char tag_buf[16] = { 0 };
			snprintf(tag_buf, sizeof(tag_buf), "fsm[%zu]", i);

			fprintf(stderr, "==== fsm[%zu]\n", i);
			fsm_print_fsm(stderr, fsm);
			fsm_capture_dump(stderr, tag_buf, fsm);
		}

		fsms[i] = fsm;
		fsms_cp[i] = fsm_clone(fsm); /* save a copy for comparison */
	}

	combined_fsm = fsm_union_array(count, fsms, bases);
	assert(combined_fsm != NULL);
	if (verbosity > 1) {
		fprintf(stderr, "%s: combined_fsm: %d states after fsm_union_array\n",
		    __func__, fsm_countstates(combined_fsm));
	}
	if (verbosity > 1) {
		for (size_t i = 0; i < count; i++) {
			fprintf(stderr, "%s: base[%zu]: state %d, capture %u\n",
			    __func__, i, bases[i].state, bases[i].capture);
		}
	}

	TIME(&pre);
	if (!fsm_determinise(combined_fsm)) {
		goto cleanup;
	}
	TIME(&post);
	size_t timeout_accum = 0;
	if (timeout != 0) {
		if (verbosity > 1) {
			DIFF_MSEC_ALWAYS("combined_determinise", pre, post, &timeout_accum);
		} else {
			DIFF_MSEC("combined_determinise", pre, post, &timeout_accum);
		}
		assert(timeout_accum < timeout);
		timeout_accum = 0;
	}

	const unsigned states_after_determinise = fsm_countstates(combined_fsm);
	if (verbosity > 1) {
		fprintf(stderr, "%s: combined_fsm: %d states after determinise\n",
		    __func__, states_after_determinise);
	}

	TIME(&pre);
	if (!fsm_minimise(combined_fsm)) {
		goto cleanup;
	}
	TIME(&post);
	if (timeout != 0) {
		if (verbosity > 1) {
			DIFF_MSEC_ALWAYS("combined_minimise", pre, post, &timeout_accum);
		} else {
			DIFF_MSEC("combined_minimise", pre, post, &timeout_accum);
		}
		assert(timeout_accum < timeout);
		timeout_accum = 0;
	}

	const unsigned states_after_minimise = fsm_countstates(combined_fsm);
	if (verbosity > 1) {
		fprintf(stderr, "%s: combined_fsm: %d states after minimise\n",
		    __func__, states_after_minimise);
	}

	if (verbosity > 4) {
		fprintf(stderr, "==== combined\n");
		fsm_print_fsm(stderr, combined_fsm);
		fsm_capture_dump(stderr, "combined", combined_fsm);
	}

	res = compare_separate_and_combined(verbosity, max_length,
	    count, combined_fsm, bases, (struct fsm **)fsms_cp);

	for (i = 0; i < count; i++) {
		fsm_free(fsms_cp[i]);
	}
	fsm_free(combined_fsm);

	if (res == EXIT_SUCCESS) {
		static size_t pass_count;
		if (verbosity == 1) {
			fprintf(stderr, "%s: pass: %zu, %zu patterns\n",
			    __func__, ++pass_count, count);
		} else if (verbosity > 1) {
			fprintf(stderr, "%s: pass: %zu, %zu patterns\n",
			    __func__, ++pass_count, count);
			for (i = 0; i < count; i++) {
				fprintf(stderr, " -- %zu: \"%s\"\n",
				    i, patterns[i]);
			}
		}
	}

	return res;

cleanup:
	for (i = 0; i < count; i++) {
		if (fsms[i] != NULL) {
			fsm_free(fsms[i]);
		}
		if (fsms_cp[i] != NULL) {
			fsm_free(fsms_cp[i]);
		}
	}
	if (combined_fsm != NULL) {
		fsm_free(combined_fsm);
	}
	return res;
}

struct cmp_combined_env {
	bool ok;
	int verbosity;
	size_t count;
	struct fsm *combined_fsm;
	const struct fsm_combined_base_pair *bases;
	size_t current_i;
	struct fsm **fsms;
	size_t max_depth;
	size_t max_steps;
	size_t max_match_count;
};

static enum fsm_generate_matches_cb_res
cmp_combined_with_separate_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque);

static int
compare_separate_and_combined(int verbosity, size_t max_length, size_t count,
    struct fsm *combined_fsm, const struct fsm_combined_base_pair *bases,
    struct fsm **fsms)
{
	const size_t max_steps = get_env_config(DEF_MAX_STEPS, "MAX_STEPS");
	const size_t max_depth = get_env_config(DEF_MAX_DEPTH, "MAX_DEPTH");
	const size_t max_match_count = get_env_config(DEF_MAX_MATCH_COUNT, "MAX_MATCH_COUNT");

	struct cmp_combined_env env = {
		.ok = true,
		.verbosity = verbosity,
		.count = count,
		.combined_fsm = combined_fsm,
		.bases = bases,
		.fsms = fsms,
		.max_steps = max_steps,
		.max_depth = max_depth,
		.max_match_count = max_match_count,
	};

	/* For each individual fsm, generate matching inputs and check that
	 * they match with the same captures in the combined fsm. */
	for (env.current_i = 0; env.current_i < count; env.current_i++) {
		if (!fsm_generate_matches(env.fsms[env.current_i], max_length,
			cmp_separate_and_combined_cb, &env)) {
			env.ok = false;
		}
		if (!env.ok) {
			break;
		}
	}
	env.current_i = (size_t)-1;

	/* Also go in the other direction, generating matches with
	 * combined and check the individual ones match as expected. */
	if (env.ok) {
		if (!fsm_generate_matches(env.combined_fsm, max_length,
			cmp_combined_with_separate_cb, &env)) {
			env.ok = false;
		}
	}

	return env.ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static enum fsm_generate_matches_cb_res
cmp_separate_and_combined_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	struct cmp_combined_env *env = opaque;
	(void)end_state;

	if (steps > env->max_steps) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	if (depth > env->max_depth) {
		return FSM_GENERATE_MATCHES_CB_RES_PRUNE;
	}

	if (match_count > env->max_match_count) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

#define MAX_CAPTURES 256
	struct fsm_capture captures_single[MAX_CAPTURES];
	struct fsm_capture captures_combined[MAX_CAPTURES];

	const fsm_end_id_t expected_end_id = (fsm_end_id_t)env->current_i;

	const uint8_t *u8_input = (const uint8_t *)input;
	fsm_state_t end_state_combined, end_state_single;

	const int res_combined = fsm_exec_with_captures(env->combined_fsm, u8_input, input_length,
	    &end_state_combined, captures_combined, MAX_CAPTURES);
	const int res_single = fsm_exec_with_captures(fsm, u8_input, input_length,
	    &end_state_single, captures_single, MAX_CAPTURES);

	if (res_combined != res_single) {
		env->ok = false;
		if (env->verbosity > 0) {
			fprintf(stderr, "%s: res_combined %d != res_single %d\n",
			    __func__, res_combined, res_single);
		}
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	fsm_end_id_t id_buf_combined[MAX_PATTERNS];
	size_t written_combined = 0;
	if (res_combined > 0) {
		const size_t exp_written = fsm_getendidcount(env->combined_fsm, end_state_combined);
		assert(exp_written <= env->count);
		const enum fsm_getendids_res gres = fsm_getendids(env->combined_fsm,
		    end_state_combined, MAX_PATTERNS, id_buf_combined, &written_combined);
		assert(gres == FSM_GETENDIDS_FOUND);
		assert(written_combined == exp_written);
	}

	/* we got here, so we have a match */
	assert(res_single > 0);

	if (env->verbosity > 3) {
		fprintf(stderr, "%s: res %d (single and combined)\n", __func__, res_single);
	}

	/* Check that the end state's endid for the single DFA is among the
	 * endids for the combined DFA's end state. */
	assert(fsm_getendidcount(fsm, end_state_single) == 1);
	assert(fsm_getendidcount(env->combined_fsm, end_state_combined) <= env->count);

	fsm_end_id_t id_buf_single[1];
	size_t written;
	const enum fsm_getendids_res gres = fsm_getendids(fsm,
	    end_state_single, 1, id_buf_single, &written);
	assert(gres == FSM_GETENDIDS_FOUND);
	assert(written == 1);
	assert(id_buf_single[0] == expected_end_id);

	bool found_single_id_in_combined = false;
	for (size_t i = 0; i < written_combined; i++) {
		if (id_buf_combined[i] == expected_end_id) {
			found_single_id_in_combined = true;
			break;
		}
	}
	assert(found_single_id_in_combined);

	bool matching = true;
	const unsigned base = env->bases[env->current_i].capture;
	assert(base < MAX_CAPTURES);
	for (int i = 0; i < res_single; i++) {
		if (env->verbosity > 3) {
			fprintf(stderr, "%d/%d: single [%ld, %ld] <-> combined [%ld, %ld]\n",
			    i, res_single,
			    captures_single[i].pos[0], captures_single[i].pos[1],
			    captures_combined[i + base].pos[0], captures_combined[i + base].pos[1]);
		}
		if ((captures_single[i].pos[0] != captures_combined[i + base].pos[0]) ||
		    (captures_single[i].pos[1] != captures_combined[i + base].pos[1])) {
			matching = false;
		}
	}

	if (!matching) {
		for (int i = 0; i < res_single; i++) {
			fprintf(stderr, "%d/%d: single [%ld, %ld] <-> combined [%ld, %ld]\n",
			    i, res_single,
			    captures_single[i].pos[0], captures_single[i].pos[1],
			    captures_combined[i + base].pos[0], captures_combined[i + base].pos[1]);
		}
		env->ok = false;
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	return FSM_GENERATE_MATCHES_CB_RES_CONTINUE;
}

static enum fsm_generate_matches_cb_res
cmp_combined_with_separate_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	/* We have an input that matched the combined DFA,
	 * use the set of end IDs to check which of the
	 * single DFAs it should/should not match, and check
	 * the endid behavior. */

	struct cmp_combined_env *env = opaque;

	if (steps > env->max_steps) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	if (depth > env->max_depth) {
		return FSM_GENERATE_MATCHES_CB_RES_PRUNE;
	}

	if (match_count > env->max_match_count) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

#define MAX_CAPTURES 256
	struct fsm_capture captures_single[MAX_CAPTURES];
	struct fsm_capture captures_combined[MAX_CAPTURES];

	const uint8_t *u8_input = (const uint8_t *)input;

	fsm_state_t end_state_combined;
	assert(fsm == env->combined_fsm);
	const int res_combined = fsm_exec_with_captures(env->combined_fsm, u8_input, input_length,
	    &end_state_combined, captures_combined, MAX_CAPTURES);
	assert(res_combined > 0); /* we got here, so we have a match */
	assert(end_state_combined == end_state);

	fsm_end_id_t id_buf_combined[MAX_PATTERNS];
	size_t written_combined = 0;
	{
		const size_t exp_written = fsm_getendidcount(env->combined_fsm, end_state_combined);
		assert(exp_written <= env->count);
		const enum fsm_getendids_res gres = fsm_getendids(env->combined_fsm,
		    end_state_combined, MAX_PATTERNS, id_buf_combined, &written_combined);
		assert(gres == FSM_GETENDIDS_FOUND);
		assert(written_combined == exp_written);
	}

	/* For each pattern, check if its endid is in the combined DFA's end state
	 * endids. If so, it should match, otherwise it should not. */
	for (size_t pattern_i = 0; pattern_i < env->count; pattern_i++) {
		const struct fsm *single_fsm = env->fsms[pattern_i];
		bool found = false;
		for (size_t endid_i = 0; endid_i < written_combined; endid_i++) {
			const fsm_end_id_t endid = id_buf_combined[endid_i];
			if (endid == pattern_i) {
				found = true;
				break;
			}
		}
		fsm_state_t end_state_single;

		const int res_single = fsm_exec_with_captures(single_fsm,
		    u8_input, input_length,
		    &end_state_single, captures_single, MAX_CAPTURES);

		if (found) {
			assert(res_single > 0);
			fsm_end_id_t id_buf_single[1];
			size_t written;
			const enum fsm_getendids_res gres = fsm_getendids(single_fsm,
			    end_state_single, 1, id_buf_single, &written);
			assert(gres == FSM_GETENDIDS_FOUND);
			assert(written == 1);
			assert(id_buf_single[0] == pattern_i);

			/* check captures */
			bool matching = true;
			const unsigned base = env->bases[pattern_i].capture;
			assert(base < MAX_CAPTURES);
			for (int i = 0; i < res_single; i++) {
				if (env->verbosity > 3) {
					fprintf(stderr, "%d/%d: single [%ld, %ld] <-> combined [%ld, %ld]\n",
					    i, res_single,
					    captures_single[i].pos[0], captures_single[i].pos[1],
					    captures_combined[i + base].pos[0], captures_combined[i + base].pos[1]);
				}
				if ((captures_single[i].pos[0] != captures_combined[i + base].pos[0]) ||
				    (captures_single[i].pos[1] != captures_combined[i + base].pos[1])) {
					matching = false;
				}
			}

			if (!matching) {
				for (int i = 0; i < res_single; i++) {
					fprintf(stderr, "%d/%d: single [%ld, %ld] <-> combined [%ld, %ld]\n",
					    i, res_single,
					    captures_single[i].pos[0], captures_single[i].pos[1],
					    captures_combined[i + base].pos[0], captures_combined[i + base].pos[1]);
				}
				env->ok = false;
				return FSM_GENERATE_MATCHES_CB_RES_HALT;
			}
		} else {
			assert(res_single == 0); /* no match */
		}
	}

	return FSM_GENERATE_MATCHES_CB_RES_CONTINUE;
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

	fsm = re_comp(RE_PCRE, scanner_next, &s, &fsm_options, RE_MULTI, &err);

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
			fsm_print_fsm(stderr, fsm);


			fprintf(stderr, "== expected:\n");
			fsm_print_fsm(stderr, oracle_min);

			fprintf(stderr, "== got:\n");
			fsm_print_fsm(stderr, cp);

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

	const struct fsm_options options = {
		.io = io_mode,
	};

	fsm = re_comp(RE_PCRE, scanner_next, &s, &options, RE_MULTI, &err);
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
	r |= fsm_print_api(f, fsm);
	r |= fsm_print_awk(f, fsm);
	r |= fsm_print_c(f, fsm);
	r |= fsm_print_dot(f, fsm);
	r |= fsm_print_fsm(f, fsm);
	r |= fsm_print_ir(f, fsm);
	r |= fsm_print_irjson(f, fsm);
	r |= fsm_print_json(f, fsm);
	r |= fsm_print_vmc(f, fsm);
	r |= fsm_print_vmdot(f, fsm);
	r |= fsm_print_vmasm(f, fsm);
	r |= fsm_print_vmasm_amd64_att(f, fsm);
	r |= fsm_print_vmasm_amd64_nasm(f, fsm);
	r |= fsm_print_vmasm_amd64_go(f, fsm);
	r |= fsm_print_sh(f, fsm);
	r |= fsm_print_go(f, fsm);
	r |= fsm_print_rust(f, fsm);
	assert(r == 0 || errno != 0);

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

static int
build_and_test_idempotent_det_and_min(const char *pattern)
{
	const int verbosity = get_env_config(0, "VERBOSITY");
	assert(pattern != NULL);

	struct re_err err;
	struct fsm *fsm;
	const size_t length = strlen(pattern);

	struct scanner s = {
		.str    = (const uint8_t *)pattern,
		.size   = length,
	};

	fsm = re_comp(RE_PCRE, scanner_next, &s, &fsm_options, RE_MULTI, &err);
	if (fsm == NULL) {
		return EXIT_SUCCESS;
	}

	if (!fsm_determinise(fsm)) {
		return EXIT_FAILURE;
	}
	if (verbosity >= 3) {
		fprintf(stderr, "=== post_det_a\n");
		fsm_print_fsm(stderr, fsm);
	}
	const size_t post_det_a = fsm_countstates(fsm);

	if (!fsm_determinise(fsm)) {
		return EXIT_FAILURE;
	}
	if (verbosity >= 3) {
		fprintf(stderr, "=== post_det_b\n");
		fsm_print_fsm(stderr, fsm);
	}
	const size_t post_det_b = fsm_countstates(fsm);
	assert(post_det_b == post_det_a);

	if (!fsm_minimise(fsm)) {
		return EXIT_FAILURE;
	}
	if (verbosity >= 3) {
		fprintf(stderr, "=== post_min_a\n");
		fsm_print_fsm(stderr, fsm);
		fsm_capture_dump(stderr, "post_a", fsm);
	}
	const size_t post_min_a = fsm_countstates(fsm);

	if (!fsm_minimise(fsm)) {
		return EXIT_FAILURE;
	}
	if (verbosity >= 3) {
		fprintf(stderr, "=== post_min_b\n");
		fsm_print_fsm(stderr, fsm);
		fsm_capture_dump(stderr, "post_b", fsm);
	}
	const size_t post_min_b = fsm_countstates(fsm);
	assert(post_min_b == post_min_a);

	if (!fsm_determinise(fsm)) {
		return EXIT_FAILURE;
	}
	const size_t post_det_c = fsm_countstates(fsm);
	assert(post_det_c == post_min_b);

	if (!fsm_minimise(fsm)) {
		return EXIT_FAILURE;
	}
	const size_t post_min_c = fsm_countstates(fsm);
	assert(post_min_c == post_det_c);

	fsm_free(fsm);
	return EXIT_SUCCESS;
}

static enum run_mode
get_run_mode(void)
{
	const char *mode = getenv("MODE");
	if (mode == NULL) {
		return MODE_REGEX; /* default */
	}

	switch (mode[0]) {
	case 'r': return MODE_REGEX;
	case 's': return MODE_REGEX_SINGLE_ONLY;
	case 'm': return MODE_REGEX_MULTI_ONLY;
	case 'i': return MODE_IDEMPOTENT_DET_MIN;
	case 'M': return MODE_SHUFFLE_MINIMISE;
	case 'p': return MODE_ALL_PRINT_FUNCTIONS;
	default:
		fprintf(stderr, "Unrecognized mode '%c', expect one of:\n", mode[0]);
		fprintf(stderr, " - r.egex (default)\n");
		fprintf(stderr, " - s.ingle regex only\n");
		fprintf(stderr, " - m.ulti regex only\n");
		fprintf(stderr, " - M.inimisation shuffling\n");
		fprintf(stderr, " - i.dempotent determinise/minimise\n");
		fprintf(stderr, " - p.rint functions\n");
		exit(EXIT_FAILURE);
		break;
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
	/* ensure the buffer is 0-terminated */
	data_buf[size] = 0;

	/* truncate to a valid c string */
	size = strlen((const char *)data_buf);
	data_buf[size] = 0;

	/* reset for each run */
	allocator_stats.hwm = 0;

	size_t dot_count = 0;
	bool has_newline = false;
	size_t first_newline;

	for (size_t i = 0; i < size; i++) {
		const uint8_t c = data_buf[i];
		if (c == '.') {
			dot_count++;
			if (dot_count >= 4) {
				/* Too many '.'s can lead to a regex that is
				 * very slow to determinise/minimise, but that
				 * failure mode is not interesting to this
				 * particular fuzzer. */
				return EXIT_SUCCESS;
			}
		}

		if (c == '(') {
			/* This triggers an "unreached" assertion in the parser.
			 * It's already been reported (issue #386), but once the
			 * fuzzer finds it, it will report it over and over.
			 * Exit here so that the fuzzer considers it uninteresting. */
			if (size - i >= 3 && 0 == memcmp("(*:", &data_buf[i], 3)) {
				return EXIT_SUCCESS;
			}
		}

		if (c == '\\') {
			/* Not supported yet. */
			return EXIT_SUCCESS;
		}

		if (c == '\r' || c == '\n') {
			if (!has_newline) {
				first_newline = i;
			}
			has_newline = true;
		}
	}

	const char *pattern = (const char *)data_buf;

	switch (get_run_mode()) {
	case MODE_REGEX:
		if (has_newline) {
			return build_and_check_multi(pattern);
		} else {
			return build_and_check_single(pattern);
		}

	case MODE_REGEX_SINGLE_ONLY:
		if (has_newline) {
			return EXIT_SUCCESS; /* ignore */
		} else {
			return build_and_check_single(pattern);
		}

	case MODE_REGEX_MULTI_ONLY:
		if (has_newline) {
			return build_and_check_multi(pattern);
		} else {
			return EXIT_SUCCESS; /* ignore */
		}

	case MODE_IDEMPOTENT_DET_MIN:
		if (has_newline) {
			assert(data_buf[first_newline] == '\n'
			    || data_buf[first_newline] == '\r');
			data_buf[first_newline] = '\0';
		}
		return build_and_test_idempotent_det_and_min(pattern);

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

default:
		assert(!"match fail");
	}

	assert(!"unreached");
}
