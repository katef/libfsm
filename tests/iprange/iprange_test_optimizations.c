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

static uint64_t opt_rng = 0x517cc1b727220a95ULL;

static uint32_t
next_u32(void)
{
	uint64_t x = opt_rng;
	uint64_t lo, hi, mid;

	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	opt_rng = x;

	lo = x & 0xffffffffULL;
	hi = x >> 32;
	mid = ((lo * 0x4f6cdd1dULL) >> 32)
	    + ((lo * 0x2545f491ULL) & 0xffffffffULL)
	    + ((hi * 0x4f6cdd1dULL) & 0xffffffffULL);
	return (uint32_t) (mid & 0xffffffffULL);
}

struct fast_dfa {
	unsigned nstates;
	fsm_state_t start;
	fsm_state_t *trans;
	int *is_end;
	unsigned *endid;
};

static int
noop_eps(const struct fsm *fsm, fsm_state_t from, fsm_state_t to, void *opaque)
{
	(void) fsm;
	(void) from;
	(void) to;
	(void) opaque;
	return 1;
}

static int
lit_cb(const struct fsm *fsm, fsm_state_t from, fsm_state_t to, char c,
	void *opaque)
{
	struct fast_dfa *d = opaque;
	(void) fsm;
	d->trans[(size_t) from * 256u + (unsigned char) c] = to;
	return 1;
}

static void
fast_dfa_init(struct fast_dfa *d, const struct fsm *fsm)
{
	d->nstates = fsm_countstates(fsm);
	assert(fsm_getstart(fsm, &d->start) == 1);

	d->trans  = malloc((size_t) d->nstates * 256u * sizeof *d->trans);
	d->is_end = calloc(d->nstates, sizeof *d->is_end);
	d->endid  = calloc(d->nstates, sizeof *d->endid);
	assert(d->trans != NULL && d->is_end != NULL && d->endid != NULL);

	for (size_t i = 0; i < (size_t) d->nstates * 256u; i++) {
		d->trans[i] = (fsm_state_t) -1;
	}

	for (fsm_state_t s = 0; s < d->nstates; s++) {
		if (fsm_isend(fsm, s)) {
			fsm_end_id_t id = 0;
			assert(fsm_endid_count(fsm, s) == 1);
			assert(fsm_endid_get(fsm, s, 1, &id) == 1);
			d->is_end[s] = 1;
			d->endid[s]  = id;
		}
	}

	assert(fsm_walk_edges(fsm, d, lit_cb, noop_eps) == 1);
}

static void
fast_dfa_free(struct fast_dfa *d)
{
	free(d->trans);
	free(d->is_end);
	free(d->endid);
}

static int
fast_dfa_match(const struct fast_dfa *d, const unsigned char *addr,
	size_t nbytes, unsigned *id_out, size_t *consumed_out)
{
	fsm_state_t cur = d->start;

	for (size_t i = 0; i < nbytes; i++) {
		cur = d->trans[(size_t) cur * 256u + addr[i]];
		if (cur == (fsm_state_t) -1) {
			return 0;
		}
		if (d->is_end[cur]) {
			*id_out = d->endid[cur];
			*consumed_out = i + 1;
			return 1;
		}
	}
	return 0;
}

static void
sub_128(const unsigned char *end, const unsigned char *start,
	size_t nbytes, unsigned char *diff)
{
	unsigned borrow = 0;

	for (size_t i = nbytes; i > 0; i--) {
		int d = (int) end[i - 1] - (int) start[i - 1] - (int) borrow;
		if (d < 0) {
			d += 256;
			borrow = 1;
		} else {
			borrow = 0;
		}
		diff[i - 1] = (unsigned char) d;
	}
}

struct raw_rule {
	unsigned char start[16];
	unsigned char end[16];
	unsigned label_idx;
	unsigned assigned_id;
	size_t seq;
};

static int
rule_beats(const struct raw_rule *cand, const struct raw_rule *best,
	enum iprange_overlap_mode mode, size_t nbytes)
{
	if (mode == IPRANGE_OVERLAP_FIRST) {
		return cand->seq < best->seq;
	}
	if (mode == IPRANGE_OVERLAP_LAST) {
		return cand->seq > best->seq;
	}
	if (mode == IPRANGE_OVERLAP_MOST_SPECIFIC) {
		unsigned char sc[16], sb[16];
		int cmp;

		sub_128(cand->end, cand->start, nbytes, sc);
		sub_128(best->end, best->start, nbytes, sb);
		cmp = memcmp(sc, sb, nbytes);
		if (cmp != 0) {
			return cmp < 0;
		}
		return cand->seq > best->seq;
	}
	return 0;
}

static int
oracle_eval_ip(const struct raw_rule *rules, size_t nrules,
	enum iprange_overlap_mode mode, size_t nbytes,
	const unsigned char *ip, unsigned *win_id_out)
{
	const struct raw_rule *best = NULL;

	for (size_t i = 0; i < nrules; i++) {
		if (memcmp(ip, rules[i].start, nbytes) >= 0 &&
		    memcmp(ip, rules[i].end, nbytes) <= 0) {
			if (best == NULL ||
			    rule_beats(&rules[i], best, mode, nbytes)) {
				best = &rules[i];
			}
		}
	}

	if (best == NULL) {
		return 0;
	}
	*win_id_out = best->assigned_id;
	return 1;
}

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

	/* API mode combination guardrails */
	{
		struct iprange *ipr = iprange_new(IPRANGE_IPV4);

		assert(iprange_set_overlap_mode(ipr,
			IPRANGE_OVERLAP_MOST_SPECIFIC) == IPRANGE_OK);
		assert(iprange_enable_streaming(ipr) == IPRANGE_ERR_RANGE);
		assert(iprange_set_overlap_mode(ipr,
			IPRANGE_OVERLAP_ERROR) == IPRANGE_OK);
		assert(iprange_add_str(ipr, "1.0.0.0", "1.0.0.1", "A", NULL)
			== IPRANGE_OK);
		assert(iprange_enable_streaming(ipr) == IPRANGE_ERR_RANGE);
		iprange_free(ipr);

		ipr = iprange_new(IPRANGE_IPV4);
		assert(iprange_enable_streaming(ipr) == IPRANGE_OK);
		assert(iprange_set_overlap_mode(ipr, IPRANGE_OVERLAP_FIRST)
			== IPRANGE_ERR_RANGE);
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

static struct fsm *
compile_with_mode(enum iprange_family family, enum iprange_overlap_mode mode,
	const struct range_spec *specs, size_t n, struct iprange **ipr_out)
{
	struct iprange *ipr;
	struct fsm *fsm = NULL;

	ipr = iprange_new(family);
	assert(ipr != NULL);
	assert(iprange_set_overlap_mode(ipr, mode) == IPRANGE_OK);

	for (size_t i = 0; i < n; i++) {
		assert(iprange_add_str(ipr, specs[i].start, specs[i].end,
			specs[i].label, NULL) == IPRANGE_OK);
	}

	assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
	assert(fsm != NULL);
	assert_valid_leaf_dfa(fsm, family);

	*ipr_out = ipr;
	return fsm;
}

/*
 * Deterministic overlap-resolution tests: nested A->B->A->C hierarchies,
 * completely overshadowed labels (no orphan leaf states), equal-span tie
 * breaking, and adjacent same-label coalescing interaction with specificity.
 */
static void
test_overlap_modes_deterministic(void)
{
	static const struct range_spec hierarchy[] = {
		{ "10.0.0.0",  "10.255.255.255", "US" }, /* /8 */
		{ "10.1.0.0",  "10.1.255.255",   "CA" }, /* /16 */
		{ "10.1.2.0",  "10.1.2.255",     "US" }, /* /24 override back to US */
		{ "10.1.2.42", "10.1.2.42",      "MX" }  /* /32 */
	};
	static const struct range_spec hierarchy_rev[] = {
		{ "10.1.2.42", "10.1.2.42",      "MX" },
		{ "10.1.2.0",  "10.1.2.255",     "US" },
		{ "10.1.0.0",  "10.1.255.255",   "CA" },
		{ "10.0.0.0",  "10.255.255.255", "US" }
	};

	/* MOST_SPECIFIC: order-independent on distinct spans */
	{
		struct iprange *ipr1 = NULL, *ipr2 = NULL;
		struct fsm *f1, *f2;

		f1 = compile_with_mode(IPRANGE_IPV4, IPRANGE_OVERLAP_MOST_SPECIFIC,
			hierarchy, 4, &ipr1);
		f2 = compile_with_mode(IPRANGE_IPV4, IPRANGE_OVERLAP_MOST_SPECIFIC,
			hierarchy_rev, 4, &ipr2);

		assert(fsm_countstates(f1) == fsm_countstates(f2));
		assert(fsm_countedges(f1) == fsm_countedges(f2));

		assert_match_str(f1, ipr1, "10.1.2.42", "MX", 4);
		assert_match_str(f1, ipr1, "10.1.2.43", "US", 4);
		assert_match_str(f1, ipr1, "10.1.3.99", "CA", 3);
		assert_match_str(f1, ipr1, "10.2.9.9",  "US", 2);
		assert_match_str(f1, ipr1, "11.0.0.0",  NULL, 0);

		assert_match_str(f2, ipr2, "10.1.2.42", "MX", 4);
		assert_match_str(f2, ipr2, "10.1.2.43", "US", 4);
		assert_match_str(f2, ipr2, "10.1.3.99", "CA", 3);
		assert_match_str(f2, ipr2, "10.2.9.9",  "US", 2);

		fsm_free(f1);
		fsm_free(f2);
		iprange_free(ipr1);
		iprange_free(ipr2);
	}

	/* FIRST: 10.0.0.0/8 overshadows CA and MX completely -> 2-state /8 DFA */
	{
		struct iprange *ipr = NULL;
		struct fsm *fsm;

		fsm = compile_with_mode(IPRANGE_IPV4, IPRANGE_OVERLAP_FIRST,
			hierarchy, 4, &ipr);
		assert(fsm_countstates(fsm) == 2);
		assert_match_str(fsm, ipr, "10.1.2.42", "US", 1);
		assert_match_str(fsm, ipr, "10.1.3.0",  "US", 1);

		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* LAST on reversed hierarchy: final /8 US overshadows MX and CA -> 2 states */
	{
		struct iprange *ipr = NULL;
		struct fsm *fsm;

		fsm = compile_with_mode(IPRANGE_IPV4, IPRANGE_OVERLAP_LAST,
			hierarchy_rev, 4, &ipr);
		assert(fsm_countstates(fsm) == 2);
		assert_match_str(fsm, ipr, "10.1.2.42", "US", 1);

		fsm_free(fsm);
		iprange_free(ipr);
	}

	/*
	 * Adjacent same-label rules must NOT merge before specificity comparison:
	 *   Rule 0: 10.0.0.0..9   -> A (span 9)
	 *   Rule 1: 10.0.0.10..19 -> A (span 9)
	 *   Rule 2: 10.0.0.10..24 -> B (span 14)
	 * On 10..19, Rule 1 (span 9) beats Rule 2 (span 14), so 0..19 -> A and
	 * 20..24 -> B.
	 */
	{
		static const struct range_spec adj_spec[] = {
			{ "10.0.0.0",  "10.0.0.9",  "A" },
			{ "10.0.0.10", "10.0.0.19", "A" },
			{ "10.0.0.10", "10.0.0.24", "B" }
		};
		struct iprange *ipr = NULL;
		struct fsm *fsm;

		fsm = compile_with_mode(IPRANGE_IPV4,
			IPRANGE_OVERLAP_MOST_SPECIFIC, adj_spec, 3, &ipr);
		assert_match_str(fsm, ipr, "10.0.0.5",  "A", 4);
		assert_match_str(fsm, ipr, "10.0.0.10", "A", 4);
		assert_match_str(fsm, ipr, "10.0.0.19", "A", 4);
		assert_match_str(fsm, ipr, "10.0.0.20", "B", 4);
		assert_match_str(fsm, ipr, "10.0.0.24", "B", 4);
		assert_match_str(fsm, ipr, "10.0.0.25", NULL, 0);

		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* Equal-span tie breaking + 255.255.255.255 upper boundary */
	{
		static const struct range_spec boundary_spec[] = {
			{ "255.255.255.240", "255.255.255.250", "A" }, /* span 10 */
			{ "255.255.255.245", "255.255.255.255", "B" }  /* span 10 */
		};
		struct iprange *ipr_ms = NULL, *ipr_fi = NULL;
		struct fsm *f_ms, *f_fi;

		f_ms = compile_with_mode(IPRANGE_IPV4,
			IPRANGE_OVERLAP_MOST_SPECIFIC, boundary_spec, 2, &ipr_ms);
		assert_match_str(f_ms, ipr_ms, "255.255.255.244", "A", 4);
		assert_match_str(f_ms, ipr_ms, "255.255.255.245", "B", 4);
		assert_match_str(f_ms, ipr_ms, "255.255.255.255", "B", 4);

		f_fi = compile_with_mode(IPRANGE_IPV4,
			IPRANGE_OVERLAP_FIRST, boundary_spec, 2, &ipr_fi);
		assert_match_str(f_fi, ipr_fi, "255.255.255.250", "A", 4);
		assert_match_str(f_fi, ipr_fi, "255.255.255.251", "B", 4);
		assert_match_str(f_fi, ipr_fi, "255.255.255.255", "B", 4);

		fsm_free(f_ms);
		fsm_free(f_fi);
		iprange_free(ipr_ms);
		iprange_free(ipr_fi);
	}

	/* Full-space 0.0.0.0/0 and ::/0 catch-all fallback with subnet overrides */
	{
		static const struct range_spec v4_catchall[] = {
			{ "0.0.0.0",     "255.255.255.255", "WORLD" },
			{ "10.0.0.0",    "10.255.255.255",  "RFC1918" },
			{ "192.168.0.0", "192.168.255.255", "RFC1918" },
			{ "192.168.1.1", "192.168.1.1",     "GW" }
		};
		static const struct range_spec v6_catchall[] = {
			{ "::",        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "WORLD6" },
			{ "2001:db8::", "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff", "DOC6" }
		};
		struct iprange *ipr4 = NULL, *ipr6 = NULL;
		struct fsm *f4, *f6;

		f4 = compile_with_mode(IPRANGE_IPV4,
			IPRANGE_OVERLAP_MOST_SPECIFIC, v4_catchall, 4, &ipr4);
		assert_match_str(f4, ipr4, "1.2.3.4",         "WORLD",   1);
		assert_match_str(f4, ipr4, "10.99.88.77",     "RFC1918", 1);
		assert_match_str(f4, ipr4, "192.167.255.255", "WORLD",   2);
		assert_match_str(f4, ipr4, "192.168.0.5",     "RFC1918", 3);
		assert_match_str(f4, ipr4, "192.168.1.1",     "GW",      4);
		assert_match_str(f4, ipr4, "192.168.1.2",     "RFC1918", 4);
		assert_match_str(f4, ipr4, "192.169.0.0",     "WORLD",   2);
		assert_match_str(f4, ipr4, "255.255.255.255", "WORLD",   1);

		f6 = compile_with_mode(IPRANGE_IPV6,
			IPRANGE_OVERLAP_MOST_SPECIFIC, v6_catchall, 2, &ipr6);
		assert_match_str(f6, ipr6, "::1",              "WORLD6", 1);
		assert_match_str(f6, ipr6, "2001:db8::1",      "DOC6",   4);
		assert_match_str(f6, ipr6, "2001:db9::1",      "WORLD6", 4);
		assert_match_str(f6, ipr6, "ffff:ffff::1",     "WORLD6", 1);

		fsm_free(f4);
		fsm_free(f6);
		iprange_free(ipr4);
		iprange_free(ipr6);
	}
}

/*
 * Exhaustive 65,536-IP subspace oracle across randomized overlapping IPv4
 * rule sets for MOST_SPECIFIC, FIRST, and LAST, checking both winning label
 * and exact shortest-prefix byte consumption.
 */
static void
test_overlap_oracle_v4(void)
{
	static const enum iprange_overlap_mode modes[3] = {
		IPRANGE_OVERLAP_MOST_SPECIFIC,
		IPRANGE_OVERLAP_FIRST,
		IPRANGE_OVERLAP_LAST
	};
	static const char * const names[5] = { "L0", "L1", "L2", "L3", "L4" };
	unsigned *win = malloc(65536u * sizeof *win);
	assert(win != NULL);

	for (unsigned trial = 0; trial < 60; trial++) {
		size_t nrules = 3u + (next_u32() % 14u);
		struct raw_rule rules[20];
		unsigned char p0 = (trial % 5 == 0) ? 255 : (unsigned char) (next_u32() & 0xffu);
		unsigned char p1 = (trial % 5 == 0) ? 255 : (unsigned char) (next_u32() & 0xffu);

		for (size_t r = 0; r < nrules; r++) {
			uint16_t a = (uint16_t) (next_u32() & 0xffffu);
			uint16_t b = (uint16_t) (next_u32() & 0xffffu);

			if ((next_u32() % 4u) == 0) {
				uint16_t span = (uint16_t) (next_u32() % 600u);
				b = (a + span > 0xffffu) ? 0xffffu : (uint16_t) (a + span);
			}
			if (a > b) {
				uint16_t t = a;
				a = b;
				b = t;
			}

			memset(&rules[r], 0, sizeof rules[r]);
			rules[r].start[0] = p0;
			rules[r].start[1] = p1;
			rules[r].start[2] = (unsigned char) (a >> 8);
			rules[r].start[3] = (unsigned char) (a & 0xffu);

			rules[r].end[0] = p0;
			rules[r].end[1] = p1;
			rules[r].end[2] = (unsigned char) (b >> 8);
			rules[r].end[3] = (unsigned char) (b & 0xffu);

			rules[r].label_idx = next_u32() % 5u;
			rules[r].seq = r;
		}

		for (size_t m = 0; m < 3; m++) {
			enum iprange_overlap_mode mode = modes[m];
			struct iprange *ipr = iprange_new(IPRANGE_IPV4);
			struct fsm *fsm = NULL;
			struct fast_dfa dfa;
			int all_same_subspace;

			assert(ipr != NULL);
			assert(iprange_set_overlap_mode(ipr, mode) == IPRANGE_OK);

			for (size_t r = 0; r < nrules; r++) {
				assert(iprange_add(ipr, rules[r].start, rules[r].end,
					names[rules[r].label_idx],
					&rules[r].assigned_id) == IPRANGE_OK);
			}

			assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
			assert_valid_leaf_dfa(fsm, IPRANGE_IPV4);
			fast_dfa_init(&dfa, fsm);

			for (uint32_t x = 0; x < 65536u; x++) {
				unsigned char ip[4] = {
					p0, p1,
					(unsigned char) (x >> 8),
					(unsigned char) (x & 0xffu)
				};
				unsigned wid = 0;
				if (oracle_eval_ip(rules, nrules, mode, 4, ip, &wid)) {
					win[x] = wid;
				} else {
					win[x] = 0xffffffffu;
				}
			}

			all_same_subspace = (win[0] != 0xffffffffu);
			for (uint32_t x = 1; all_same_subspace && x < 65536u; x++) {
				if (win[x] != win[0]) {
					all_same_subspace = 0;
				}
			}

			for (uint32_t x = 0; x < 65536u; x++) {
				unsigned char ip[4] = {
					p0, p1,
					(unsigned char) (x >> 8),
					(unsigned char) (x & 0xffu)
				};
				unsigned got_id = 0;
				size_t got_bytes = 0;
				int matched = fast_dfa_match(&dfa, ip, 4,
					&got_id, &got_bytes);

				if (win[x] == 0xffffffffu) {
					assert(matched == 0);
				} else {
					size_t exp_bytes = 4;
					uint32_t blk_base = x & 0xff00u;
					int uniform_24 = 1;

					assert(matched == 1);
					assert(got_id == win[x]);

					if (all_same_subspace) {
						exp_bytes = 2;
					} else {
						for (uint32_t y = blk_base; y < blk_base + 256u; y++) {
							if (win[y] != win[x]) {
								uniform_24 = 0;
								break;
							}
						}
						if (uniform_24) {
							exp_bytes = 3;
						}
					}
					assert(got_bytes == exp_bytes);
				}
			}

			/* Outside p0.p1/16 must not match */
			{
				unsigned char out_ip[4] = { (unsigned char) (p0 ^ 1u), p1, 0, 0 };
				unsigned dummy_id = 0;
				size_t dummy_b = 0;
				assert(fast_dfa_match(&dfa, out_ip, 4, &dummy_id, &dummy_b) == 0);
			}

			fast_dfa_free(&dfa);
			fsm_free(fsm);
			iprange_free(ipr);
		}
	}

	free(win);
}

/*
 * Randomized IPv6 overlap-resolution oracle across all three resolution modes.
 */
static void
test_overlap_oracle_v6(void)
{
	static const enum iprange_overlap_mode modes[3] = {
		IPRANGE_OVERLAP_MOST_SPECIFIC,
		IPRANGE_OVERLAP_FIRST,
		IPRANGE_OVERLAP_LAST
	};
	static const char * const names[4] = { "V6_A", "V6_B", "V6_C", "V6_D" };

	for (unsigned trial = 0; trial < 40; trial++) {
		size_t nrules = 3u + (next_u32() % 10u);
		struct raw_rule rules[16];
		unsigned char base[16];

		for (size_t b = 0; b < 14; b++) {
			base[b] = (trial % 4 == 0) ? 0xffu : (unsigned char) (next_u32() & 0xffu);
		}
		base[14] = 0;
		base[15] = 0;

		for (size_t r = 0; r < nrules; r++) {
			uint16_t a = (uint16_t) (next_u32() & 0xffffu);
			uint16_t b = (uint16_t) (next_u32() & 0xffffu);
			if (a > b) {
				uint16_t t = a;
				a = b;
				b = t;
			}

			memset(&rules[r], 0, sizeof rules[r]);
			memcpy(rules[r].start, base, 14);
			rules[r].start[14] = (unsigned char) (a >> 8);
			rules[r].start[15] = (unsigned char) (a & 0xffu);

			memcpy(rules[r].end, base, 14);
			rules[r].end[14] = (unsigned char) (b >> 8);
			rules[r].end[15] = (unsigned char) (b & 0xffu);

			rules[r].label_idx = next_u32() % 4u;
			rules[r].seq = r;
		}

		for (size_t m = 0; m < 3; m++) {
			enum iprange_overlap_mode mode = modes[m];
			struct iprange *ipr = iprange_new(IPRANGE_IPV6);
			struct fsm *fsm = NULL;
			struct fast_dfa dfa;

			assert(ipr != NULL);
			assert(iprange_set_overlap_mode(ipr, mode) == IPRANGE_OK);

			for (size_t r = 0; r < nrules; r++) {
				assert(iprange_add(ipr, rules[r].start, rules[r].end,
					names[rules[r].label_idx],
					&rules[r].assigned_id) == IPRANGE_OK);
			}

			assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
			assert_valid_leaf_dfa(fsm, IPRANGE_IPV6);
			fast_dfa_init(&dfa, fsm);

			/* Check all rule boundaries +/- 1 and 512 random probes */
			for (size_t r = 0; r < nrules; r++) {
				unsigned char probes[4][16];
				size_t np = 0;

				memcpy(probes[np++], rules[r].start, 16);
				memcpy(probes[np++], rules[r].end, 16);
				memcpy(probes[np], rules[r].start, 16);
				if (ip_dec(probes[np], 16)) {
					np++;
				}
				memcpy(probes[np], rules[r].end, 16);
				if (ip_inc(probes[np], 16)) {
					np++;
				}

				for (size_t p = 0; p < np; p++) {
					unsigned exp_id = 0, got_id = 0;
					size_t got_b = 0;
					int exp_m = oracle_eval_ip(rules, nrules, mode,
						16, probes[p], &exp_id);
					int got_m = fast_dfa_match(&dfa, probes[p], 16,
						&got_id, &got_b);
					assert(exp_m == got_m);
					if (exp_m) {
						assert(got_id == exp_id);
					}
				}
			}

			for (unsigned q = 0; q < 512; q++) {
				unsigned char ip[16];
				uint16_t tail = (uint16_t) (next_u32() & 0xffffu);
				unsigned exp_id = 0, got_id = 0;
				size_t got_b = 0;
				int exp_m, got_m;

				memcpy(ip, base, 14);
				ip[14] = (unsigned char) (tail >> 8);
				ip[15] = (unsigned char) (tail & 0xffu);

				exp_m = oracle_eval_ip(rules, nrules, mode, 16,
					ip, &exp_id);
				got_m = fast_dfa_match(&dfa, ip, 16, &got_id, &got_b);
				assert(exp_m == got_m);
				if (exp_m) {
					assert(got_id == exp_id);
				}
			}

			fast_dfa_free(&dfa);
			fsm_free(fsm);
			iprange_free(ipr);
		}
	}
}

int
main(void)
{
	test_register_memo_cross_branch_dedup();
	test_register_memo_rehash_many_labels();
	test_streaming_mode();
	test_overlap_modes_deterministic();
	test_overlap_oracle_v4();
	test_overlap_oracle_v6();
	return 0;
}
