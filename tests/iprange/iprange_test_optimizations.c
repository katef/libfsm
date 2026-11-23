#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "test_helpers.h"

/*
 * Verify bottom-up state memoization deduplicates identical subtrees across
 * distinct prefixes and merges adjacent byte transitions to shared internal
 * states.
 */
static void
test_register_memo_cross_branch_dedup(void)
{
	static const struct range_spec specs[] = {
		{ "10.1.0.0",     "10.1.0.50",     "A" },
		{ "10.1.0.51",    "10.1.0.99",     "B" },
		{ "10.1.0.100",   "10.1.0.200",    "A" },

		{ "10.2.0.0",     "10.2.0.50",     "A" },
		{ "10.2.0.51",    "10.2.0.99",     "B" },
		{ "10.2.0.100",   "10.2.0.200",    "A" },

		{ "192.168.0.0",   "192.168.0.50",  "A" },
		{ "192.168.0.51",  "192.168.0.99",  "B" },
		{ "192.168.0.100", "192.168.0.200", "A" }
	};
	struct iprange *ipr = NULL;
	struct fsm *fsm;

	fsm = compile_specs(IPRANGE_IPV4, specs,
		sizeof specs / sizeof *specs, &ipr);

	/*
	 * Minimal state topology:
	 *   - 1 start state (0)
	 *   - 2 depth-1 states: under 10 ([1..2] -> d2) and under 192 ([168] -> d2)
	 *   - 1 shared depth-2 state: [0] -> d3
	 *   - 1 shared depth-3 state: [0..50]->A, [51..99]->B, [100..200]->A
	 *   - 2 leaf states: A, B
	 *   Total = 7 states.
	 */
	assert(fsm_countstates(fsm) == 7);

	assert_match_str(fsm, ipr, "10.1.0.0",     "A", 4);
	assert_match_str(fsm, ipr, "10.1.0.50",    "A", 4);
	assert_match_str(fsm, ipr, "10.1.0.51",    "B", 4);
	assert_match_str(fsm, ipr, "10.1.0.100",   "A", 4);
	assert_match_str(fsm, ipr, "10.1.0.201",   NULL, 0);
	assert_match_str(fsm, ipr, "10.2.0.75",    "B", 4);
	assert_match_str(fsm, ipr, "192.168.0.25", "A", 4);
	assert_match_str(fsm, ipr, "192.168.0.99", "B", 4);
	assert_match_str(fsm, ipr, "192.168.1.0",  NULL, 0);

	fsm_free(fsm);
	iprange_free(ipr);
}

/*
 * Exercise hash table rehashing, memoized state pool growth, edge run pool
 * growth, and > 400 unique labels across IPv4 ranges.
 */
static void
test_register_memo_rehash_many_labels(void)
{
	struct iprange *ipr;
	struct fsm *fsm = NULL;
	char label[32];

	ipr = iprange_new(IPRANGE_IPV4);
	assert(ipr != NULL);

	for (unsigned i = 0; i < 450; i++) {
		unsigned char s[4], e[4];

		s[0] = (unsigned char) (10u + (i / 64u));
		s[1] = (unsigned char) ((i % 64u) * 4u);
		s[2] = (unsigned char) (i % 13u);
		s[3] = (unsigned char) (1u + (i % 20u));

		memcpy(e, s, 4);
		e[2] = (unsigned char) (s[2] + 2u);
		e[3] = (unsigned char) (200u + (i % 40u));

		snprintf(label, sizeof label, "LBL_%03u", i);
		assert(iprange_add(ipr, s, e, label, NULL) == IPRANGE_OK);
	}

	assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
	assert(fsm != NULL);
	assert(iprange_record_count(ipr) == 450);
	assert_valid_leaf_dfa(fsm, IPRANGE_IPV4);

	for (unsigned i = 0; i < 450; i += 17) {
		unsigned char s[4], before[4];

		s[0] = (unsigned char) (10u + (i / 64u));
		s[1] = (unsigned char) ((i % 64u) * 4u);
		s[2] = (unsigned char) (i % 13u);
		s[3] = (unsigned char) (1u + (i % 20u));

		memcpy(before, s, 4);
		before[3]--;

		snprintf(label, sizeof label, "LBL_%03u", i);
		assert_match_bytes(fsm, ipr, s, label, 4);
		assert_match_bytes(fsm, ipr, before, NULL, 0);
	}

	fsm_free(fsm);
	iprange_free(ipr);
}

int
main(void)
{
	test_register_memo_cross_branch_dedup();
	test_register_memo_rehash_many_labels();
	return 0;
}
