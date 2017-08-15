#include "test_libfsm.h"

int scanner_next(void *opaque)
{
	struct scanner *s = (struct scanner *)opaque;
	assert(s->tag == 'S');

	if (s->offset == s->size) {
		return EOF;
	}

	unsigned char c = s->str[s->offset];
	s->offset++;
	int res = (int)c;

	return res;
}

void print_or_hexdump(FILE *f, const uint8_t *buf, size_t size)
{
	bool all_printable = true;
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

// Uncomment to dump as a C hex literal string
//#define HEXDUMP_CSTR
void hexdump(FILE *f, const uint8_t *buf, size_t size)
{
	size_t offset = 0;
	while (offset < size) {
		size_t cur = (size - offset > 16 ? 16 : size - offset);
#ifdef HEXDUMP_CSTR
		for (size_t i = 0; i < cur; i++) {
			fprintf(f, "0x%02x, ", buf[offset + i]);
		}

#else
		fprintf(f, "%04zx -- ", offset);
		for (size_t i = 0; i < cur; i++) {
			fprintf(f, "%02x ", buf[offset + i]);
		}

		for (size_t i = cur; i < 16; i++) {
			fprintf(f, "   ");
		}

		fprintf(f, "  ");
		for (size_t i = 0; i < cur; i++) {
			uint8_t c = buf[offset + i];
			fprintf(f, "%c", (isprint(c) ? c : '.'));
		}
#endif
		fprintf(f, "\n");
		offset += cur;
	}
}

enum theft_hook_trial_pre_res
trial_pre_fail_once(const struct theft_hook_trial_pre_info *info,
    void *penv)
{
	struct test_env *env = (struct test_env *)penv;
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
	struct test_env *env = (struct test_env *)penv;
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

size_t delta_msec(const struct timeval *pre, const struct timeval *post)
{
	return (1000 * (post->tv_sec - pre->tv_sec)) +
	    (post->tv_usec/1000 - pre->tv_usec/1000);
}

size_t *gen_permutation_vector(size_t length, uint32_t seed)
{
	size_t *res = calloc(length, sizeof(size_t));
	if (res == NULL) { return NULL; }

	/* This is a linear congruential random number generator,
	 * which isn't actually that great as a random number generator,
	 * but has the handy property that it will walk through all
	 * the values in range exactly once before repeating any
	 * (provided the input variables are right). See Knuth
	 * Volume 2, section 3.2.1 for more details. */

	uint32_t mod = 1;
	const uint32_t ceil = length;
	while (mod < ceil) { mod <<= 1; }

	uint32_t state = seed & ((1LLU << 29) - 1);
	uint32_t a = (4 * state) + 1;
	static unsigned long primes[] = {
		11, 101, 1009, 10007,
		100003, 1000003, 10000019, 100000007, 1000000007,
		1538461, 1865471, 17471, 2147483647 /* 2**32 - 1 */, };
	uint32_t c = primes[(state * 16451) % sizeof(primes)/sizeof(primes[0])];

	size_t offset = 0;
	const uint32_t mask = mod - 1;
	while (offset < length) {
		do {
			state = ((a * state) + c) & mask;
		} while (state >= ceil);
		res[offset++] = state;
	}
	return res;
}
