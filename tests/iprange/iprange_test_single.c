#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>

#include "test_helpers.h"

static void
test_empty_and_errors(void)
{
	struct iprange *ipr;
	struct fsm *fsm = NULL;

	/* Empty IPv4 set */
	ipr = iprange_new(IPRANGE_IPV4);
	assert(ipr != NULL);
	assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
	assert_valid_leaf_dfa(fsm, IPRANGE_IPV4);
	assert(fsm_countstates(fsm) == 1);
	assert_match_str(fsm, ipr, "0.0.0.0", NULL, 0);
	assert_match_str(fsm, ipr, "255.255.255.255", NULL, 0);
	fsm_free(fsm);
	iprange_free(ipr);

	/* Empty IPv6 set */
	ipr = iprange_new(IPRANGE_IPV6);
	assert(ipr != NULL);
	assert(iprange_compile(ipr, &fsm, NULL, NULL) == IPRANGE_OK);
	assert_valid_leaf_dfa(fsm, IPRANGE_IPV6);
	assert(fsm_countstates(fsm) == 1);
	assert_match_str(fsm, ipr, "::", NULL, 0);
	assert_match_str(fsm, ipr, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
		NULL, 0);
	fsm_free(fsm);
	iprange_free(ipr);

	/* Reversed ranges and invalid IP strings */
	ipr = iprange_new(IPRANGE_IPV4);
	assert(ipr != NULL);
	assert(iprange_add_str(ipr, "10.0.0.5", "10.0.0.4", "bad", NULL) ==
		IPRANGE_ERR_RANGE);
	assert(iprange_add_str(ipr, "10.0.1.0", "10.0.0.255", "bad", NULL) ==
		IPRANGE_ERR_RANGE);
	assert(iprange_add_str(ipr, "10.1.0.0", "10.0.255.255", "bad", NULL) ==
		IPRANGE_ERR_RANGE);
	assert(iprange_add_str(ipr, "11.0.0.0", "10.255.255.255", "bad", NULL) ==
		IPRANGE_ERR_RANGE);
	assert(iprange_add_str(ipr, "not-an-ip", "10.0.0.1", "bad", NULL) ==
		IPRANGE_ERR_PARSE);
	assert(iprange_record_count(ipr) == 0);
	iprange_free(ipr);

	ipr = iprange_new(IPRANGE_IPV6);
	assert(ipr != NULL);
	assert(iprange_add_str(ipr, "2001:db8::2", "2001:db8::1", "bad", NULL) ==
		IPRANGE_ERR_RANGE);
	assert(iprange_add_str(ipr, "2001:db9::", "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff",
		"bad", NULL) == IPRANGE_ERR_RANGE);
	iprange_free(ipr);
}

static void
test_single_ipv4_decompositions(void)
{
	struct iprange *ipr;
	struct fsm *fsm;

	/* /32 single addresses at extremes and interior */
	{
		const struct range_spec specs[] = {
			{ "0.0.0.0",         "0.0.0.0",         "zero" },
			{ "1.2.3.4",         "1.2.3.4",         "host" },
			{ "255.255.255.255", "255.255.255.255", "bcast" }
		};
		fsm = compile_specs(IPRANGE_IPV4, specs, 3, &ipr);
		assert_match_str(fsm, ipr, "0.0.0.0", "zero", 4);
		assert_match_str(fsm, ipr, "0.0.0.1", NULL, 0);
		assert_match_str(fsm, ipr, "1.2.3.3", NULL, 0);
		assert_match_str(fsm, ipr, "1.2.3.4", "host", 4);
		assert_match_str(fsm, ipr, "1.2.3.5", NULL, 0);
		assert_match_str(fsm, ipr, "255.255.255.254", NULL, 0);
		assert_match_str(fsm, ipr, "255.255.255.255", "bcast", 4);
		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* /0 full IPv4 space: 2 states, 1 byte consumed */
	{
		const struct range_spec specs[] = {
			{ "0.0.0.0", "255.255.255.255", "all" }
		};
		fsm = compile_specs(IPRANGE_IPV4, specs, 1, &ipr);
		assert(fsm_countstates(fsm) == 2);
		assert_match_str(fsm, ipr, "0.0.0.0", "all", 1);
		assert_match_str(fsm, ipr, "127.0.0.1", "all", 1);
		assert_match_str(fsm, ipr, "255.255.255.255", "all", 1);
		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* Byte-aligned CIDRs (/8, /16, /24) and multi-/24 ranges */
	{
		const struct range_spec specs[] = {
			{ "10.0.0.0",    "10.255.255.255", "slash8" },
			{ "172.16.0.0",  "172.16.255.255", "slash16" },
			{ "192.0.2.0",   "192.0.2.255",    "slash24" },
			{ "192.0.3.0",   "192.0.5.255",    "three_slash24" }
		};
		fsm = compile_specs(IPRANGE_IPV4, specs, 4, &ipr);
		assert_match_str(fsm, ipr, "9.255.255.255", NULL, 0);
		assert_match_str(fsm, ipr, "10.0.0.0", "slash8", 1);
		assert_match_str(fsm, ipr, "10.255.255.255", "slash8", 1);
		assert_match_str(fsm, ipr, "11.0.0.0", NULL, 0);

		assert_match_str(fsm, ipr, "172.15.255.255", NULL, 0);
		assert_match_str(fsm, ipr, "172.16.0.0", "slash16", 2);
		assert_match_str(fsm, ipr, "172.16.255.255", "slash16", 2);
		assert_match_str(fsm, ipr, "172.17.0.0", NULL, 0);

		assert_match_str(fsm, ipr, "192.0.1.255", NULL, 0);
		assert_match_str(fsm, ipr, "192.0.2.0", "slash24", 3);
		assert_match_str(fsm, ipr, "192.0.2.255", "slash24", 3);

		assert_match_str(fsm, ipr, "192.0.3.0", "three_slash24", 3);
		assert_match_str(fsm, ipr, "192.0.4.128", "three_slash24", 3);
		assert_match_str(fsm, ipr, "192.0.5.255", "three_slash24", 3);
		assert_match_str(fsm, ipr, "192.0.6.0", NULL, 0);
		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* Zero-ending upper bounds (regression for eocts[d] - 1 underflow) */
	{
		const struct range_spec specs[] = {
			{ "1.0.0.0",  "1.0.1.0",  "r1" },
			{ "2.0.0.0",  "3.0.0.0",  "r2" },
			{ "10.1.0.0", "10.3.0.0", "r3" }
		};
		fsm = compile_specs(IPRANGE_IPV4, specs, 3, &ipr);
		assert_match_str(fsm, ipr, "0.255.255.255", NULL, 0);
		assert_match_str(fsm, ipr, "1.0.0.0", "r1", 3);
		assert_match_str(fsm, ipr, "1.0.0.255", "r1", 3);
		assert_match_str(fsm, ipr, "1.0.1.0", "r1", 4);
		assert_match_str(fsm, ipr, "1.0.1.1", NULL, 0);
		assert_match_str(fsm, ipr, "1.0.1.255", NULL, 0);

		assert_match_str(fsm, ipr, "1.255.255.255", NULL, 0);
		assert_match_str(fsm, ipr, "2.0.0.0", "r2", 1);
		assert_match_str(fsm, ipr, "2.255.255.255", "r2", 1);
		assert_match_str(fsm, ipr, "3.0.0.0", "r2", 4);
		assert_match_str(fsm, ipr, "3.0.0.1", NULL, 0);

		assert_match_str(fsm, ipr, "10.0.255.255", NULL, 0);
		assert_match_str(fsm, ipr, "10.1.0.0", "r3", 2);
		assert_match_str(fsm, ipr, "10.2.255.255", "r3", 2);
		assert_match_str(fsm, ipr, "10.3.0.0", "r3", 4);
		assert_match_str(fsm, ipr, "10.3.0.1", NULL, 0);
		fsm_free(fsm);
		iprange_free(ipr);
	}

	/* Multi-byte carry propagation across 255 -> 0 octets */
	{
		const struct range_spec specs[] = {
			{ "0.255.255.255", "2.0.0.0",   "carry3" },
			{ "5.1.255.5",     "5.2.0.3",   "carry1" },
			{ "8.255.255.250", "9.0.0.5",   "carry2" }
		};
		fsm = compile_specs(IPRANGE_IPV4, specs, 3, &ipr);

		assert_match_str(fsm, ipr, "0.255.255.254", NULL, 0);
		assert_match_str(fsm, ipr, "0.255.255.255", "carry3", 4);
		assert_match_str(fsm, ipr, "1.0.0.0", "carry3", 1);
		assert_match_str(fsm, ipr, "1.255.255.255", "carry3", 1);
		assert_match_str(fsm, ipr, "2.0.0.0", "carry3", 4);
		assert_match_str(fsm, ipr, "2.0.0.1", NULL, 0);

		assert_match_str(fsm, ipr, "5.1.0.0", NULL, 0);
		assert_match_str(fsm, ipr, "5.1.255.4", NULL, 0);
		assert_match_str(fsm, ipr, "5.1.255.5", "carry1", 4);
		assert_match_str(fsm, ipr, "5.1.255.255", "carry1", 4);
		assert_match_str(fsm, ipr, "5.2.0.0", "carry1", 4);
		assert_match_str(fsm, ipr, "5.2.0.3", "carry1", 4);
		assert_match_str(fsm, ipr, "5.2.0.4", NULL, 0);
		assert_match_str(fsm, ipr, "5.2.1.0", NULL, 0);

		assert_match_str(fsm, ipr, "8.255.255.249", NULL, 0);
		assert_match_str(fsm, ipr, "8.255.255.250", "carry2", 4);
		assert_match_str(fsm, ipr, "8.255.255.255", "carry2", 4);
		assert_match_str(fsm, ipr, "9.0.0.0", "carry2", 4);
		assert_match_str(fsm, ipr, "9.0.0.5", "carry2", 4);
		assert_match_str(fsm, ipr, "9.0.0.6", NULL, 0);

		fsm_free(fsm);
		iprange_free(ipr);
	}
}

static void
test_single_ipv6_decompositions(void)
{
	struct iprange *ipr;
	struct fsm *fsm;

	const struct range_spec specs[] = {
		{ "::", "::", "unspec" },
		{ "::1", "::1", "loopback" },
		{ "0:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "1::", "carry_v6" },
		{ "2001:db8::", "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff", "doc" },
		{ "2001:db9::", "2001:db9:0:1::", "zero_end_v6" },
		{ "ff00::", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "mcast" }
	};

	fsm = compile_specs(IPRANGE_IPV6, specs,
		sizeof specs / sizeof *specs, &ipr);

	assert_match_str(fsm, ipr, "::", "unspec", 16);
	assert_match_str(fsm, ipr, "::1", "loopback", 16);
	assert_match_str(fsm, ipr, "::2", NULL, 0);

	assert_match_str(fsm, ipr, "0:ffff:ffff:ffff:ffff:ffff:ffff:fffe", NULL, 0);
	assert_match_str(fsm, ipr, "0:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "carry_v6", 16);
	assert_match_str(fsm, ipr, "1::", "carry_v6", 16);
	assert_match_str(fsm, ipr, "1::1", NULL, 0);

	assert_match_str(fsm, ipr, "2001:db7:ffff:ffff:ffff:ffff:ffff:ffff", NULL, 0);
	assert_match_str(fsm, ipr, "2001:db8::", "doc", 4);
	assert_match_str(fsm, ipr, "2001:db8:1234:5678::1", "doc", 4);
	assert_match_str(fsm, ipr, "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff", "doc", 4);

	assert_match_str(fsm, ipr, "2001:db9::", "zero_end_v6", 8);
	assert_match_str(fsm, ipr, "2001:db9:0:0:ffff:ffff:ffff:ffff", "zero_end_v6", 8);
	assert_match_str(fsm, ipr, "2001:db9:0:1::", "zero_end_v6", 16);
	assert_match_str(fsm, ipr, "2001:db9:0:1::1", NULL, 0);

	assert_match_str(fsm, ipr, "feff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", NULL, 0);
	assert_match_str(fsm, ipr, "ff00::", "mcast", 1);
	assert_match_str(fsm, ipr, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "mcast", 1);

	fsm_free(fsm);
	iprange_free(ipr);
}

static void
verify_csv_file(const char *path, enum iprange_family family)
{
	FILE *f;
	char line[512];
	struct range_spec specs[128];
	char storage[128][3][128];
	size_t n = 0;
	struct iprange *ipr = NULL;
	struct fsm *fsm;
	size_t nbytes = (size_t) family;

	f = fopen(path, "r");
	assert(f != NULL);

	while (fgets(line, sizeof line, f) != NULL) {
		char *c1, *c2;
		line[strcspn(line, "\r\n")] = '\0';
		if (line[0] == '\0') {
			continue;
		}
		c1 = strchr(line, ',');
		assert(c1 != NULL);
		*c1++ = '\0';
		c2 = strchr(c1, ',');
		assert(c2 != NULL);
		*c2++ = '\0';

		assert(n < 128);
		strcpy(storage[n][0], line);
		strcpy(storage[n][1], c1);
		strcpy(storage[n][2], c2);
		specs[n].start = storage[n][0];
		specs[n].end   = storage[n][1];
		specs[n].label = storage[n][2];
		n++;
	}
	fclose(f);

	fsm = compile_specs(family, specs, n, &ipr);

	for (size_t i = 0; i < n; i++) {
		unsigned char s[16], e[16], mid[16];
		parse_ip(family, specs[i].start, s);
		parse_ip(family, specs[i].end, e);
		ip_mid(s, e, nbytes, mid);

		assert_match_bytes(fsm, ipr, s, specs[i].label, 0);
		assert_match_bytes(fsm, ipr, mid, specs[i].label, 0);
		assert_match_bytes(fsm, ipr, e, specs[i].label, 0);
	}

	if (family == IPRANGE_IPV6) {
		/* Gap between 0099:ffff:... and 0100:: in v6ranges.csv */
		assert_match_str(fsm, ipr, "9a::", NULL, 0);
		assert_match_str(fsm, ipr, "ff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
			NULL, 0);
	}

	fsm_free(fsm);
	iprange_free(ipr);
}

int
main(void)
{
	test_empty_and_errors();
	test_single_ipv4_decompositions();
	test_single_ipv6_decompositions();
	verify_csv_file("examples/iprange/v4ranges.csv", IPRANGE_IPV4);
	verify_csv_file("examples/iprange/v6ranges.csv", IPRANGE_IPV6);
	return EXIT_SUCCESS;
}
