/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include <adt/xalloc.h>

void
print_or_hexdump(FILE *f, const uint8_t *buf, size_t size)
{
	bool all_printable;

	all_printable = true;

	for (size_t i = 0; i < size; i++) {
		if (!isprint(buf[i])) {
			all_printable = false;
			break;
		}
	}

	if (all_printable) {
		fprintf(f, "\n");
		hexdump(f, buf, size);
	} else {
		fprintf(f, "%s", buf);
	}
}

/* Uncomment to dump as a C hex literal string */
//#define HEXDUMP_CSTR
void
hexdump(FILE *f, const uint8_t *buf, size_t size)
{
	size_t offset = 0;
	while (offset < size) {
		size_t curr = (size - offset > 16 ? 16 : size - offset);

#ifdef HEXDUMP_CSTR
		for (size_t i = 0; i < curr; i++) {
			fprintf(f, "0x%02x, ", buf[offset + i]);
		}

#else
		fprintf(f, "%04zx -- ", offset);
		for (size_t i = 0; i < curr; i++) {
			fprintf(f, "%02x ", buf[offset + i]);
		}

		for (size_t i = curr; i < 16; i++) {
			fprintf(f, "   ");
		}

		fprintf(f, "  ");
		for (size_t i = 0; i < curr; i++) {
			uint8_t c = buf[offset + i];
			fprintf(f, "%c", (isprint(c) ? c : '.'));
		}
#endif
		fprintf(f, "\n");
		offset += curr;
	}
}

enum theft_hook_trial_pre_res
trial_pre_fail_once(const struct theft_hook_trial_pre_info *info,
    void *penv)
{
	struct test_env *env = penv;
	assert(env->tag == 'E');

	if (info->failures >= 1) {
		return THEFT_HOOK_TRIAL_PRE_HALT;
	}

	return THEFT_HOOK_TRIAL_PRE_CONTINUE;
}

enum theft_hook_trial_post_res
trial_post_inc_verbosity(const struct theft_hook_trial_post_info *info,
    void *penv)
{
	struct test_env *env;

	env = penv;

	assert(env->tag == 'E');
	env->verbosity = VERBOSITY_LOW;

	if (info->result == THEFT_TRIAL_FAIL) {
		env->verbosity = (!info->repeat ? VERBOSITY_HIGH : VERBOSITY_LOW);
		return THEFT_HOOK_TRIAL_POST_REPEAT_ONCE;
	} else if (info->result == THEFT_TRIAL_FAIL) {
		fprintf(stderr, "SKIP...\n");
	}

	return theft_hook_trial_post_print_result(info, &env->print_env);
}

size_t
delta_msec(const struct timeval *pre, const struct timeval *post)
{
	return (1000 * (post->tv_sec - pre->tv_sec)) +
		(post->tv_usec/1000 - pre->tv_usec/1000);
}

size_t *
gen_permutation_vector(size_t length, uint32_t seed)
{
	size_t *res;
	uint32_t mod;
	uint32_t ceil;
	uint32_t state;
	uint32_t a, c;
	size_t offset;
	uint32_t mask;

	res = xcalloc(length, sizeof *res);

	/*
	 * This is a linear congruential random number generator,
	 * which isn't actually that great as a random number generator,
	 * but has the handy property that it will walk through all
	 * the values in range exactly once before repeating any
	 * (provided the input variables are right).
	 * See Knuth Volume 2, section 3.2.1 for more details.
	 */

	mod  = 1;
	ceil = length;
	while (mod < ceil) {
		mod <<= 1;
	}

	state = seed & ((1LLU << 29) - 1);
	a = (4 * state) + 1;
	c = 2147483647;		/* (2 ** 31) - 1 */

	offset = 0;
	mask = mod - 1;
	while (offset < length) {
		do {
			state = ((a * state) + c) & mask;
		} while (state >= ceil);

		res[offset++] = state;
	}

	return res;
}

