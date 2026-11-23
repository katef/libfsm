#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "test_helpers.h"

void
u32_to_octets(uint32_t ip, unsigned char out[4])
{
	out[0] = (unsigned char) ((ip >> 24) & 0xffu);
	out[1] = (unsigned char) ((ip >> 16) & 0xffu);
	out[2] = (unsigned char) ((ip >>  8) & 0xffu);
	out[3] = (unsigned char) ( ip        & 0xffu);
}

uint32_t
octets_to_u32(const unsigned char octs[4])
{
	return ((uint32_t) octs[0] << 24) |
	       ((uint32_t) octs[1] << 16) |
	       ((uint32_t) octs[2] <<  8) |
	       ((uint32_t) octs[3]);
}

void
parse_ip(enum iprange_family family, const char *s, unsigned char out[16])
{
	int af = (family == IPRANGE_IPV4) ? AF_INET : AF_INET6;
	memset(out, 0, 16);
	assert(inet_pton(af, s, out) == 1);
}

int
ip_inc(unsigned char *ip, size_t nbytes)
{
	for (size_t i = nbytes; i > 0; i--) {
		if (ip[i - 1] < 255) {
			ip[i - 1]++;
			return 1;
		}
		ip[i - 1] = 0;
	}
	return 0; /* wrapped past all-0xff */
}

int
ip_dec(unsigned char *ip, size_t nbytes)
{
	for (size_t i = nbytes; i > 0; i--) {
		if (ip[i - 1] > 0) {
			ip[i - 1]--;
			return 1;
		}
		ip[i - 1] = 255;
	}
	return 0; /* wrapped below all-0x00 */
}

void
ip_mid(const unsigned char *a, const unsigned char *b,
	size_t nbytes, unsigned char *out)
{
	unsigned carry = 0;
	unsigned char sum[16];

	for (size_t i = nbytes; i > 0; i--) {
		unsigned s = (unsigned) a[i - 1] + (unsigned) b[i - 1] + carry;
		sum[i - 1] = (unsigned char) (s & 0xffu);
		carry = s >> 8;
	}

	for (size_t i = 0; i < nbytes; i++) {
		unsigned cur = (carry << 8) | sum[i];
		out[i] = (unsigned char) (cur >> 1);
		carry = cur & 1u;
	}
}

struct dag_ctx {
	unsigned nstates;
	unsigned *max_depth;
	unsigned *in_deg;
	fsm_state_t *adj; /* nstates * 256 */
};

static int
dag_noop_eps(const struct fsm *fsm, fsm_state_t from, fsm_state_t to,
	void *opaque)
{
	(void) fsm;
	(void) from;
	(void) to;
	(void) opaque;
	assert(!"unexpected epsilon transition");
	return 0;
}

static int
dag_lit_cb(const struct fsm *fsm, fsm_state_t from, fsm_state_t to,
	char c, void *opaque)
{
	struct dag_ctx *ctx = opaque;
	unsigned char uc = (unsigned char) c;

	(void) fsm;
	assert(from < ctx->nstates);
	assert(to < ctx->nstates);
	assert(ctx->adj[(size_t) from * 256u + uc] == (fsm_state_t) -1);
	ctx->adj[(size_t) from * 256u + uc] = to;
	ctx->in_deg[to]++;
	return 1;
}

/*
 * Verify all structural invariants of a shortest-prefix leaf DFA:
 * - Deterministic, epsilon-free, valid non-end start state.
 * - Every end state has 0 outgoing edges and exactly 1 endid.
 * - Every non-end state (when language is non-empty) has >= 1 outgoing edge
 *    and (unless start) does not have all 256 edges going to a single end state.
 * - Acyclic (DAG) with maximum path length <= family (4 or 16).
 * - Minimal: re-running fsm_minimise() preserves state and edge counts.
 */
void
assert_valid_leaf_dfa(const struct fsm *fsm, enum iprange_family family)
{
	fsm_state_t start = 0;
	unsigned n, processed;
	struct fsm *clone;
	struct dag_ctx dctx;
	fsm_state_t *queue;
	size_t qhead = 0, qtail = 0;

	assert(fsm != NULL);
	assert(fsm_all(fsm, fsm_isdfa) == 1);
	assert(!fsm_has(fsm, fsm_hasepsilons));
	assert(fsm_getstart(fsm, &start) == 1);
	assert(!fsm_isend(fsm, start));

	n = fsm_countstates(fsm);
	assert(n >= 1);

	if (n == 1) {
		assert(fsm_empty(fsm) == 1);
		assert(!fsm_hasoutgoing(fsm, start));
		return;
	}

	for (fsm_state_t s = 0; s < n; s++) {
		if (fsm_isend(fsm, s)) {
			assert(!fsm_hasoutgoing(fsm, s));
			assert(fsm_endid_count(fsm, s) == 1);
		} else {
			assert(fsm_hasoutgoing(fsm, s));
			if (s != start && fsm_iscomplete(fsm, s)) {
				unsigned int freq = 0;
				fsm_state_t mode = fsm_findmode(fsm, s, &freq);
				assert(!(freq == 256 && fsm_isend(fsm, mode)));
			}
		}
	}

	/* Verify DAG and depth <= family via Kahn's topological sort */
	dctx.nstates   = n;
	dctx.max_depth = calloc(n, sizeof *dctx.max_depth);
	dctx.in_deg    = calloc(n, sizeof *dctx.in_deg);
	dctx.adj       = malloc((size_t) n * 256u * sizeof *dctx.adj);
	queue          = malloc(n * sizeof *queue);
	assert(dctx.max_depth != NULL && dctx.in_deg != NULL &&
	       dctx.adj != NULL && queue != NULL);

	for (size_t i = 0; i < (size_t) n * 256u; i++) {
		dctx.adj[i] = (fsm_state_t) -1;
	}

	assert(fsm_walk_edges(fsm, &dctx, dag_lit_cb, dag_noop_eps) == 1);
	assert(dctx.in_deg[start] == 0);

	for (fsm_state_t s = 0; s < n; s++) {
		if (dctx.in_deg[s] == 0) {
			assert(s == start);
			queue[qtail++] = s;
		}
	}

	processed = 0;
	while (qhead < qtail) {
		fsm_state_t u = queue[qhead++];
		processed++;
		assert(dctx.max_depth[u] <= (unsigned) family);

		for (unsigned c = 0; c < 256; c++) {
			fsm_state_t v = dctx.adj[(size_t) u * 256u + c];
			if (v == (fsm_state_t) -1) {
				continue;
			}
			if (dctx.max_depth[u] + 1 > dctx.max_depth[v]) {
				dctx.max_depth[v] = dctx.max_depth[u] + 1;
			}
			if (--dctx.in_deg[v] == 0) {
				queue[qtail++] = v;
			}
		}
	}
	assert(processed == n); /* strictly acyclic, all states reachable */

	free(queue);
	free(dctx.adj);
	free(dctx.in_deg);
	free(dctx.max_depth);

	/* Verify canonical minimality */
	clone = fsm_clone(fsm);
	assert(clone != NULL);
	assert(fsm_minimise(clone) == 1);
	assert(fsm_countstates(clone) == n);
	assert(fsm_countedges(clone) == fsm_countedges(fsm));
	fsm_free(clone);
}

struct raw_slice {
	const unsigned char *p;
	size_t rem;
};

static int
raw_slice_getc(void *opaque)
{
	struct raw_slice *s = opaque;
	if (s->rem == 0) {
		return EOF;
	}
	s->rem--;
	return (int) (*s->p++);
}

void
assert_match_bytes(const struct fsm *fsm, const struct iprange *ipr,
	const unsigned char *addr, const char *expected_label,
	size_t expected_bytes)
{
	enum iprange_family family = iprange_get_family(ipr);
	size_t nbytes = (size_t) family;
	unsigned id = 0;
	size_t consumed = 0;
	int res;
	size_t raw_matches = 0;
	size_t raw_len = 0;

	/* Verify prefix-freeness: at most 1 prefix length in 1..nbytes accepts */
	for (size_t len = 1; len <= nbytes; len++) {
		struct raw_slice s = { addr, len };
		fsm_state_t end_st;
		int r = fsm_exec(fsm, raw_slice_getc, &s, &end_st, NULL);
		assert(r == 0 || r == 1);
		if (r == 1) {
			raw_matches++;
			raw_len = len;
		}
	}
	assert(raw_matches <= 1);

	res = iprange_match(fsm, family, addr, &id, &consumed);
	if (expected_label == NULL) {
		assert(raw_matches == 0);
		assert(res == 0);
	} else {
		const char *got_label;
		assert(raw_matches == 1);
		assert(res == 1);
		assert(consumed == raw_len);
		assert(consumed >= 1 && consumed <= nbytes);
		got_label = iprange_label(ipr, id);
		assert(got_label != NULL);
		if (strcmp(got_label, expected_label) != 0) {
			fprintf(stderr, "label mismatch: got '%s', expected '%s'\n",
				got_label, expected_label);
			assert(strcmp(got_label, expected_label) == 0);
		}
		if (expected_bytes > 0) {
			if (consumed != expected_bytes) {
				fprintf(stderr, "prefix len mismatch: got %zu, expected %zu\n",
					consumed, expected_bytes);
				assert(consumed == expected_bytes);
			}
		}
	}
}

void
assert_match_str(const struct fsm *fsm, const struct iprange *ipr,
	const char *ip_str, const char *expected_label,
	size_t expected_bytes)
{
	unsigned char addr[16];
	parse_ip(iprange_get_family(ipr), ip_str, addr);
	assert_match_bytes(fsm, ipr, addr, expected_label, expected_bytes);
}

struct fsm *
compile_specs(enum iprange_family family,
	const struct range_spec *specs, size_t n,
	struct iprange **ipr_out)
{
	struct iprange *ipr;
	struct fsm *fsm = NULL;
	enum iprange_err err;

	ipr = iprange_new(family);
	assert(ipr != NULL);

	for (size_t i = 0; i < n; i++) {
		err = iprange_add_str(ipr, specs[i].start, specs[i].end,
			specs[i].label, NULL);
		assert(err == IPRANGE_OK);
	}

	err = iprange_compile(ipr, &fsm, NULL, NULL);
	assert(err == IPRANGE_OK);
	assert(fsm != NULL);
	assert_valid_leaf_dfa(fsm, family);

	*ipr_out = ipr;
	return fsm;
}

static void
check_probe_equivalence(enum iprange_family family,
	const struct fsm *fsm_a, const struct iprange *ipr_a,
	const struct fsm *fsm_b, const struct iprange *ipr_b,
	const unsigned char *ip)
{
	unsigned id_a = 0, id_b = 0;
	size_t len_a = 0, len_b = 0;
	int ma = iprange_match(fsm_a, family, ip, &id_a, &len_a);
	int mb = iprange_match(fsm_b, family, ip, &id_b, &len_b);

	assert(ma == mb);
	if (ma == 1) {
		const char *la = iprange_label(ipr_a, id_a);
		const char *lb = iprange_label(ipr_b, id_b);
		assert(la != NULL && lb != NULL);
		assert(strcmp(la, lb) == 0);
		assert(len_a == len_b);
	}
}

static void
probe_spec_boundaries(enum iprange_family family,
	const struct range_spec *specs, size_t n,
	const struct fsm *fsm_a, const struct iprange *ipr_a,
	const struct fsm *fsm_b, const struct iprange *ipr_b)
{
	size_t nbytes = (size_t) family;

	for (size_t i = 0; i < n; i++) {
		unsigned char s[16], e[16], tmp[16];

		parse_ip(family, specs[i].start, s);
		parse_ip(family, specs[i].end, e);

		check_probe_equivalence(family, fsm_a, ipr_a, fsm_b, ipr_b, s);
		check_probe_equivalence(family, fsm_a, ipr_a, fsm_b, ipr_b, e);

		ip_mid(s, e, nbytes, tmp);
		check_probe_equivalence(family, fsm_a, ipr_a, fsm_b, ipr_b, tmp);

		memcpy(tmp, s, nbytes);
		if (ip_dec(tmp, nbytes)) {
			check_probe_equivalence(family, fsm_a, ipr_a,
				fsm_b, ipr_b, tmp);
		}
		memcpy(tmp, s, nbytes);
		if (ip_inc(tmp, nbytes)) {
			check_probe_equivalence(family, fsm_a, ipr_a,
				fsm_b, ipr_b, tmp);
		}
		memcpy(tmp, e, nbytes);
		if (ip_dec(tmp, nbytes)) {
			check_probe_equivalence(family, fsm_a, ipr_a,
				fsm_b, ipr_b, tmp);
		}
		memcpy(tmp, e, nbytes);
		if (ip_inc(tmp, nbytes)) {
			check_probe_equivalence(family, fsm_a, ipr_a,
				fsm_b, ipr_b, tmp);
		}
	}
}

void
assert_equivalent_specs(enum iprange_family family,
	const struct range_spec *specs_a, size_t na,
	const struct range_spec *specs_b, size_t nb)
{
	struct iprange *ipr_a = NULL, *ipr_b = NULL;
	struct fsm *fsm_a, *fsm_b;

	fsm_a = compile_specs(family, specs_a, na, &ipr_a);
	fsm_b = compile_specs(family, specs_b, nb, &ipr_b);

	assert(fsm_equal(fsm_a, fsm_b) == 1);
	assert(fsm_countstates(fsm_a) == fsm_countstates(fsm_b));
	assert(fsm_countedges(fsm_a) == fsm_countedges(fsm_b));

	probe_spec_boundaries(family, specs_a, na, fsm_a, ipr_a, fsm_b, ipr_b);
	probe_spec_boundaries(family, specs_b, nb, fsm_a, ipr_a, fsm_b, ipr_b);

	fsm_free(fsm_a);
	fsm_free(fsm_b);
	iprange_free(ipr_a);
	iprange_free(ipr_b);
}

void
record_conflict_cb(const char * const *labels, const unsigned *ids,
	size_t count, const unsigned char *example_ip, void *opaque)
{
	struct conflict_log *log = opaque;
	struct recorded_conflict *dst;

	assert(log != NULL);
	assert(log->count < sizeof log->items / sizeof *log->items);
	assert(count >= 2 && count <= 8);

	dst = &log->items[log->count++];
	dst->count = count;
	for (size_t i = 0; i < count; i++) {
		assert(labels[i] != NULL);
		assert(strlen(labels[i]) < sizeof dst->labels[i]);
		strcpy(dst->labels[i], labels[i]);
		dst->ids[i] = ids[i];
	}
	memcpy(dst->example_ip, example_ip, 16);
}

int
conflict_has_label(const struct recorded_conflict *c, const char *label)
{
	for (size_t i = 0; i < c->count; i++) {
		if (strcmp(c->labels[i], label) == 0) {
			return 1;
		}
	}
	return 0;
}

int
spec_set_contains_ip_for_label(enum iprange_family family,
	const struct range_spec *specs, size_t n,
	const char *label, const unsigned char *ip)
{
	size_t nbytes = (size_t) family;

	for (size_t i = 0; i < n; i++) {
		unsigned char s[16], e[16];
		if (strcmp(specs[i].label, label) != 0) {
			continue;
		}
		parse_ip(family, specs[i].start, s);
		parse_ip(family, specs[i].end, e);
		if (memcmp(s, ip, nbytes) <= 0 && memcmp(ip, e, nbytes) <= 0) {
			return 1;
		}
	}

	return 0;
}
