#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>

#include "test_helpers.h"

#define ARR_LEN(a) (sizeof (a) / sizeof *(a))

static void
assert_two_way_conflict(enum iprange_family family,
	const struct range_spec *specs, size_t n,
	const char *label_a, const char *label_b,
	const char *expected_smallest_ip)
{
	struct iprange *ipr;
	struct fsm *fsm = (struct fsm *) 0x1;
	struct conflict_log log;
	unsigned char exp_ip[16];
	size_t nbytes = (size_t) family;

	memset(&log, 0, sizeof log);
	ipr = iprange_new(family);
	assert(ipr != NULL);

	for (size_t i = 0; i < n; i++) {
		assert(iprange_add_str(ipr, specs[i].start, specs[i].end,
			specs[i].label, NULL) == IPRANGE_OK);
	}

	assert(iprange_compile(ipr, &fsm, record_conflict_cb, &log) ==
		IPRANGE_ERR_CONFLICT);
	assert(fsm == NULL);
	assert(log.count == 1);
	assert(log.items[0].count == 2);
	assert(conflict_has_label(&log.items[0], label_a));
	assert(conflict_has_label(&log.items[0], label_b));

	/* Verify example_ip actually lies in both label_a and label_b */
	assert(spec_set_contains_ip_for_label(family, specs, n,
		label_a, log.items[0].example_ip));
	assert(spec_set_contains_ip_for_label(family, specs, n,
		label_b, log.items[0].example_ip));

	if (expected_smallest_ip != NULL) {
		parse_ip(family, expected_smallest_ip, exp_ip);
		assert(memcmp(log.items[0].example_ip, exp_ip, nbytes) == 0);
	}

	/* Also verify iprange_compile without callback returns IPRANGE_ERR_CONFLICT */
	iprange_free(ipr);

	ipr = iprange_new(family);
	assert(ipr != NULL);
	for (size_t i = 0; i < n; i++) {
		assert(iprange_add_str(ipr, specs[i].start, specs[i].end,
			specs[i].label, NULL) == IPRANGE_OK);
	}
	assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_ERR_CONFLICT);
	assert(fsm == NULL);
	iprange_free(ipr);
}

static void
test_exact_duplicate_conflicts(void)
{
	/* /32 duplicate */
	{
		const struct range_spec specs[] = {
			{ "8.8.8.8", "8.8.8.8", "dns_a" },
			{ "8.8.8.8", "8.8.8.8", "dns_b" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, specs, 2,
			"dns_a", "dns_b", "8.8.8.8");
	}

	/* /8 duplicate */
	{
		const struct range_spec specs[] = {
			{ "10.0.0.0", "10.255.255.255", "rfc1918" },
			{ "10.0.0.0", "10.255.255.255", "corp" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, specs, 2,
			"rfc1918", "corp", "10.0.0.0");
	}
}

static void
test_cross_depth_containment_conflicts(void)
{
	/* /8 vs /32 (both orders: previously missed when prefix depths differed) */
	{
		const struct range_spec a_then_b[] = {
			{ "10.0.0.0", "10.255.255.255", "broad" },
			{ "10.1.2.3", "10.1.2.3",       "narrow" }
		};
		const struct range_spec b_then_a[] = {
			{ "10.1.2.3", "10.1.2.3",       "narrow" },
			{ "10.0.0.0", "10.255.255.255", "broad" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, a_then_b, 2,
			"broad", "narrow", "10.1.2.3");
		assert_two_way_conflict(IPRANGE_IPV4, b_then_a, 2,
			"broad", "narrow", "10.1.2.3");
	}

	/* /8 vs /24 and /8 vs /16 */
	{
		const struct range_spec s24[] = {
			{ "10.0.0.0", "10.255.255.255", "broad" },
			{ "10.1.2.0", "10.1.2.255",     "sub24" }
		};
		const struct range_spec s16[] = {
			{ "10.1.0.0", "10.1.255.255",   "sub16" },
			{ "10.0.0.0", "10.255.255.255", "broad" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, s24, 2,
			"broad", "sub24", "10.1.2.0");
		assert_two_way_conflict(IPRANGE_IPV4, s16, 2,
			"broad", "sub16", "10.1.0.0");
	}

	/* /0 full space vs /32 */
	{
		const struct range_spec s0[] = {
			{ "0.0.0.0",   "255.255.255.255", "all" },
			{ "192.0.2.9", "192.0.2.9",       "pin" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, s0, 2,
			"all", "pin", "192.0.2.9");
	}

	/* Non-CIDR outer vs non-CIDR inner */
	{
		const struct range_spec arb[] = {
			{ "1.2.3.4",   "5.6.7.8",   "outer" },
			{ "2.100.0.5", "3.0.10.20", "inner" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, arb, 2,
			"outer", "inner", "2.100.0.5");
	}

	/* Shared start boundary and shared end boundary */
	{
		const struct range_spec shared_start[] = {
			{ "10.0.0.0", "10.0.0.100", "A" },
			{ "10.0.0.0", "10.0.0.0",   "B" }
		};
		const struct range_spec shared_end[] = {
			{ "10.0.0.0",   "10.0.0.100", "A" },
			{ "10.0.0.100", "10.0.0.100", "B" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, shared_start, 2,
			"A", "B", "10.0.0.0");
		assert_two_way_conflict(IPRANGE_IPV4, shared_end, 2,
			"A", "B", "10.0.0.100");
	}
}

static void
test_kissing_and_partial_conflicts(void)
{
	/* Single-IP overlap (A.end == B.start) within and across octets */
	{
		const struct range_spec k_oct3[] = {
			{ "10.0.0.0",  "10.0.0.50",  "A" },
			{ "10.0.0.50", "10.0.0.100", "B" }
		};
		const struct range_spec k_oct2[] = {
			{ "10.0.0.0", "10.0.1.0",   "A" },
			{ "10.0.1.0", "10.0.1.255", "B" }
		};
		const struct range_spec k_oct0[] = {
			{ "1.0.0.0", "2.0.0.0",         "A" },
			{ "2.0.0.0", "2.255.255.255",   "B" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, k_oct3, 2,
			"A", "B", "10.0.0.50");
		assert_two_way_conflict(IPRANGE_IPV4, k_oct2, 2,
			"A", "B", "10.0.1.0");
		assert_two_way_conflict(IPRANGE_IPV4, k_oct0, 2,
			"A", "B", "2.0.0.0");
	}

	/* Partial overlaps across 1, 2, and 4 octets */
	{
		const struct range_spec p1[] = {
			{ "10.0.0.10", "10.0.0.60", "A" },
			{ "10.0.0.40", "10.0.0.90", "B" }
		};
		const struct range_spec p2[] = {
			{ "10.0.0.200", "10.0.2.50",  "A" },
			{ "10.0.1.100", "10.0.3.10",  "B" }
		};
		const struct range_spec p4[] = {
			{ "2.3.4.5", "4.5.6.7", "B" },
			{ "1.2.3.4", "3.4.5.6", "A" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, p1, 2,
			"A", "B", "10.0.0.40");
		assert_two_way_conflict(IPRANGE_IPV4, p2, 2,
			"A", "B", "10.0.1.100");
		assert_two_way_conflict(IPRANGE_IPV4, p4, 2,
			"A", "B", "2.3.4.5");
	}

	/* Multi-range records where only later subranges of A and B overlap */
	{
		const struct range_spec multi[] = {
			{ "1.0.0.0",   "1.255.255.255", "A" },
			{ "2.0.0.0",   "2.255.255.255", "B" },
			{ "10.0.0.0",  "10.0.5.10",     "A" },
			{ "10.0.5.10", "10.0.9.255",    "B" }
		};
		assert_two_way_conflict(IPRANGE_IPV4, multi, 4,
			"A", "B", "10.0.5.10");
	}
}

static void
test_three_way_conflict(void)
{
	/*
	 * A = [10.0.0.0, 10.0.0.100]
	 * B = [10.0.0.30, 10.0.0.80]
	 * C = [10.0.0.50, 10.0.0.120]
	 *
	 * Distinct overlap combinations:
	 *  - {A, B}    on [10.0.0.30, 10.0.0.49] (smallest: 10.0.0.30)
	 *  - {A, B, C} on [10.0.0.50, 10.0.0.80] (smallest: 10.0.0.50)
	 *  - {A, C}    on [10.0.0.81, 10.0.0.100] (smallest: 10.0.0.81)
	 */
	const struct range_spec specs[] = {
		{ "10.0.0.0",  "10.0.0.100", "A" },
		{ "10.0.0.30", "10.0.0.80",  "B" },
		{ "10.0.0.50", "10.0.0.120", "C" }
	};
	struct iprange *ipr;
	struct fsm *fsm = NULL;
	struct conflict_log log;
	int saw_abc = 0;

	memset(&log, 0, sizeof log);
	ipr = iprange_new(IPRANGE_IPV4);
	assert(ipr != NULL);
	for (size_t i = 0; i < ARR_LEN(specs); i++) {
		assert(iprange_add_str(ipr, specs[i].start, specs[i].end,
			specs[i].label, NULL) == IPRANGE_OK);
	}

	assert(iprange_compile(ipr, &fsm, record_conflict_cb, &log) ==
		IPRANGE_ERR_CONFLICT);
	assert(fsm == NULL);
	assert(log.count == 3);

	for (size_t i = 0; i < log.count; i++) {
		const struct recorded_conflict *c = &log.items[i];
		for (size_t j = 0; j < c->count; j++) {
			assert(spec_set_contains_ip_for_label(IPRANGE_IPV4,
				specs, ARR_LEN(specs), c->labels[j], c->example_ip));
		}
		if (c->count == 3) {
			unsigned char exp[16];
			assert(conflict_has_label(c, "A"));
			assert(conflict_has_label(c, "B"));
			assert(conflict_has_label(c, "C"));
			parse_ip(IPRANGE_IPV4, "10.0.0.50", exp);
			assert(memcmp(c->example_ip, exp, 4) == 0);
			saw_abc = 1;
		}
	}
	assert(saw_abc == 1);
	iprange_free(ipr);
}

static void
test_adjacent_different_labels_do_not_conflict(void)
{
	/* Tight A.end + 1 == B.start boundaries across every octet position */
	const struct range_spec specs[] = {
		{ "0.0.0.0",     "0.255.255.255", "b0_left" },
		{ "1.0.0.0",     "1.0.0.0",       "b0_right" },
		{ "10.0.0.0",    "10.0.0.49",     "b3_left" },
		{ "10.0.0.50",   "10.0.0.100",    "b3_right" },
		{ "10.0.1.0",    "10.0.1.255",    "b2_left" },
		{ "10.0.2.0",    "10.0.2.255",    "b2_right" },
		{ "10.1.0.0",    "10.1.255.255",  "b1_left" },
		{ "10.2.0.0",    "10.2.255.255",  "b1_right" },
		{ "19.255.255.255", "19.255.255.255", "carry_left" },
		{ "20.0.0.0",    "20.0.0.0",      "carry_right" }
	};
	struct iprange *ipr;
	struct fsm *fsm = compile_specs(IPRANGE_IPV4,
		specs, ARR_LEN(specs), &ipr);

	assert_match_str(fsm, ipr, "0.255.255.255", "b0_left", 1);
	assert_match_str(fsm, ipr, "1.0.0.0", "b0_right", 4);
	assert_match_str(fsm, ipr, "1.0.0.1", NULL, 0);

	assert_match_str(fsm, ipr, "10.0.0.49", "b3_left", 4);
	assert_match_str(fsm, ipr, "10.0.0.50", "b3_right", 4);
	assert_match_str(fsm, ipr, "10.0.0.101", NULL, 0);

	assert_match_str(fsm, ipr, "10.0.1.255", "b2_left", 3);
	assert_match_str(fsm, ipr, "10.0.2.0", "b2_right", 3);

	assert_match_str(fsm, ipr, "10.1.255.255", "b1_left", 2);
	assert_match_str(fsm, ipr, "10.2.0.0", "b1_right", 2);

	assert_match_str(fsm, ipr, "19.255.255.255", "carry_left", 4);
	assert_match_str(fsm, ipr, "20.0.0.0", "carry_right", 4);

	fsm_free(fsm);
	iprange_free(ipr);
}

static void
test_ipv6_conflicts_and_adjacencies(void)
{
	/* /8 vs /128 conflict */
	{
		const struct range_spec specs[] = {
			{ "ff00::", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "mcast" },
			{ "ff02::1", "ff02::1", "all_nodes" }
		};
		assert_two_way_conflict(IPRANGE_IPV6, specs, 2,
			"mcast", "all_nodes", "ff02::1");
	}

	/* Single-IP overlap at hextet boundary */
	{
		const struct range_spec specs[] = {
			{ "2001:db8::",    "2001:db8:0:1::", "A" },
			{ "2001:db8:0:1::", "2001:db8:0:2::", "B" }
		};
		assert_two_way_conflict(IPRANGE_IPV6, specs, 2,
			"A", "B", "2001:db8:0:1::");
	}

	/* Partial overlap across hextets */
	{
		const struct range_spec specs[] = {
			{ "2001:db8:1:2::100", "2001:db8:1:5::50",  "A" },
			{ "2001:db8:1:3::200", "2001:db8:1:8::999", "B" }
		};
		assert_two_way_conflict(IPRANGE_IPV6, specs, 2,
			"A", "B", "2001:db8:1:3::200");
	}

	/* Adjacent IPv6 ranges do NOT conflict */
	{
		const struct range_spec specs[] = {
			{ "2001:db8::", "2001:db8:0:0:ffff:ffff:ffff:ffff", "left" },
			{ "2001:db8:0:1::", "2001:db8:0:1:ffff:ffff:ffff:ffff", "right" }
		};
		struct iprange *ipr;
		struct fsm *fsm = compile_specs(IPRANGE_IPV6, specs, 2, &ipr);
		assert_match_str(fsm, ipr, "2001:db8::ffff", "left", 8);
		assert_match_str(fsm, ipr, "2001:db8:0:1::", "right", 8);
		fsm_free(fsm);
		iprange_free(ipr);
	}
}

int
main(void)
{
	test_exact_duplicate_conflicts();
	test_cross_depth_containment_conflicts();
	test_kissing_and_partial_conflicts();
	test_three_way_conflict();
	test_adjacent_different_labels_do_not_conflict();
	test_ipv6_conflicts_and_adjacencies();
	return EXIT_SUCCESS;
}
