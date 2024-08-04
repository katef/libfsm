#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include <re/re.h>

#include <assert.h>

#include "../../src/libfsm/minimise_test_oracle.h"

const char *test_cases[] = {
	"a(?:b*)",
	"(?:x|b)*",
	"x*b",
	"(?:abc|def)*",
	"(?:ab|cd|ef)*",
	"(?:a+|b)a+",
	"(?:a*ba)+",
	"(?:a|cd)+e?x",
	"-> 1 'a';",
	"(?:abc|def)+",
	"(?:abc|def)*",
	"(?:b|a*)",
	"a*b+",
	"a+b+",
	"[0-4]+c?2",
	"[0-4]+c+2",
	"(?:a|bb|ccc)*",
	"(?:a|bb)*",
	"[12]+c?2",
	"a*",
	"ab*c",			/* regression */
};
static const size_t test_case_count = sizeof(test_cases)/sizeof(test_cases[0]);

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

static int
check_minimisation(const char *pattern)
{
	struct re_err err;
	struct fsm *fsm;
	const size_t length = strlen(pattern);

	struct scanner s = {
		.str    = (const uint8_t *)pattern,
		.size   = length,
		.offset = 0
	};

	fsm = re_comp(RE_PCRE, scanner_next, &s, NULL, RE_MULTI, &err);
	assert(fsm != NULL);
	if (!fsm_determinise(fsm)) {
		return 0;
	}

	const long trim_res = fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE, NULL);
	assert(trim_res != -1);

	/* This needs a trimmed, capture-less DFA as input. */
	struct fsm *oracle_min = fsm_minimise_test_oracle(fsm);
	assert(oracle_min != NULL);

	const size_t expected_state_count = fsm_countstates(oracle_min);

	if (!fsm_minimise(fsm)) {
		return 0;
	}

	const size_t state_count_min = fsm_countstates(fsm);
	int res = 0;

	if (state_count_min == expected_state_count) {
		res = 1;
	} else {
		fprintf(stderr, "%s: failure for pattern '%s'f, expected %zu states when minimised, got %zu\n",
		    __func__, pattern, expected_state_count, state_count_min);

		fprintf(stderr, "== expected:\n");
		fsm_dump(stderr, oracle_min);

		fprintf(stderr, "== got:\n");
		fsm_dump(stderr, fsm);
	}

	fsm_free(oracle_min);
	fsm_free(fsm);
	return res;
}

int main(void)
{
	size_t pass = 0;
	size_t fail = 0;

	for (size_t t_i = 0; t_i < test_case_count; t_i++) {
		const char *tc = test_cases[t_i];
		if (check_minimisation(tc)) {
			printf("pass %s\n", tc);
			pass++;
		} else {
			printf("FAIL %s\n", tc);
			fail++;
		}
	}

	printf("%s: pass %zu, fail %zu\n", __FILE__, pass, fail);

	return fail == 0
	    ? EXIT_SUCCESS
	    : EXIT_FAILURE;
}
