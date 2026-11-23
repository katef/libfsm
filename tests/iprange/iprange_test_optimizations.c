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
	assert(iprange_enable_streaming(ipr) == IPRANGE_OK);

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

/*
 * Streaming ingestion: guardrails, empty stream, coalescing equivalence,
 * out-of-order rejection, cross-label conflict rejection, and CSV parity.
 */
static void
compile_csv_file(enum iprange_family family, const char *path, int streaming,
	struct fsm **fsm_out)
{
	FILE *f;
	struct iprange *ipr;
	char line[256];

	f = fopen(path, "r");
	assert(f != NULL);

	ipr = iprange_new(family);
	assert(ipr != NULL);
	if (streaming) {
		assert(iprange_enable_streaming(ipr) == IPRANGE_OK);
	}

	while (fgets(line, sizeof line, f) != NULL) {
		char *c1 = strchr(line, ',');
		char *c2;

		assert(c1 != NULL);
		*c1++ = '\0';
		c2 = strchr(c1, ',');
		assert(c2 != NULL);
		*c2++ = '\0';
		c2[strcspn(c2, "\r\n")] = '\0';

		assert(iprange_add_str(ipr, line, c1, c2, NULL) == IPRANGE_OK);
	}

	fclose(f);

	assert(iprange_compile(ipr, fsm_out, NULL, NULL) == IPRANGE_OK);
	assert(*fsm_out != NULL);
	assert_valid_leaf_dfa(*fsm_out, family);
	iprange_free(ipr);
}

static void
test_streaming_mode(void)
{
	/* Empty stream */
	{
		struct iprange *ipr = iprange_new(IPRANGE_IPV4);
		struct fsm *fsm = NULL;

		assert(iprange_enable_streaming(ipr) == IPRANGE_OK);
		assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
		assert(fsm_countstates(fsm) == 1);
		assert(fsm_countedges(fsm) == 0);
		assert_match_str(fsm, ipr, "0.0.0.0", NULL, 0);
		assert_match_str(fsm, ipr, "255.255.255.255", NULL, 0);
		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* Enabling streaming after adding ranges must fail */
	{
		struct iprange *ipr = iprange_new(IPRANGE_IPV4);

		assert(iprange_add_str(ipr, "1.0.0.0", "1.0.0.1", "A", NULL)
			== IPRANGE_OK);
		assert(iprange_enable_streaming(ipr) == IPRANGE_ERR_RANGE);
		iprange_free(ipr);
	}

	/* Out-of-order rejection, including after nested same-label range */
	{
		struct iprange *ipr = iprange_new(IPRANGE_IPV4);

		assert(iprange_enable_streaming(ipr) == IPRANGE_OK);
		assert(iprange_add_str(ipr, "10.0.0.0", "10.0.0.20", "A", NULL)
			== IPRANGE_OK);
		assert(iprange_add_str(ipr, "10.0.0.10", "10.0.0.15", "A", NULL)
			== IPRANGE_OK);
		assert(iprange_add_str(ipr, "10.0.0.5", "10.0.0.8", "A", NULL)
			== IPRANGE_ERR_ORDER);
		iprange_free(ipr);
	}

	/* Cross-label overlap rejection after nested narrower same-label range */
	{
		struct iprange *ipr = iprange_new(IPRANGE_IPV4);

		assert(iprange_enable_streaming(ipr) == IPRANGE_OK);
		assert(iprange_add_str(ipr, "10.0.0.0", "10.0.0.20", "A", NULL)
			== IPRANGE_OK);
		assert(iprange_add_str(ipr, "10.0.0.2", "10.0.0.5", "A", NULL)
			== IPRANGE_OK);
		assert(iprange_add_str(ipr, "10.0.0.15", "10.0.0.25", "B", NULL)
			== IPRANGE_ERR_CONFLICT);
		iprange_free(ipr);
	}

	/* Streaming coalescing produces identical minimal DFA to batch mode */
	{
		static const struct range_spec stream_specs[] = {
			{ "10.0.0.0",   "10.0.0.100", "A" },
			{ "10.0.0.50",  "10.0.0.127", "A" },
			{ "10.0.0.128", "10.0.0.255", "A" }, /* completes /24 */
			{ "10.0.1.0",   "10.0.1.255", "B" },
			{ "10.0.2.0",   "10.0.2.10",  "A" },
			{ "255.255.255.250", "255.255.255.255", "B" }
		};
		struct iprange *ipr_s = iprange_new(IPRANGE_IPV4);
		struct iprange *ipr_b = NULL;
		struct fsm *fsm_s = NULL, *fsm_b = NULL;

		assert(iprange_enable_streaming(ipr_s) == IPRANGE_OK);
		for (size_t i = 0; i < sizeof stream_specs / sizeof *stream_specs; i++) {
			assert(iprange_add_str(ipr_s, stream_specs[i].start,
				stream_specs[i].end, stream_specs[i].label, NULL)
				== IPRANGE_OK);
		}
		assert(iprange_compile(ipr_s, &fsm_s, NULL, NULL) == IPRANGE_OK);
		assert_valid_leaf_dfa(fsm_s, IPRANGE_IPV4);

		fsm_b = compile_specs(IPRANGE_IPV4, stream_specs,
			sizeof stream_specs / sizeof *stream_specs, &ipr_b);

		assert(fsm_countstates(fsm_s) == fsm_countstates(fsm_b));
		assert(fsm_countedges(fsm_s) == fsm_countedges(fsm_b));
		assert(fsm_equal(fsm_s, fsm_b) == 1);

		/* 10.0.0.0/24 must resolve at byte 3 */
		assert_match_str(fsm_s, ipr_s, "10.0.0.42", "A", 3);
		assert_match_str(fsm_s, ipr_s, "10.0.1.99", "B", 3);
		assert_match_str(fsm_s, ipr_s, "10.0.2.5",  "A", 4);
		assert_match_str(fsm_s, ipr_s, "255.255.255.255", "B", 4);

		fsm_free(fsm_s);
		fsm_free(fsm_b);
		iprange_free(ipr_s);
		iprange_free(ipr_b);
	}

	/* CSV parity on v4ranges.csv and v6ranges.csv */
	{
		struct fsm *v4_batch = NULL, *v4_stream = NULL;
		struct fsm *v6_batch = NULL, *v6_stream = NULL;

		compile_csv_file(IPRANGE_IPV4, "examples/iprange/v4ranges.csv",
			0, &v4_batch);
		compile_csv_file(IPRANGE_IPV4, "examples/iprange/v4ranges.csv",
			1, &v4_stream);
		assert(fsm_countstates(v4_batch) == fsm_countstates(v4_stream));
		assert(fsm_countedges(v4_batch) == fsm_countedges(v4_stream));
		assert(fsm_equal(v4_batch, v4_stream) == 1);
		fsm_free(v4_batch);
		fsm_free(v4_stream);

		compile_csv_file(IPRANGE_IPV6, "examples/iprange/v6ranges.csv",
			0, &v6_batch);
		compile_csv_file(IPRANGE_IPV6, "examples/iprange/v6ranges.csv",
			1, &v6_stream);
		assert(fsm_countstates(v6_batch) == fsm_countstates(v6_stream));
		assert(fsm_countedges(v6_batch) == fsm_countedges(v6_stream));
		assert(fsm_equal(v6_batch, v6_stream) == 1);
		fsm_free(v6_batch);
		fsm_free(v6_stream);
	}
}

int
main(void)
{
	test_register_memo_cross_branch_dedup();
	test_register_memo_rehash_many_labels();
	test_streaming_mode();
	return 0;
}
