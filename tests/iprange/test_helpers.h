#ifndef IPRANGE_TEST_HELPERS_H
#define IPRANGE_TEST_HELPERS_H

#include <stddef.h>
#include <stdint.h>

#include <fsm/fsm.h>
#include "iprange.h"

struct range_spec {
	const char *start;
	const char *end;
	const char *label;
};

struct recorded_conflict {
	char labels[8][64];
	unsigned ids[8];
	size_t count;
	unsigned char example_ip[16];
};

struct conflict_log {
	struct recorded_conflict items[32];
	size_t count;
};

void
u32_to_octets(uint32_t ip, unsigned char out[4]);

uint32_t
octets_to_u32(const unsigned char octs[4]);

void
parse_ip(enum iprange_family family, const char *s, unsigned char out[16]);

int
ip_inc(unsigned char *ip, size_t nbytes);

int
ip_dec(unsigned char *ip, size_t nbytes);

void
ip_mid(const unsigned char *a, const unsigned char *b,
	size_t nbytes, unsigned char *out);

void
assert_valid_leaf_dfa(const struct fsm *fsm, enum iprange_family family);

void
assert_match_bytes(const struct fsm *fsm, const struct iprange *ipr,
	const unsigned char *addr, const char *expected_label,
	size_t expected_bytes);

void
assert_match_str(const struct fsm *fsm, const struct iprange *ipr,
	const char *ip_str, const char *expected_label,
	size_t expected_bytes);

struct fsm *
compile_specs(enum iprange_family family,
	const struct range_spec *specs, size_t n,
	struct iprange **ipr_out);

void
assert_equivalent_specs(enum iprange_family family,
	const struct range_spec *specs_a, size_t na,
	const struct range_spec *specs_b, size_t nb);

void
record_conflict_cb(const char * const *labels, const unsigned *ids,
	size_t count, const unsigned char *example_ip, void *opaque);

int
conflict_has_label(const struct recorded_conflict *c, const char *label);

int
spec_set_contains_ip_for_label(enum iprange_family family,
	const struct range_spec *specs, size_t n,
	const char *label, const unsigned char *ip);

#endif
