/*
 * Copyright 2026 Devon H. O'Dell
 *
 * See LICENCE for the full copyright terms.
 */

/*
 * End-to-end integration tests and workload benchmarks for every libfsm and
 * libre subsystem that calls bm_range(), verifying canonical DFA equivalence
 * and byte-identical code generation across all candidate implementations.
 */

#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/options.h>
#include <fsm/parser.h>
#include <fsm/pred.h>
#include <fsm/print.h>

#include <re/re.h>

#include "libre/ast.h"
#include "libre/print.h"

#include "bitmap_impls.h"
#include "bitmap_shim.h"

#define DEF_SCALE  2UL
#define DEF_TRIALS 21UL
#define MAX_IMPLS  32U

struct mem_slice {
	const unsigned char *p;
	size_t n;
};

static int
mem_getc(void *opaque)
{
	struct mem_slice *ms = opaque;

	if (ms->n == 0) {
		return EOF;
	}
	ms->n--;
	return (int) *ms->p++;
}

static int
exec_bytes(const struct fsm *dfa, const void *buf, size_t len)
{
	struct mem_slice ms;
	fsm_state_t end;

	ms.p = buf;
	ms.n = len;
	return fsm_exec(dfa, mem_getc, &ms, &end, NULL);
}

static double
now_us(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	return (double) ts.tv_sec * 1e6 + (double) ts.tv_nsec * 1e-3;
}

static uint32_t
xs32(uint32_t *state)
{
	uint32_t x = *state;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}

static int
cmp_double(const void *pa, const void *pb)
{
	double a = *(const double *) pa;
	double b = *(const double *) pb;

	return (a > b) - (a < b);
}

static double
sqrt_nonneg(double x)
{
	double g;
	int i;

	if (x <= 0.0) {
		return 0.0;
	}
	g = x > 1.0 ? x : 1.0;
	for (i = 0; i < 64; i++) {
		g = 0.5 * (g + x / g);
	}
	return g;
}

static double
quantile_sorted(const double *s, size_t n, double q)
{
	double pos, frac;
	size_t idx;

	if (n == 1) {
		return s[0];
	}
	pos = q * (double) (n - 1);
	idx = (size_t) pos;
	frac = pos - (double) idx;
	if (idx + 1 >= n) {
		return s[n - 1];
	}
	return s[idx] + frac * (s[idx + 1] - s[idx]);
}

static void
format_stats(char *buf, size_t cap, const double *samples, double *scratch,
	size_t n, int prec)
{
	double mean = 0.0, var = 0.0;
	double p25, p50, p75, hi, sd;
	size_t i;

	memcpy(scratch, samples, n * sizeof *scratch);
	qsort(scratch, n, sizeof *scratch, cmp_double);

	for (i = 0; i < n; i++) {
		mean += scratch[i];
	}
	mean /= (double) n;

	if (n > 1) {
		for (i = 0; i < n; i++) {
			double d = scratch[i] - mean;

			var += d * d;
		}
		sd = sqrt_nonneg(var / (double) (n - 1));
	} else {
		sd = 0.0;
	}

	p25 = quantile_sorted(scratch, n, 0.25);
	p50 = quantile_sorted(scratch, n, 0.50);
	p75 = quantile_sorted(scratch, n, 0.75);
	hi  = scratch[n - 1];

	(void) snprintf(buf, cap, "%.*f [%.*f/%.*f/%.*f] (%.*f)",
		prec, p50, prec, p25, prec, p75, prec, hi, prec, sd);
}

static void
shuffle_indices(size_t *order, size_t n, uint32_t *rng)
{
	size_t i;

	for (i = n; i > 1; i--) {
		size_t j = (size_t) (xs32(rng) % (uint32_t) i);
		size_t tmp = order[i - 1];

		order[i - 1] = order[j];
		order[j] = tmp;
	}
}

static FILE *
open_sink(void)
{
	FILE *f = fopen("/dev/null", "w");

	if (f == NULL) {
		f = tmpfile();
	}
	return f;
}

static char *
slurp_tmpfile(FILE *tf, size_t *len_out)
{
	long pos;
	size_t len;
	char *buf;

	if (fflush(tf) != 0) {
		return NULL;
	}
	pos = ftell(tf);
	if (pos < 0) {
		return NULL;
	}
	len = (size_t) pos;
	buf = malloc(len + 1);
	if (buf == NULL) {
		return NULL;
	}
	rewind(tf);
	if (len > 0 && fread(buf, 1, len, tf) != len) {
		free(buf);
		return NULL;
	}
	buf[len] = '\0';
	*len_out = len;
	return buf;
}

/*
 * Builds an NFA matching `steps` UTF-8 scalar values (excluding '"' and '\\')
 * directly through fsm_addedge_range() and fsm_addedge_literal(), exercising
 * the RFC 3629 surrogate and overlong exclusion subranges.
 */
static struct fsm *
make_utf8_chain(size_t steps)
{
	struct fsm *fsm;
	fsm_state_t s;
	size_t i;

	fsm = fsm_new(NULL);
	if (fsm == NULL) {
		return NULL;
	}
	if (!fsm_addstate(fsm, &s)) {
		fsm_free(fsm);
		return NULL;
	}
	fsm_setstart(fsm, s);

	for (i = 0; i < steps; i++) {
		fsm_state_t next, c1, c2, c3, e0, ed, f0, f4;

		if (!fsm_addstate(fsm, &next)
			|| !fsm_addstate(fsm, &c1)
			|| !fsm_addstate(fsm, &c2)
			|| !fsm_addstate(fsm, &c3)
			|| !fsm_addstate(fsm, &e0)
			|| !fsm_addstate(fsm, &ed)
			|| !fsm_addstate(fsm, &f0)
			|| !fsm_addstate(fsm, &f4))
		{
			fsm_free(fsm);
			return NULL;
		}

		if (!fsm_addedge_range(fsm, s, next, ' ', '!')
			|| !fsm_addedge_range(fsm, s, next, '#', '[')
			|| !fsm_addedge_range(fsm, s, next, ']', '~')
			|| !fsm_addedge_range(fsm, s, c1, '\xc2', '\xdf')
			|| !fsm_addedge_literal(fsm, s, e0, '\xe0')
			|| !fsm_addedge_range(fsm, s, c2, '\xe1', '\xec')
			|| !fsm_addedge_range(fsm, s, c2, '\xee', '\xef')
			|| !fsm_addedge_literal(fsm, s, ed, '\xed')
			|| !fsm_addedge_literal(fsm, s, f0, '\xf0')
			|| !fsm_addedge_range(fsm, s, c3, '\xf1', '\xf3')
			|| !fsm_addedge_literal(fsm, s, f4, '\xf4')
			|| !fsm_addedge_range(fsm, e0, c1, '\xa0', '\xbf')
			|| !fsm_addedge_range(fsm, ed, c1, '\x80', '\x9f')
			|| !fsm_addedge_range(fsm, f0, c2, '\x90', '\xbf')
			|| !fsm_addedge_range(fsm, f4, c2, '\x80', '\x8f')
			|| !fsm_addedge_range(fsm, c3, c2, '\x80', '\xbf')
			|| !fsm_addedge_range(fsm, c2, c1, '\x80', '\xbf')
			|| !fsm_addedge_range(fsm, c1, next, '\x80', '\xbf'))
		{
			fsm_free(fsm);
			return NULL;
		}

		s = next;
	}

	fsm_setend(fsm, s, 1);
	return fsm;
}

static int
bench_utf8_dfa_api(unsigned long iters, size_t steps, double *us_out)
{
	unsigned long i;
	double t0, dt;

	t0 = now_us();
	for (i = 0; i < iters; i++) {
		struct fsm *f = make_utf8_chain(steps);

		if (f == NULL) {
			return 0;
		}
		fsm_free(f);
	}
	dt = now_us() - t0;

	*us_out = dt / (double) iters;
	return 1;
}

/*
 * Serializes a UTF-8 DFA with consolidate_edges enabled so FSM_PRINT_FSM
 * emits contiguous transitions as `x..y` range edges, which fsm_parse()
 * then reconstructs through bm_range().
 */
static int
prepare_fsm_fixture(size_t repeat_len, struct fsm **dfa_out, FILE **fsm_file_out)
{
	char pat[512];
	const char *p;
	struct re_err err;
	struct fsm *dfa;
	struct fsm_options opt;
	FILE *tf;
	int n;

	memset(&opt, 0, sizeof opt);
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	n = snprintf(pat, sizeof pat,
		"^(?:[\\x20-\\x21\\x23-\\x5b\\x5d-\\x7e]|\\\\[\\x20-\\x7e]|"
		"[\\xc2-\\xdf][\\x80-\\xbf]|\\xe0[\\xa0-\\xbf][\\x80-\\xbf]|"
		"[\\xe1-\\xec\\xee-\\xef][\\x80-\\xbf]{2}|"
		"\\xed[\\x80-\\x9f][\\x80-\\xbf]|"
		"\\xf0[\\x90-\\xbf][\\x80-\\xbf]{2}|"
		"[\\xf1-\\xf3][\\x80-\\xbf]{3}|"
		"\\xf4[\\x80-\\x8f][\\x80-\\xbf]{2}){%zu}$",
		repeat_len);
	if (n < 0 || (size_t) n >= sizeof pat) {
		return 0;
	}

	p = pat;
	dfa = re_comp(RE_PCRE, fsm_sgetc, &p, NULL, 0, &err);
	if (dfa == NULL) {
		return 0;
	}
	if (!fsm_determinise(dfa) || !fsm_minimise(dfa)) {
		fsm_free(dfa);
		return 0;
	}

	tf = tmpfile();
	if (tf == NULL) {
		fsm_free(dfa);
		return 0;
	}

	if (fsm_print(tf, dfa, &opt, NULL, FSM_PRINT_FSM) != 0
		|| fflush(tf) != 0)
	{
		fclose(tf);
		fsm_free(dfa);
		return 0;
	}

	*dfa_out = dfa;
	*fsm_file_out = tf;
	return 1;
}

static int
bench_fsm_parse_ranges(FILE *fsm_file, unsigned long iters, double *us_out)
{
	unsigned long i;
	double t0, dt;

	t0 = now_us();
	for (i = 0; i < iters; i++) {
		struct fsm *f;

		rewind(fsm_file);
		f = fsm_parse(fsm_file, NULL);
		if (f == NULL) {
			return 0;
		}
		fsm_free(f);
	}
	dt = now_us() - t0;

	*us_out = dt / (double) iters;
	return 1;
}

static int
capture_ir_codegen(const struct fsm *dfa, char **c_out, size_t *c_len,
	char **vmc_out, size_t *vmc_len)
{
	struct fsm_options opt;
	FILE *tf_c, *tf_vmc;

	memset(&opt, 0, sizeof opt);
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	tf_c = tmpfile();
	tf_vmc = tmpfile();
	if (tf_c == NULL || tf_vmc == NULL) {
		if (tf_c != NULL) {
			fclose(tf_c);
		}
		if (tf_vmc != NULL) {
			fclose(tf_vmc);
		}
		return 0;
	}

	if (fsm_print(tf_c, dfa, &opt, NULL, FSM_PRINT_C) != 0
		|| fsm_print(tf_vmc, dfa, &opt, NULL, FSM_PRINT_VMC) != 0)
	{
		fclose(tf_c);
		fclose(tf_vmc);
		return 0;
	}

	*c_out = slurp_tmpfile(tf_c, c_len);
	*vmc_out = slurp_tmpfile(tf_vmc, vmc_len);
	fclose(tf_c);
	fclose(tf_vmc);

	if (*c_out == NULL || *vmc_out == NULL) {
		free(*c_out);
		free(*vmc_out);
		return 0;
	}

	return 1;
}

static int
bench_ir_codegen(const struct fsm *dfa, FILE *sink, unsigned long iters,
	double *us_out)
{
	struct fsm_options opt;
	unsigned long i;
	double t0, dt;

	memset(&opt, 0, sizeof opt);
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	t0 = now_us();
	for (i = 0; i < iters; i++) {
		rewind(sink);
		if (fsm_print(sink, dfa, &opt, NULL, FSM_PRINT_C) != 0) {
			return 0;
		}
		rewind(sink);
		if (fsm_print(sink, dfa, &opt, NULL, FSM_PRINT_VMC) != 0) {
			return 0;
		}
	}
	dt = now_us() - t0;

	*us_out = dt / (double) iters;
	return 1;
}

/*
 * Decomposes an arbitrary 32-bit IPv4 interval [ip_lo, ip_hi] at the first
 * differing octet into an interior span (a[oct]+1 .. b[oct]-1 followed by
 * 0..255 tails), a lower-bound suffix chain, and an upper-bound suffix chain.
 */
static int
add_ipv4_range(struct fsm *fsm, fsm_state_t root, fsm_state_t accept,
	uint32_t ip_lo, uint32_t ip_hi)
{
	unsigned char a[4], b[4];
	fsm_state_t cur = root;
	int oct = 0;
	int k;

	assert(ip_lo <= ip_hi);

	a[0] = (unsigned char) ((ip_lo >> 24) & 0xffU);
	a[1] = (unsigned char) ((ip_lo >> 16) & 0xffU);
	a[2] = (unsigned char) ((ip_lo >> 8) & 0xffU);
	a[3] = (unsigned char) (ip_lo & 0xffU);

	b[0] = (unsigned char) ((ip_hi >> 24) & 0xffU);
	b[1] = (unsigned char) ((ip_hi >> 16) & 0xffU);
	b[2] = (unsigned char) ((ip_hi >> 8) & 0xffU);
	b[3] = (unsigned char) (ip_hi & 0xffU);

	while (oct < 4 && a[oct] == b[oct]) {
		fsm_state_t nxt;

		if (!fsm_addstate(fsm, &nxt)
			|| !fsm_addedge_literal(fsm, cur, nxt, (char) a[oct]))
		{
			return 0;
		}
		cur = nxt;
		oct++;
	}
	if (oct == 4) {
		return fsm_addedge_epsilon(fsm, cur, accept);
	}
	if (oct == 3) {
		return fsm_addedge_range(fsm, cur, accept, (char) a[3], (char) b[3]);
	}

	if (a[oct] + 1 < b[oct]) {
		fsm_state_t mid = cur;

		for (k = oct; k < 4; k++) {
			fsm_state_t dst = accept;
			unsigned char lo = (k == oct) ? (unsigned char) (a[oct] + 1) : 0U;
			unsigned char hi = (k == oct) ? (unsigned char) (b[oct] - 1) : 255U;

			if (k < 3 && !fsm_addstate(fsm, &dst)) {
				return 0;
			}
			if (!fsm_addedge_range(fsm, mid, dst, (char) lo, (char) hi)) {
				return 0;
			}
			mid = dst;
		}
	}

	{
		fsm_state_t lo_s;

		if (!fsm_addstate(fsm, &lo_s)
			|| !fsm_addedge_literal(fsm, cur, lo_s, (char) a[oct]))
		{
			return 0;
		}
		for (k = oct + 1; k < 4; k++) {
			fsm_state_t dst = accept;

			if (k < 3 && !fsm_addstate(fsm, &dst)) {
				return 0;
			}
			if (!fsm_addedge_range(fsm, lo_s, dst, (char) a[k], '\xff')) {
				return 0;
			}
			lo_s = dst;
		}
	}

	{
		fsm_state_t hi_s;

		if (!fsm_addstate(fsm, &hi_s)
			|| !fsm_addedge_literal(fsm, cur, hi_s, (char) b[oct]))
		{
			return 0;
		}
		for (k = oct + 1; k < 4; k++) {
			fsm_state_t dst = accept;

			if (k < 3 && !fsm_addstate(fsm, &dst)) {
				return 0;
			}
			if (!fsm_addedge_range(fsm, hi_s, dst, '\x00', (char) b[k])) {
				return 0;
			}
			hi_s = dst;
		}
	}

	return 1;
}

static struct fsm *
make_ipv4_table_fsm(const uint32_t *lo_ips, const uint32_t *hi_ips, size_t n_ranges)
{
	struct fsm *f;
	fsm_state_t root, acc;
	size_t i;

	f = fsm_new(NULL);
	if (f == NULL || !fsm_addstate(f, &root) || !fsm_addstate(f, &acc)) {
		fsm_free(f);
		return NULL;
	}
	fsm_setstart(f, root);
	fsm_setend(f, acc, 1);

	for (i = 0; i < n_ranges; i++) {
		if (!add_ipv4_range(f, root, acc, lo_ips[i], hi_ips[i])) {
			fsm_free(f);
			return NULL;
		}
	}

	return f;
}

static int
init_ipv4_fixture(size_t n_ranges, uint32_t **lo_out, uint32_t **hi_out)
{
	uint32_t *lo_ips, *hi_ips;
	uint32_t cur_ip = UINT32_C(0x01000000);
	uint32_t st = UINT32_C(0xdeadbeef);
	size_t i;

	lo_ips = malloc(n_ranges * sizeof *lo_ips);
	hi_ips = malloc(n_ranges * sizeof *hi_ips);
	if (lo_ips == NULL || hi_ips == NULL) {
		free(lo_ips);
		free(hi_ips);
		return 0;
	}

	for (i = 0; i < n_ranges; i++) {
		uint32_t span = 32U + (xs32(&st) & 0x3ffffU);

		lo_ips[i] = cur_ip;
		hi_ips[i] = cur_ip + span;
		cur_ip += span + 17U + ((st >> 18) & 0xffU);
	}

	*lo_out = lo_ips;
	*hi_out = hi_ips;
	return 1;
}

static int
bench_ipv4_range_table(const uint32_t *lo_ips, const uint32_t *hi_ips,
	size_t n_ranges, unsigned long iters, double *us_out)
{
	unsigned long it;
	double t0, dt;

	t0 = now_us();
	for (it = 0; it < iters; it++) {
		struct fsm *f = make_ipv4_table_fsm(lo_ips, hi_ips, n_ranges);

		if (f == NULL) {
			return 0;
		}
		fsm_free(f);
	}
	dt = now_us() - t0;

	*us_out = dt / (double) iters;
	return 1;
}

static const char *const pcre_class_pats[] = {
	"^[A-Za-z_][A-Za-z0-9_.-]{31}$",
	"^[\\x20-\\x7e]{32}$",
	"^[^\\x00-\\x1f\\x7f\"\\\\]{32}$",
	"^([0-9]{1,3}\\.){3}[0-9]{1,3}$",
	"^(?:[\\xc2-\\xdf][\\x80-\\xbf]|[\\xe0-\\xef][\\x80-\\xbf]{2}|[\\xf0-\\xf4][\\x80-\\xbf]{3}){8}$",
	"^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,6}$"
};

#define PCRE_CLASS_PAT_COUNT (sizeof pcre_class_pats / sizeof *pcre_class_pats)

static int
bench_re_comp_classes(unsigned long iters, double *us_out)
{
	unsigned long it;
	double t0, dt;

	t0 = now_us();
	for (it = 0; it < iters; it++) {
		size_t p;

		for (p = 0; p < PCRE_CLASS_PAT_COUNT; p++) {
			const char *s = pcre_class_pats[p];
			struct re_err err;
			struct fsm *f;

			f = re_comp(RE_PCRE, fsm_sgetc, &s, NULL, 0, &err);
			if (f == NULL) {
				return 0;
			}
			fsm_free(f);
		}
	}
	dt = now_us() - t0;

	*us_out = dt / (double) iters;
	return 1;
}

static const char *const ast_charset_re =
	"^(?:[0-9A-Za-z_.-]|[\\x20-\\x2f\\x3a-\\x40\\x5b-\\x60\\x7b-\\x7e]|"
	"[\\x80-\\xbf]|[\\xc2-\\xdf]|[\\xe0-\\xef]|[\\xf0-\\xf4]|"
	"[^\\x00-\\x08\\x0b-\\x1f\\x7f]|[a-fA-F0-9]|[0-7]|[A-Z]|[a-z])+$";

static char *
capture_ast_pcre(const struct ast *ast, size_t *len_out)
{
	static const struct fsm_options opt;
	FILE *tf;
	char *out;

	tf = tmpfile();
	if (tf == NULL) {
		return NULL;
	}
	if (ast_print(tf, ast, &opt, 0, AST_PRINT_PCRE) != 0) {
		fclose(tf);
		return NULL;
	}
	out = slurp_tmpfile(tf, len_out);
	fclose(tf);
	return out;
}

static int
bench_ast_print_pcre(const struct ast *ast, FILE *sink, unsigned long iters,
	double *us_out)
{
	static const struct fsm_options opt;
	unsigned long it;
	double t0, dt;

	t0 = now_us();
	for (it = 0; it < iters; it++) {
		rewind(sink);
		if (ast_print(sink, ast, &opt, 0, AST_PRINT_PCRE) != 0) {
			return 0;
		}
	}
	dt = now_us() - t0;

	*us_out = dt / (double) iters;
	return 1;
}

/*
 * Minimized DFAs are canonical up to state isomorphism, so determinising and
 * minimising the NFAs produced under each bm_range() candidate allows exact
 * structural equivalence checks via fsm_equal() alongside execution probes
 * and byte-identical C, VMC, and PCRE output comparisons.
 */
static int
verify_all_impls_end_to_end(void)
{
	static const uint32_t ip_lo[5] = {
		UINT32_C(0x0a000001), /* 10.0.0.1 */
		UINT32_C(0x0a00000a), /* 10.0.0.10 */
		UINT32_C(0xac100000), /* 172.16.0.0 */
		UINT32_C(0xc0a80164), /* 192.168.1.100 */
		UINT32_C(0xcb007107)  /* 203.0.113.7 */
	};
	static const uint32_t ip_hi[5] = {
		UINT32_C(0x0a000001), /* 10.0.0.1 */
		UINT32_C(0x0a0000fa), /* 10.0.0.250 */
		UINT32_C(0xac1fffff), /* 172.31.255.255 */
		UINT32_C(0xc0a80432), /* 192.168.4.50 */
		UINT32_C(0xcb0073c8)  /* 203.0.115.200 */
	};
	static const struct {
		unsigned char octets[4];
		int expect;
	} ip_probes[] = {
		{ { 10,  0,   0,   0   }, 0 },
		{ { 10,  0,   0,   1   }, 1 },
		{ { 10,  0,   0,   9   }, 0 },
		{ { 10,  0,   0,   10  }, 1 },
		{ { 10,  0,   0,   250 }, 1 },
		{ { 10,  0,   0,   251 }, 0 },
		{ { 172, 15,  255, 255 }, 0 },
		{ { 172, 16,  0,   0   }, 1 },
		{ { 172, 31,  255, 255 }, 1 },
		{ { 172, 32,  0,   0   }, 0 },
		{ { 192, 168, 1,   99  }, 0 },
		{ { 192, 168, 1,   100 }, 1 },
		{ { 192, 168, 4,   50  }, 1 },
		{ { 192, 168, 4,   51  }, 0 }
	};
#define STR_PROBE(lit, exp) { (lit), sizeof (lit) - 1, (exp) }
	static const struct {
		const char *s;
		size_t len;
		int expect;
	} utf8_probes[] = {
		STR_PROBE("abcd", 1),
		STR_PROBE("\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80!", 1),
		STR_PROBE("ab\"d", 0),                 /* unescaped '"' */
		STR_PROBE("\xc0\xaf""abc", 0),         /* overlong 2-byte */
		STR_PROBE("\xed\xa0\x80""abc", 0),     /* UTF-16 surrogate */
		STR_PROBE("\xf4\x90\x80\x80""abc", 0)  /* > U+10FFFF */
	};
#undef STR_PROBE

	struct fsm *ref_utf8 = NULL;
	struct fsm *ref_fixture_dfa = NULL;
	FILE *ref_fsm_file = NULL;
	char *ref_c = NULL, *ref_vmc = NULL, *ref_pcre = NULL;
	size_t ref_c_len = 0, ref_vmc_len = 0, ref_pcre_len = 0;
	struct fsm *ref_ip = NULL;
	struct fsm *ref_re[PCRE_CLASS_PAT_COUNT];
	struct ast *ast = NULL;
	size_t p, m;
	int ok = 0;

	memset(ref_re, 0, sizeof ref_re);

	bm_range_shim_set(bm_range_loop_set);

	ref_utf8 = make_utf8_chain(4);
	if (ref_utf8 == NULL || !fsm_determinise(ref_utf8) || !fsm_minimise(ref_utf8)) {
		goto cleanup;
	}

	if (!prepare_fsm_fixture(4, &ref_fixture_dfa, &ref_fsm_file)) {
		goto cleanup;
	}

	if (!capture_ir_codegen(ref_fixture_dfa, &ref_c, &ref_c_len,
		&ref_vmc, &ref_vmc_len))
	{
		goto cleanup;
	}

	ref_ip = make_ipv4_table_fsm(ip_lo, ip_hi, sizeof ip_lo / sizeof *ip_lo);
	if (ref_ip == NULL || !fsm_determinise(ref_ip) || !fsm_minimise(ref_ip)) {
		goto cleanup;
	}

	for (p = 0; p < PCRE_CLASS_PAT_COUNT; p++) {
		const char *s = pcre_class_pats[p];
		struct re_err err;

		ref_re[p] = re_comp(RE_PCRE, fsm_sgetc, &s, NULL, 0, &err);
		if (ref_re[p] == NULL
			|| !fsm_determinise(ref_re[p])
			|| !fsm_minimise(ref_re[p]))
		{
			goto cleanup;
		}
	}

	{
		const char *s = ast_charset_re;
		struct re_err err;

		ast = re_parse(RE_PCRE, fsm_sgetc, &s, 0, &err, NULL);
		if (ast == NULL) {
			goto cleanup;
		}
		ref_pcre = capture_ast_pcre(ast, &ref_pcre_len);
		if (ref_pcre == NULL) {
			goto cleanup;
		}
	}

	for (m = 0; m < bm_range_impl_count; m++) {
		const struct bm_range_impl *im = &bm_range_impls[m];
		struct fsm *got_utf8, *got_parsed, *got_ip;
		char *got_c = NULL, *got_vmc = NULL, *got_pcre = NULL;
		size_t got_c_len = 0, got_vmc_len = 0, got_pcre_len = 0;
		size_t i;

		bm_range_shim_set(im->fn);

		got_utf8 = make_utf8_chain(4);
		if (got_utf8 == NULL
			|| !fsm_determinise(got_utf8)
			|| !fsm_minimise(got_utf8)
			|| fsm_equal(got_utf8, ref_utf8) != 1)
		{
			fprintf(stderr, "FAIL %s: utf8_dfa_api mismatch\n", im->name);
			fsm_free(got_utf8);
			goto cleanup;
		}
		for (i = 0; i < sizeof utf8_probes / sizeof *utf8_probes; i++) {
			int r = exec_bytes(got_utf8, utf8_probes[i].s, utf8_probes[i].len);

			if (r != utf8_probes[i].expect) {
				fprintf(stderr, "FAIL %s: utf8_dfa_api probe %zu\n", im->name, i);
				fsm_free(got_utf8);
				goto cleanup;
			}
		}
		fsm_free(got_utf8);

		rewind(ref_fsm_file);
		got_parsed = fsm_parse(ref_fsm_file, NULL);
		if (got_parsed == NULL
			|| !fsm_determinise(got_parsed)
			|| !fsm_minimise(got_parsed)
			|| fsm_equal(got_parsed, ref_fixture_dfa) != 1)
		{
			fprintf(stderr, "FAIL %s: fsm_parse_ranges mismatch\n", im->name);
			fsm_free(got_parsed);
			goto cleanup;
		}
		fsm_free(got_parsed);

		if (!capture_ir_codegen(ref_fixture_dfa, &got_c, &got_c_len,
			&got_vmc, &got_vmc_len)
			|| got_c_len != ref_c_len
			|| memcmp(got_c, ref_c, ref_c_len) != 0
			|| got_vmc_len != ref_vmc_len
			|| memcmp(got_vmc, ref_vmc, ref_vmc_len) != 0)
		{
			fprintf(stderr, "FAIL %s: ir_codegen_c_vmc mismatch\n", im->name);
			free(got_c);
			free(got_vmc);
			goto cleanup;
		}
		free(got_c);
		free(got_vmc);

		got_ip = make_ipv4_table_fsm(ip_lo, ip_hi, sizeof ip_lo / sizeof *ip_lo);
		if (got_ip == NULL
			|| !fsm_determinise(got_ip)
			|| !fsm_minimise(got_ip)
			|| fsm_equal(got_ip, ref_ip) != 1)
		{
			fprintf(stderr, "FAIL %s: ipv4_range_table mismatch\n", im->name);
			fsm_free(got_ip);
			goto cleanup;
		}
		for (i = 0; i < sizeof ip_probes / sizeof *ip_probes; i++) {
			int r = exec_bytes(got_ip, ip_probes[i].octets, 4);

			if (r != ip_probes[i].expect) {
				fprintf(stderr, "FAIL %s: ipv4_range_table probe %zu\n",
					im->name, i);
				fsm_free(got_ip);
				goto cleanup;
			}
		}
		fsm_free(got_ip);

		for (p = 0; p < PCRE_CLASS_PAT_COUNT; p++) {
			const char *s = pcre_class_pats[p];
			struct re_err err;
			struct fsm *got_re;

			got_re = re_comp(RE_PCRE, fsm_sgetc, &s, NULL, 0, &err);
			if (got_re == NULL
				|| !fsm_determinise(got_re)
				|| !fsm_minimise(got_re)
				|| fsm_equal(got_re, ref_re[p]) != 1)
			{
				fprintf(stderr, "FAIL %s: re_comp_classes[%zu] mismatch\n",
					im->name, p);
				fsm_free(got_re);
				goto cleanup;
			}
			fsm_free(got_re);
		}

		got_pcre = capture_ast_pcre(ast, &got_pcre_len);
		if (got_pcre == NULL
			|| got_pcre_len != ref_pcre_len
			|| memcmp(got_pcre, ref_pcre, ref_pcre_len) != 0)
		{
			fprintf(stderr, "FAIL %s: ast_print_pcre mismatch\n", im->name);
			free(got_pcre);
			goto cleanup;
		}
		free(got_pcre);
	}

	ok = 1;

cleanup:
	bm_range_shim_set(NULL);
	free(ref_pcre);
	if (ast != NULL) {
		ast_free(ast);
	}
	for (p = 0; p < PCRE_CLASS_PAT_COUNT; p++) {
		fsm_free(ref_re[p]);
	}
	fsm_free(ref_ip);
	free(ref_c);
	free(ref_vmc);
	if (ref_fsm_file != NULL) {
		fclose(ref_fsm_file);
	}
	fsm_free(ref_fixture_dfa);
	fsm_free(ref_utf8);
	return ok;
}

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s [-q] [-l] [-i substr] [-t trials] [scale]\n", prog);
}

enum workload_id {
	WKLD_UTF8_DFA_API,
	WKLD_FSM_PARSE_RANGES,
	WKLD_IR_CODEGEN_C_VMC,
	WKLD_IPV4_RANGE_TABLE,
	WKLD_RE_COMP_CLASSES,
	WKLD_AST_PRINT_PCRE,
	WKLD_TOTAL,
	WKLD_SLOT_COUNT
};

#define WKLD_COUNT WKLD_TOTAL

struct wkld_meta {
	const char *name;
	const char *site;
	int prec;
};

static const struct wkld_meta wklds[WKLD_SLOT_COUNT] = {
	[WKLD_UTF8_DFA_API]     = { "utf8_dfa_api",     "src/libfsm/edge.c (fsm_addedge_range)",  1 },
	[WKLD_FSM_PARSE_RANGES] = { "fsm_parse_ranges", "src/libfsm/parser.act (.fsm x..y)",      1 },
	[WKLD_IR_CODEGEN_C_VMC] = { "ir_codegen_c_vmc", "src/libfsm/print/ir.c (find_coverage)",  0 },
	[WKLD_IPV4_RANGE_TABLE] = { "ipv4_range_table", "examples/iprange (IPv4 range table)",    1 },
	[WKLD_RE_COMP_CLASSES]  = { "re_comp_classes",  "src/libre/ast_compile.c (PCRE classes)", 1 },
	[WKLD_AST_PRINT_PCRE]   = { "ast_print_pcre",   "src/libre/ast.c (ast_expr_charset)",     2 },
	[WKLD_TOTAL]            = { "TOTAL",            "sum across all six workloads",           0 }
};

static int
run_one_sample(unsigned long scale, size_t dfa_steps, FILE *fsm_file,
	const struct fsm *dfa, const uint32_t *lo_ips, const uint32_t *hi_ips,
	size_t n_ips, const struct ast *ast, FILE *sink, double out[WKLD_SLOT_COUNT])
{
	size_t w;
	double sum = 0.0;

	if (!bench_utf8_dfa_api(20UL * scale, dfa_steps * 2U, &out[WKLD_UTF8_DFA_API])
		|| !bench_fsm_parse_ranges(fsm_file, 5UL * scale, &out[WKLD_FSM_PARSE_RANGES])
		|| !bench_ir_codegen(dfa, sink, 3UL * scale, &out[WKLD_IR_CODEGEN_C_VMC])
		|| !bench_ipv4_range_table(lo_ips, hi_ips, n_ips, 10UL * scale,
			&out[WKLD_IPV4_RANGE_TABLE])
		|| !bench_re_comp_classes(5UL * scale, &out[WKLD_RE_COMP_CLASSES])
		|| !bench_ast_print_pcre(ast, sink, 100UL * scale, &out[WKLD_AST_PRINT_PCRE]))
	{
		return 0;
	}

	for (w = 0; w < WKLD_COUNT; w++) {
		sum += out[w];
	}
	out[WKLD_TOTAL] = sum;
	return 1;
}

static void
print_wkld_group(const char *heading, const enum workload_id *cols, size_t ncols,
	const size_t *sel, size_t nsel, const double *samples, double *scratch,
	size_t trials)
{
	size_t s, c;

	printf("\n--- %s ---\n", heading);
	printf("%-14s", "implementation");
	for (c = 0; c < ncols; c++) {
		printf("  %35s", wklds[cols[c]].name);
	}
	printf("\n");

	for (s = 0; s < nsel; s++) {
		printf("%-14s", bm_range_impls[sel[s]].name);
		for (c = 0; c < ncols; c++) {
			enum workload_id w = cols[c];
			char cell[64];

			format_stats(cell, sizeof cell,
				&samples[(w * nsel + s) * trials], scratch, trials,
				wklds[w].prec);
			printf("  %35s", cell);
		}
		printf("\n");
	}
}

int
main(int argc, char *argv[])
{
	static const enum workload_id grp_range[] = {
		WKLD_UTF8_DFA_API,
		WKLD_IPV4_RANGE_TABLE,
		WKLD_FSM_PARSE_RANGES
	};
	static const enum workload_id grp_cgen[] = {
		WKLD_IR_CODEGEN_C_VMC,
		WKLD_RE_COMP_CLASSES,
		WKLD_AST_PRINT_PCRE,
		WKLD_TOTAL
	};

	const char *prog = argv[0];
	unsigned long scale = DEF_SCALE;
	unsigned long trials_ul = DEF_TRIALS;
	const char *impl_filter = NULL;
	int quiet = 0;
	int rc = EXIT_FAILURE;
	size_t trials;
	size_t dfa_steps = 64U;
	size_t n_ips = 1000U;
	size_t sel[MAX_IMPLS];
	size_t order[MAX_IMPLS];
	size_t nsel = 0;
	struct fsm *dfa = NULL;
	FILE *fsm_file = NULL;
	FILE *sink = NULL;
	struct ast *ast = NULL;
	uint32_t *lo_ips = NULL, *hi_ips = NULL;
	double *samples = NULL;
	double *scratch = NULL;
	uint32_t rng = UINT32_C(0x20261123);
	size_t i, w, r, k;

	bm_range_impls_init();

	{
		int c;

		while (c = getopt(argc, argv, "qli:t:"), c != -1) {
			char *end;
			unsigned long v;

			switch (c) {
			case 'q':
				quiet = 1;
				break;

			case 'l':
				for (i = 0; i < bm_range_impl_count; i++) {
					printf("%-14s %s\n",
						bm_range_impls[i].name, bm_range_impls[i].desc);
				}
				return EXIT_SUCCESS;

			case 'i':
				impl_filter = optarg;
				break;

			case 't':
				errno = 0;
				v = strtoul(optarg, &end, 10);
				if (errno != 0 || end == optarg || *end != '\0' || v == 0) {
					usage(prog);
					return EXIT_FAILURE;
				}
				trials_ul = v;
				break;

			case '?':
			default:
				usage(prog);
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc > 1) {
		usage(prog);
		return EXIT_FAILURE;
	}
	if (argc == 1) {
		char *end;
		unsigned long v;

		errno = 0;
		v = strtoul(argv[0], &end, 10);
		if (errno != 0 || end == argv[0] || *end != '\0' || v == 0) {
			usage(prog);
			return EXIT_FAILURE;
		}
		scale = v;
	}

	for (i = 0; i < bm_range_impl_count && nsel < MAX_IMPLS; i++) {
		if (impl_filter == NULL || strstr(bm_range_impls[i].name, impl_filter) != NULL) {
			sel[nsel] = i;
			order[nsel] = nsel;
			nsel++;
		}
	}
	if (nsel == 0) {
		fprintf(stderr, "no implementations matched filter '%s'\n", impl_filter);
		return EXIT_FAILURE;
	}

	if (!verify_all_impls_end_to_end()) {
		return EXIT_FAILURE;
	}

	if (quiet) {
		return EXIT_SUCCESS;
	}

	trials = (size_t) trials_ul;

	bm_range_shim_set(NULL);
	if (!prepare_fsm_fixture(dfa_steps, &dfa, &fsm_file)) {
		fprintf(stderr, "prepare_fsm_fixture failed\n");
		return EXIT_FAILURE;
	}

	if (!init_ipv4_fixture(n_ips, &lo_ips, &hi_ips)) {
		fprintf(stderr, "init_ipv4_fixture failed\n");
		goto done;
	}

	sink = open_sink();
	if (sink == NULL) {
		perror("open_sink");
		goto done;
	}

	{
		const char *p = ast_charset_re;
		struct re_err err;

		ast = re_parse(RE_PCRE, fsm_sgetc, &p, 0, &err, NULL);
		if (ast == NULL) {
			goto done;
		}
	}

	samples = calloc(WKLD_SLOT_COUNT * nsel * trials, sizeof *samples);
	scratch = malloc(trials * sizeof *scratch);
	if (samples == NULL || scratch == NULL) {
		perror("calloc");
		goto done;
	}

	/* Untimed warmup pass across selected candidates */
	for (k = 0; k < nsel; k++) {
		double warm[WKLD_SLOT_COUNT];

		bm_range_shim_set(bm_range_impls[sel[k]].fn);
		if (!run_one_sample(1UL, dfa_steps, fsm_file, dfa, lo_ips, hi_ips,
			n_ips, ast, sink, warm))
		{
			goto done;
		}
	}

	/*
	 * Shuffle implementation order on every round so slow clock or allocator
	 * drift distributes evenly across candidates rather than biasing by index.
	 */
	for (r = 0; r < trials; r++) {
		shuffle_indices(order, nsel, &rng);

		for (k = 0; k < nsel; k++) {
			size_t s = order[k];
			const struct bm_range_impl *im = &bm_range_impls[sel[s]];
			double cur[WKLD_SLOT_COUNT];

			bm_range_shim_set(im->fn);
			if (!run_one_sample(scale, dfa_steps, fsm_file, dfa, lo_ips, hi_ips,
				n_ips, ast, sink, cur))
			{
				fprintf(stderr, "benchmark failed for %s\n", im->name);
				goto done;
			}
			for (w = 0; w < WKLD_SLOT_COUNT; w++) {
				samples[(w * nsel + s) * trials + r] = cur[w];
			}
		}
	}

	bm_range_shim_set(NULL);

	printf("=== END-TO-END LIBFSM WORKLOADS (us/iter, scale=%lu, %zu shuffled trials) ===\n",
		scale, trials);
	printf("format: p50 [p25/p75/max] (sigma)\n");
	for (w = 0; w < WKLD_COUNT; w++) {
		printf("  %-18s %s\n", wklds[w].name, wklds[w].site);
	}

	print_wkld_group("RANGE CONSTRUCTION & PARSING", grp_range,
		sizeof grp_range / sizeof *grp_range, sel, nsel, samples, scratch,
		trials);
	print_wkld_group("CODEGEN, REGEX COMPILATION & SUITE TOTAL", grp_cgen,
		sizeof grp_cgen / sizeof *grp_cgen, sel, nsel, samples, scratch,
		trials);

	rc = EXIT_SUCCESS;

done:
	bm_range_shim_set(NULL);
	free(scratch);
	free(samples);
	free(lo_ips);
	free(hi_ips);
	if (sink != NULL) {
		fclose(sink);
	}
	if (ast != NULL) {
		ast_free(ast);
	}
	if (fsm_file != NULL) {
		fclose(fsm_file);
	}
	fsm_free(dfa);
	return rc;
}
