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
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#include "test_helpers.h"

/* Deterministic xorshift64* PRNG for reproducible randomized tests */
static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static uint32_t
rng_u32(void)
{
	uint64_t x = rng_state;
	uint64_t lo, hi, mid;

	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	rng_state = x;

	lo = x & 0xffffffffULL;
	hi = x >> 32;
	mid = ((lo * 0x4f6cdd1dULL) >> 32)
	    + ((lo * 0x2545f491ULL) & 0xffffffffULL)
	    + ((hi * 0x4f6cdd1dULL) & 0xffffffffULL);
	return (uint32_t) (mid & 0xffffffffULL);
}

/*
 * Extracted table representation of a compiled leaf DFA so we can exhaustively
 * step millions of IPs in O(1) per byte (avoiding O(|V|) fsm_isdfa checks inside
 * fsm_exec on every probe), while still cross-checking against iprange_match().
 */
struct fast_dfa {
	unsigned nstates;
	fsm_state_t start;
	fsm_state_t *trans; /* nstates * 256 */
	int *is_end;
	unsigned *endid;
};

static int
fast_noop_eps(const struct fsm *fsm, fsm_state_t from, fsm_state_t to,
	void *opaque)
{
	(void) fsm;
	(void) from;
	(void) to;
	(void) opaque;
	return 1;
}

static int
fast_lit_cb(const struct fsm *fsm, fsm_state_t from, fsm_state_t to,
	char c, void *opaque)
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

	assert(fsm_walk_edges(fsm, d, fast_lit_cb, fast_noop_eps) == 1);
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

/*
 * Exhaustive 65,536-IP oracle over 10.0.[0..255].[0..255]
 * across 150 random sets of overlapping/nested/adjacent intervals.
 */
static void
test_exhaustive_single_label_oracle_64k(void)
{
	unsigned char *oracle = malloc(65536);
	assert(oracle != NULL);

	for (unsigned trial = 0; trial < 150; trial++) {
		unsigned nintervals = 1 + (rng_u32() % 14);
		struct iprange *ipr_raw = iprange_new(IPRANGE_IPV4);
		struct iprange *ipr_can = iprange_new(IPRANGE_IPV4);
		struct fsm *fsm_raw = NULL, *fsm_can = NULL;
		struct fast_dfa fdfa;

		assert(ipr_raw != NULL && ipr_can != NULL);
		memset(oracle, 0, 65536);

		for (unsigned k = 0; k < nintervals; k++) {
			uint32_t a, b;
			unsigned mode = rng_u32() % 5;

			if (mode == 0) {
				a = rng_u32() % 65536u;
				b = rng_u32() % 65536u;
			} else if (mode == 1) {
				a = (rng_u32() % 256u) << 8;
				b = ((rng_u32() % 256u) << 8) | 0xffu;
			} else if (mode == 2) {
				a = rng_u32() % 65536u;
				b = a + (rng_u32() % 600u);
				if (b > 65535u) {
					b = 65535u;
				}
			} else if (mode == 3) {
				a = b = rng_u32() % 65536u;
			} else {
				a = rng_u32() % 65280u;
				b = ((a >> 8) + 1u) << 8;
				if (b > 65535u) {
					b = 65535u;
				}
			}

			if (a > b) {
				uint32_t tmp = a;
				a = b;
				b = tmp;
			}

			for (uint32_t x = a; x <= b; x++) {
				oracle[x] = 1;
			}

			unsigned char s[4] = { 10, 0, (unsigned char) (a >> 8), (unsigned char) (a & 0xffu) };
			unsigned char e[4] = { 10, 0, (unsigned char) (b >> 8), (unsigned char) (b & 0xffu) };
			assert(iprange_add(ipr_raw, s, e, "net", NULL) == IPRANGE_OK);
		}

		/* Build merged canonical intervals from oracle[] */
		uint32_t idx = 0;
		while (idx < 65536u) {
			if (!oracle[idx]) {
				idx++;
				continue;
			}
			uint32_t end = idx;
			while (end + 1 < 65536u && oracle[end + 1]) {
				end++;
			}
			unsigned char s[4] = { 10, 0, (unsigned char) (idx >> 8), (unsigned char) (idx & 0xffu) };
			unsigned char e[4] = { 10, 0, (unsigned char) (end >> 8), (unsigned char) (end & 0xffu) };
			assert(iprange_add(ipr_can, s, e, "net", NULL) == IPRANGE_OK);
			idx = end + 1;
		}

		assert(iprange_compile(ipr_raw, &fsm_raw, NULL, NULL) == IPRANGE_OK);
		assert(iprange_compile(ipr_can, &fsm_can, NULL, NULL) == IPRANGE_OK);

		assert_valid_leaf_dfa(fsm_raw, IPRANGE_IPV4);
		assert_valid_leaf_dfa(fsm_can, IPRANGE_IPV4);
		assert(fsm_equal(fsm_raw, fsm_can) == 1);
		assert(fsm_countstates(fsm_raw) == fsm_countstates(fsm_can));
		assert(fsm_countedges(fsm_raw) == fsm_countedges(fsm_can));

		fast_dfa_init(&fdfa, fsm_raw);

		int all_64k = 1;
		int full_24[256];
		for (unsigned hi = 0; hi < 256u; hi++) {
			full_24[hi] = 1;
			for (unsigned lo = 0; lo < 256u; lo++) {
				if (!oracle[(hi << 8) | lo]) {
					full_24[hi] = 0;
					all_64k = 0;
				}
			}
		}

		/* Exhaustively verify all 65,536 IPs */
		for (uint32_t x = 0; x < 65536u; x++) {
			unsigned char addr[4] = {
				10, 0,
				(unsigned char) (x >> 8),
				(unsigned char) (x & 0xffu)
			};
			unsigned id = 0;
			size_t consumed = 0;
			int m = fast_dfa_match(&fdfa, addr, 4, &id, &consumed);

			assert(m == (int) oracle[x]);
			if (m == 1) {
				assert(id == 0);
				if (all_64k) {
					assert(consumed == 2);
				} else {
					assert(consumed == (full_24[x >> 8] ? 3u : 4u));
				}
			}
		}

		/* Also cross-check iprange_match() (fsm_exec) on 64 sampled IPs per trial */
		for (unsigned p = 0; p < 64; p++) {
			uint32_t x = rng_u32() % 65536u;
			unsigned char addr[4] = {
				10, 0,
				(unsigned char) (x >> 8),
				(unsigned char) (x & 0xffu)
			};
			assert_match_bytes(fsm_raw, ipr_raw, addr,
				oracle[x] ? "net" : NULL, 0);
		}

		assert_match_str(fsm_raw, ipr_raw, "10.1.0.0", NULL, 0);
		assert_match_str(fsm_raw, ipr_raw, "9.255.255.255", NULL, 0);

		fast_dfa_free(&fdfa);
		fsm_free(fsm_raw);
		fsm_free(fsm_can);
		iprange_free(ipr_raw);
		iprange_free(ipr_can);
	}

	free(oracle);
}

/*
 * Multi-label randomized oracle straddling 9.255.248.0 .. 10.0.7.255
 * (4096 IPs across the 9.255.255.255 -> 10.0.0.0 carry boundary).
 */
static void
test_multilabel_cross_octet_oracle(void)
{
	const uint32_t base = 0x09fff800u; /* 9.255.248.0 */
	const uint32_t span = 4096u;       /* through 10.0.7.255 */
	static const char * const names[4] = { "L0", "L1", "L2", "L3" };
	uint8_t mask[4096];

	for (unsigned trial = 0; trial < 250; trial++) {
		unsigned nintervals = 2 + (rng_u32() % 8);
		struct iprange *ipr = iprange_new(IPRANGE_IPV4);
		struct fsm *fsm = NULL;
		struct conflict_log log;
		int expect_conflict = 0;

		assert(ipr != NULL);
		memset(mask, 0, sizeof mask);
		memset(&log, 0, sizeof log);

		for (unsigned k = 0; k < nintervals; k++) {
			unsigned lbl = rng_u32() % 4;
			uint32_t a = rng_u32() % span;
			uint32_t b;

			if ((rng_u32() % 3) == 0) {
				b = a + (rng_u32() % 300u);
			} else {
				b = a + (rng_u32() % 1200u);
			}
			if (b >= span) {
				b = span - 1;
			}

			for (uint32_t x = a; x <= b; x++) {
				mask[x] |= (uint8_t) (1u << lbl);
			}

			unsigned char s[4], e[4];
			u32_to_octets(base + a, s);
			u32_to_octets(base + b, e);
			assert(iprange_add(ipr, s, e, names[lbl], NULL) == IPRANGE_OK);
		}

		for (uint32_t x = 0; x < span; x++) {
			uint8_t m = mask[x];
			if (m != 0 && (m & (m - 1)) != 0) {
				expect_conflict = 1;
				break;
			}
		}

		enum iprange_err err = iprange_compile(ipr, &fsm,
			record_conflict_cb, &log);

		if (expect_conflict) {
			assert(err == IPRANGE_ERR_CONFLICT);
			assert(fsm == NULL);
			assert(log.count >= 1);

			for (size_t i = 0; i < log.count; i++) {
				uint32_t ip32 = octets_to_u32(log.items[i].example_ip);
				assert(ip32 >= base && ip32 < base + span);
				uint8_t expected_bits = 0;
				for (size_t j = 0; j < log.items[i].count; j++) {
					unsigned lidx = (unsigned) (log.items[i].labels[j][1] - '0');
					expected_bits |= (uint8_t) (1u << lidx);
				}
				assert(mask[ip32 - base] == expected_bits);
				for (uint32_t y = 0; y < ip32 - base; y++) {
					assert(mask[y] != expected_bits);
				}
			}
		} else {
			struct fast_dfa fdfa;

			assert(err == IPRANGE_OK);
			assert(fsm != NULL);
			assert_valid_leaf_dfa(fsm, IPRANGE_IPV4);
			fast_dfa_init(&fdfa, fsm);

			for (uint32_t x = 0; x < span; x++) {
				unsigned char addr[4];
				unsigned id = 0;
				size_t consumed = 0;
				int m;

				u32_to_octets(base + x, addr);
				m = fast_dfa_match(&fdfa, addr, 4, &id, &consumed);
				if (mask[x] == 0) {
					assert(m == 0);
				} else {
					unsigned lbl = 0;
					while (((mask[x] >> lbl) & 1u) == 0) {
						lbl++;
					}
					assert(m == 1);
					assert(strcmp(iprange_label(ipr, id), names[lbl]) == 0);
				}
			}

			unsigned char before[4], after[4];
			u32_to_octets(base - 1, before);
			u32_to_octets(base + span, after);
			assert_match_bytes(fsm, ipr, before, NULL, 0);
			assert_match_bytes(fsm, ipr, after, NULL, 0);

			fast_dfa_free(&fdfa);
			fsm_free(fsm);
		}

		iprange_free(ipr);
	}
}

/*
 * Randomized IPv6 differential oracle over 2001:db8::[0..1023]
 */
static void
test_ipv6_random_oracle(void)
{
	static const char * const names[3] = { "V6_A", "V6_B", "V6_C" };
	uint8_t mask[1024];

	for (unsigned trial = 0; trial < 120; trial++) {
		unsigned nintervals = 1 + (rng_u32() % 8);
		struct iprange *ipr = iprange_new(IPRANGE_IPV6);
		struct fsm *fsm = NULL;
		int expect_conflict = 0;

		assert(ipr != NULL);
		memset(mask, 0, sizeof mask);

		for (unsigned k = 0; k < nintervals; k++) {
			unsigned lbl = (trial < 60) ? 0 : (rng_u32() % 3);
			uint32_t a = rng_u32() % 1024u;
			uint32_t b = a + (rng_u32() % 400u);
			if (b >= 1024u) {
				b = 1023u;
			}

			for (uint32_t x = a; x <= b; x++) {
				mask[x] |= (uint8_t) (1u << lbl);
			}

			unsigned char s[16] = { 0x20, 0x01, 0x0d, 0xb8, 0 };
			unsigned char e[16] = { 0x20, 0x01, 0x0d, 0xb8, 0 };
			s[14] = (unsigned char) (a >> 8);
			s[15] = (unsigned char) (a & 0xffu);
			e[14] = (unsigned char) (b >> 8);
			e[15] = (unsigned char) (b & 0xffu);
			assert(iprange_add(ipr, s, e, names[lbl], NULL) == IPRANGE_OK);
		}

		for (uint32_t x = 0; x < 1024u; x++) {
			uint8_t m = mask[x];
			if (m != 0 && (m & (m - 1)) != 0) {
				expect_conflict = 1;
				break;
			}
		}

		enum iprange_err err = iprange_compile(ipr, &fsm, NULL, NULL);
		if (expect_conflict) {
			assert(err == IPRANGE_ERR_CONFLICT);
			assert(fsm == NULL);
		} else {
			struct fast_dfa fdfa;

			assert(err == IPRANGE_OK);
			assert_valid_leaf_dfa(fsm, IPRANGE_IPV6);
			fast_dfa_init(&fdfa, fsm);

			for (uint32_t x = 0; x < 1024u; x++) {
				unsigned char addr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0 };
				unsigned id = 0;
				size_t consumed = 0;
				int m;

				addr[14] = (unsigned char) (x >> 8);
				addr[15] = (unsigned char) (x & 0xffu);
				m = fast_dfa_match(&fdfa, addr, 16, &id, &consumed);
				if (mask[x] == 0) {
					assert(m == 0);
				} else {
					unsigned lbl = 0;
					while (((mask[x] >> lbl) & 1u) == 0) {
						lbl++;
					}
					assert(m == 1);
					assert(strcmp(iprange_label(ipr, id), names[lbl]) == 0);
				}
			}

			fast_dfa_free(&fdfa);
			fsm_free(fsm);
		}

		iprange_free(ipr);
	}
}

static int
test_accept_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *md,
	void *lang_opaque, void *hook_opaque)
{
	const struct iprange *ipr = hook_opaque;
	(void) opt;
	(void) lang_opaque;
	assert(md->end_id_count == 1);
	fprintf(f, "return 0x%x; /* %s */",
		md->end_ids[0], iprange_label(ipr, md->end_ids[0]));
	return 0;
}

static int
test_reject_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *md,
	void *lang_opaque, void *hook_opaque)
{
	if (md != NULL && md->end_id_count > 0) {
		return test_accept_c(f, opt, md, lang_opaque, hook_opaque);
	}
	fprintf(f, "return -1;");
	return 0;
}

static void
test_print_backends(void)
{
	const struct range_spec specs[] = {
		{ "10.0.0.0",        "10.255.255.255",  "private" },
		{ "10.1.2.0",        "10.1.2.255",      "private" },
		{ "255.255.255.255", "255.255.255.255", "bcast" }
	};
	struct iprange *ipr;
	struct fsm *fsm = compile_specs(IPRANGE_IPV4, specs, 3, &ipr);
	struct fsm_options opt;
	struct fsm_hooks hooks;
	FILE *tmp;

	memset(&opt, 0, sizeof opt);
	memset(&hooks, 0, sizeof hooks);
	opt.ambig = AMBIG_ERROR;
	opt.always_hex = 1;
	opt.anonymous_states = 1;
	opt.consolidate_edges = 1;
	opt.case_ranges = 1;
	opt.fragment = 1;
	opt.comments = 1;
	hooks.accept = test_accept_c;
	hooks.reject = test_reject_c;
	hooks.hook_opaque = ipr;

	tmp = tmpfile();
	assert(tmp != NULL);
	assert(fsm_print(tmp, fsm, &opt, &hooks, FSM_PRINT_C) == 0);
	assert(ftell(tmp) > 0);
	fclose(tmp);

	memset(&hooks, 0, sizeof hooks);
	tmp = tmpfile();
	assert(tmp != NULL);
	assert(fsm_print(tmp, fsm, &opt, &hooks, FSM_PRINT_DOT) == 0);
	assert(ftell(tmp) > 0);
	fclose(tmp);

	fsm_free(fsm);
	iprange_free(ipr);
}

int
main(void)
{
	test_exhaustive_single_label_oracle_64k();
	test_multilabel_cross_octet_oracle();
	test_ipv6_random_oracle();
	test_print_backends();
	return EXIT_SUCCESS;
}
