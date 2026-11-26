#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "test_helpers.h"

#define ARR_LEN(a) (sizeof (a) / sizeof *(a))

static void
test_exact_duplicates(void)
{
	/* Duplicate /8, /24, /32, and arbitrary non-CIDR range */
	const struct range_spec dup[] = {
		{ "10.0.0.0",   "10.255.255.255", "priv" },
		{ "10.0.0.0",   "10.255.255.255", "priv" },
		{ "192.0.2.0",  "192.0.2.255",    "doc" },
		{ "192.0.2.0",  "192.0.2.255",    "doc" },
		{ "192.0.2.0",  "192.0.2.255",    "doc" },
		{ "1.2.3.4",    "1.2.3.4",        "host" },
		{ "1.2.3.4",    "1.2.3.4",        "host" },
		{ "20.1.5.10",  "22.3.9.200",     "arb" },
		{ "20.1.5.10",  "22.3.9.200",     "arb" }
	};
	const struct range_spec canon[] = {
		{ "10.0.0.0",   "10.255.255.255", "priv" },
		{ "192.0.2.0",  "192.0.2.255",    "doc" },
		{ "1.2.3.4",    "1.2.3.4",        "host" },
		{ "20.1.5.10",  "22.3.9.200",     "arb" }
	};

	assert_equivalent_specs(IPRANGE_IPV4,
		dup, ARR_LEN(dup), canon, ARR_LEN(canon));
}

static void
test_containment_overlaps(void)
{
	/* Outer /8 followed by inner /16, /24, /32, and non-CIDR */
	{
		const struct range_spec outer_first[] = {
			{ "10.0.0.0",   "10.255.255.255", "net" },
			{ "10.1.0.0",   "10.1.255.255",   "net" },
			{ "10.1.2.0",   "10.1.2.255",     "net" },
			{ "10.1.2.3",   "10.1.2.3",       "net" },
			{ "10.5.7.9",   "10.42.99.200",   "net" }
		};
		const struct range_spec inner_first[] = {
			{ "10.1.2.3",   "10.1.2.3",       "net" },
			{ "10.5.7.9",   "10.42.99.200",   "net" },
			{ "10.1.2.0",   "10.1.2.255",     "net" },
			{ "10.1.0.0",   "10.1.255.255",   "net" },
			{ "10.0.0.0",   "10.255.255.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",   "10.255.255.255", "net" }
		};

		assert_equivalent_specs(IPRANGE_IPV4,
			outer_first, ARR_LEN(outer_first), canon, ARR_LEN(canon));
		assert_equivalent_specs(IPRANGE_IPV4,
			inner_first, ARR_LEN(inner_first), canon, ARR_LEN(canon));

		/* Verify 10.1.2.99 still matches "net" in 1 byte! */
		struct iprange *ipr;
		struct fsm *fsm = compile_specs(IPRANGE_IPV4,
			outer_first, ARR_LEN(outer_first), &ipr);
		assert(fsm_countstates(fsm) == 2);
		assert_match_str(fsm, ipr, "10.1.2.99", "net", 1);
		assert_match_str(fsm, ipr, "10.1.2.3", "net", 1);
		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* Boundary-sharing containment (shared start and shared end) */
	{
		const struct range_spec shared_bounds[] = {
			{ "10.0.0.0",     "10.255.255.255", "net" },
			{ "10.0.0.0",     "10.0.0.0",       "net" },
			{ "10.0.0.0",     "10.0.255.255",   "net" },
			{ "10.255.0.0",   "10.255.255.255", "net" },
			{ "10.255.255.255", "10.255.255.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",     "10.255.255.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4,
			shared_bounds, ARR_LEN(shared_bounds), canon, ARR_LEN(canon));
	}

	/* Non-CIDR arbitrary range containing another non-CIDR range */
	{
		const struct range_spec nested[] = {
			{ "2.3.4.5", "4.5.6.7", "arb" },
			{ "1.2.3.4", "5.6.7.8", "arb" },
			{ "1.2.3.4", "1.2.3.99", "arb" },
			{ "5.6.7.0", "5.6.7.8", "arb" }
		};
		const struct range_spec canon[] = {
			{ "1.2.3.4", "5.6.7.8", "arb" }
		};
		assert_equivalent_specs(IPRANGE_IPV4,
			nested, ARR_LEN(nested), canon, ARR_LEN(canon));
	}
}

static void
test_kissing_single_point_overlaps(void)
{
	/* A.end == B.start within last octet -> collapses to /24 (3 bytes) */
	{
		const struct range_spec kiss3[] = {
			{ "10.0.0.0",   "10.0.0.128", "net" },
			{ "10.0.0.128", "10.0.0.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",   "10.0.0.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4,
			kiss3, ARR_LEN(kiss3), canon, ARR_LEN(canon));
	}

	/* A.end == B.start across /24 boundary -> collapses to /23 */
	{
		const struct range_spec kiss2[] = {
			{ "10.0.0.0", "10.0.1.0",   "net" },
			{ "10.0.1.0", "10.0.1.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0", "10.0.1.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4,
			kiss2, ARR_LEN(kiss2), canon, ARR_LEN(canon));
	}

	/* A.end == B.start across /8 interior -> collapses to 10.0.0.0/8 (2 states!) */
	{
		const struct range_spec kiss1[] = {
			{ "10.128.0.0", "10.255.255.255", "net" },
			{ "10.0.0.0",   "10.128.0.0",     "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",   "10.255.255.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4,
			kiss1, ARR_LEN(kiss1), canon, ARR_LEN(canon));
	}
}

static void
test_partial_overlaps(void)
{
	/* Partial overlap within final octet */
	{
		const struct range_spec p1[] = {
			{ "10.0.0.10", "10.0.0.100", "net" },
			{ "10.0.0.50", "10.0.0.200", "net" }
		};
		const struct range_spec p1_rev[] = {
			{ "10.0.0.50", "10.0.0.200", "net" },
			{ "10.0.0.10", "10.0.0.100", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.10", "10.0.0.200", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, p1, 2, canon, 1);
		assert_equivalent_specs(IPRANGE_IPV4, p1_rev, 2, canon, 1);
	}

	/* Partial overlap crossing 3rd/4th octet boundary */
	{
		const struct range_spec p2[] = {
			{ "10.0.0.100", "10.0.2.50",  "net" },
			{ "10.0.1.200", "10.0.3.10",  "net" }
		};
		const struct range_spec p2_rev[] = {
			{ "10.0.1.200", "10.0.3.10",  "net" },
			{ "10.0.0.100", "10.0.2.50",  "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.100", "10.0.3.10",  "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, p2, 2, canon, 1);
		assert_equivalent_specs(IPRANGE_IPV4, p2_rev, 2, canon, 1);
	}

	/* Partial overlap crossing all 4 octets */
	{
		const struct range_spec p4[] = {
			{ "1.200.100.50", "3.10.20.30", "net" },
			{ "2.50.60.70",   "4.0.0.5",    "net" }
		};
		const struct range_spec canon[] = {
			{ "1.200.100.50", "4.0.0.5",    "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, p4, 2, canon, 1);
	}

	/* Partial overlap completing a /8 from two ugly non-CIDR intervals */
	{
		const struct range_spec ugly[] = {
			{ "10.0.0.0",    "10.180.12.34",   "net" },
			{ "10.90.56.78", "10.255.255.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",    "10.255.255.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, ugly, 2, canon, 1);
	}
}

static void
test_adjacent_merges(void)
{
	/* Two /9s -> /8 (2 states, 1 byte) */
	{
		const struct range_spec halves[] = {
			{ "10.128.0.0", "10.255.255.255", "net" },
			{ "10.0.0.0",   "10.127.255.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",   "10.255.255.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, halves, 2, canon, 1);
	}

	/* Arbitrary non-CIDR split point inside /8 -> collapses back to /8 */
	{
		const struct range_spec split[] = {
			{ "10.0.0.0",      "10.37.99.211",   "net" },
			{ "10.37.99.212",  "10.255.255.255", "net" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.0",      "10.255.255.255", "net" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, split, 2, canon, 1);
	}

	/* Entire IPv4 space split into 3 adjacent/overlapping pieces -> 2 states */
	{
		const struct range_spec all_split[] = {
			{ "100.0.0.0", "200.12.34.56",    "all" },
			{ "0.0.0.0",   "120.5.6.7",       "all" },
			{ "200.12.34.57", "255.255.255.255", "all" }
		};
		const struct range_spec canon[] = {
			{ "0.0.0.0",   "255.255.255.255", "all" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, all_split, 3, canon, 1);
	}
}

static void
test_multi_range_and_multi_record_overlaps(void)
{
	/* Bridge range closing gap between two intervals to complete a /24 */
	{
		const struct range_spec bridge[] = {
			{ "192.168.1.0",   "192.168.1.50",  "lan" },
			{ "192.168.1.200", "192.168.1.255", "lan" },
			{ "192.168.1.40",  "192.168.1.210", "lan" }
		};
		const struct range_spec canon[] = {
			{ "192.168.1.0",   "192.168.1.255", "lan" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, bridge, 3, canon, 1);
	}

	/* Scrambled 6-interval staircase */
	{
		const struct range_spec stairs[] = {
			{ "10.0.2.10",  "10.0.3.100", "s" },
			{ "10.0.0.5",   "10.0.0.200", "s" },
			{ "10.0.4.0",   "10.0.5.50",  "s" },
			{ "10.0.0.150", "10.0.1.80",  "s" },
			{ "10.0.3.50",  "10.0.4.10",  "s" },
			{ "10.0.1.80",  "10.0.2.20",  "s" }
		};
		const struct range_spec canon[] = {
			{ "10.0.0.5",   "10.0.5.50",  "s" }
		};
		assert_equivalent_specs(IPRANGE_IPV4, stairs, 6, canon, 1);
	}

	/*
	 * Interleaved multi-record table with internal overlaps AND cross-depth
	 * equivalent suffix subtrees (1.0.* -> A, 1.[1..255].* -> B, AND
	 * 2.5.0.* -> A, 2.5.[1..255].* -> B): verifies bottom-up memoization
	 * merges equivalent states across different depths.
	 */
	{
		const struct range_spec messy[] = {
			{ "1.0.0.0",   "1.0.180.99",    "A" },
			{ "1.1.0.0",   "1.200.0.0",     "B" },
			{ "1.0.100.0", "1.0.255.255",   "A" },
			{ "1.150.5.5", "1.255.255.255", "B" },
			{ "2.5.0.0",   "2.5.0.200",     "A" },
			{ "2.5.1.0",   "2.5.128.0",     "B" },
			{ "2.5.0.100", "2.5.0.255",     "A" },
			{ "2.5.100.0", "2.5.255.255",   "B" }
		};
		const struct range_spec canon[] = {
			{ "1.0.0.0",   "1.0.255.255",   "A" },
			{ "1.1.0.0",   "1.255.255.255", "B" },
			{ "2.5.0.0",   "2.5.0.255",     "A" },
			{ "2.5.1.0",   "2.5.255.255",   "B" }
		};
		assert_equivalent_specs(IPRANGE_IPV4,
			messy, ARR_LEN(messy), canon, ARR_LEN(canon));

		/* Verify state sharing: start, S_after_2, S_shared_decision, End_A, End_B = 5 states! */
		struct iprange *ipr;
		struct fsm *fsm = compile_specs(IPRANGE_IPV4,
			messy, ARR_LEN(messy), &ipr);
		assert(fsm_countstates(fsm) == 5);
		assert_match_str(fsm, ipr, "1.0.99.99", "A", 2);
		assert_match_str(fsm, ipr, "1.42.99.99", "B", 2);
		assert_match_str(fsm, ipr, "2.5.0.99", "A", 3);
		assert_match_str(fsm, ipr, "2.5.42.99", "B", 3);
		assert_match_str(fsm, ipr, "2.4.0.1", NULL, 0);
		fsm_free(fsm);
		iprange_free(ipr);
	}
}

static void
test_ipv6_same_label_overlaps(void)
{
	/* Nested + partial + adjacent IPv6 ranges collapsing to 2001:db8::/32 (4 bytes) */
	const struct range_spec v6_messy[] = {
		{ "2001:db8:0:0:1::", "2001:db8:0:0:1::ffff", "doc" },
		{ "2001:db8::",       "2001:db8:9000::1234",  "doc" },
		{ "2001:db8:4000::",  "2001:db8:fffe:ffff:ffff:ffff:ffff:ffff", "doc" },
		{ "2001:db8:ffff::",  "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff", "doc" },
		{ "2001:db8::1",      "2001:db8::1",          "doc" }
	};
	const struct range_spec v6_canon[] = {
		{ "2001:db8::", "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff", "doc" }
	};

	assert_equivalent_specs(IPRANGE_IPV6,
		v6_messy, ARR_LEN(v6_messy), v6_canon, ARR_LEN(v6_canon));

	/* Two adjacent /65s collapsing to a /64 (8 bytes) */
	{
		const struct range_spec halves65[] = {
			{ "2001:db8:1:2:8000::", "2001:db8:1:2:ffff:ffff:ffff:ffff", "s64" },
			{ "2001:db8:1:2::",      "2001:db8:1:2:7fff:ffff:ffff:ffff", "s64" }
		};
		const struct range_spec canon64[] = {
			{ "2001:db8:1:2::",      "2001:db8:1:2:ffff:ffff:ffff:ffff", "s64" }
		};
		assert_equivalent_specs(IPRANGE_IPV6, halves65, 2, canon64, 1);

		struct iprange *ipr;
		struct fsm *fsm = compile_specs(IPRANGE_IPV6, halves65, 2, &ipr);
		assert_match_str(fsm, ipr, "2001:db8:1:2::1", "s64", 8);
		assert_match_str(fsm, ipr, "2001:db8:1:2:ffff::1", "s64", 8);
		assert_match_str(fsm, ipr, "2001:db8:1:3::1", NULL, 0);
		fsm_free(fsm);
		iprange_free(ipr);
	}
}

int
main(void)
{
	test_exact_duplicates();
	test_containment_overlaps();
	test_kissing_single_point_overlaps();
	test_partial_overlaps();
	test_adjacent_merges();
	test_multi_range_and_multi_record_overlaps();
	test_ipv6_same_label_overlaps();
	return EXIT_SUCCESS;
}
