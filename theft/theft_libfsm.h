/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef THEFT_LIBFSM_H
#define THEFT_LIBFSM_H

#include <sys/time.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include <unistd.h>

#include <theft.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/cost.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <re/re.h>

struct test_env {
	char tag;
	enum re_dialect dialect;
	uint8_t verbosity;
	struct theft_print_trial_result_env print_env;
	size_t shrink_timeout; /* in seconds */
	size_t started_second;
	size_t last_trace_second;
};

struct string_pair {
	size_t size;
	uint8_t *string;
};

void print_or_hexdump(FILE *f, const uint8_t *buf, size_t size);
void hexdump(FILE *f, const uint8_t *buf, size_t size);

typedef bool test_fun(theft_seed);
typedef bool test_fun1(theft_seed, uintptr_t arg);
typedef bool test_fun2(theft_seed,
    uintptr_t arg0, uintptr_t arg1);
typedef bool test_fun3(theft_seed,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

void reg_test(const char *name, test_fun *test);
void reg_test1(const char *name, test_fun1 *test,
    uintptr_t arg);
void reg_test2(const char *name, test_fun2 *test,
    uintptr_t arg0, uintptr_t arg1);
void reg_test3(const char *name, test_fun3 *test,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

/* Register tests within a module. */
void register_test_literals(void);
void register_test_re(void);
void register_test_adt_ipriq(void);
void register_test_adt_priq(void);
void register_test_capture_string_set(void);
void register_test_nfa(void);
void register_test_adt_edge_set(void);
void register_test_trim(void);
void register_test_minimise(void);

int test_get_verbosity(void);

/* Halt after one failure. */
enum theft_hook_trial_pre_res
trial_pre_fail_once(const struct theft_hook_trial_pre_info *info,
    void *penv);

/* On failure, increase verbosity and repeat once. */
enum theft_hook_trial_post_res
trial_post_inc_verbosity(const struct theft_hook_trial_post_info *info,
    void *penv);

size_t delta_msec(const struct timeval *pre, const struct timeval *post);

size_t *gen_permutation_vector(size_t length, uint32_t seed);

int
wrap_fsm_exec(struct fsm *fsm, const struct string_pair *pair, fsm_state_t *state);

struct fsm *
wrap_re_comp(enum re_dialect dialect, const struct string_pair *pair,
	const struct fsm_options *opt,
	struct re_err *err);

#define LOG_OUT stdout

#define VERBOSITY_LOW (test_get_verbosity())
#define VERBOSITY_HIGH (test_get_verbosity() + 1)

#if 1
#define LOG(...) fprintf(LOG_OUT, __VA_ARGS__)
#else
#define LOG(...)
#endif

#define ASSERT(COND, ...)					       \
	do {							       \
		if (!(COND)) {					       \
			if (m->env->verbosity > 0) {		       \
				fprintf(stderr, __VA_ARGS__);	       \
			}					       \
			return false;				       \
		}						       \
	} while(0)


#define LOG_FAIL(VERBOSITY, ...)				       \
	do {							       \
		if (VERBOSITY > 0) {				       \
			fprintf(LOG_OUT, __VA_ARGS__);		       \
		}						       \
	} while(0)

#endif

#define BITSET_CHECK(SET, BIT) (SET[BIT/64] & ((uint64_t)1 << (BIT & 63)))
#define BITSET_SET(SET, BIT) (SET[BIT/64] |= ((uint64_t)1 << (BIT & 63)))
