/*
 * Copyright 2026 Devon H. O'Dell
 *
 * See LICENCE for the full copyright terms.
 */

#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif

#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <adt/bitmap.h>

#include "bitmap_impls.h"

#define DEF_OPS    50000UL
#define DEF_TRIALS 21UL
#define VERIFY_OPS 4096UL
#define RING_LEN   4096U
#define MAX_IMPLS  32U
#define MAX_PATS   32U
#define MODE_COUNT 3U
#define CACHE_WS   (48U * 1024U / sizeof (uint64_t))

#define ARRAY_LEN(a) (sizeof (a) / sizeof *(a))

enum bench_mode {
	MODE_THROUGHPUT = 1 << 0,
	MODE_LATENCY    = 1 << 1,
	MODE_CACHE      = 1 << 2,
	MODE_ALL        = MODE_THROUGHPUT | MODE_LATENCY | MODE_CACHE
};

struct range_pair {
	unsigned char x;
	unsigned char y;
};

struct pattern_spec {
	const char *name;
	struct range_pair *pairs;
	size_t n;
};

static volatile uint64_t bench_sink;

/*
 * Loaded once before timed latency loops so the compiler cannot fold away
 * the zero mask that chains each 4-word bitmap read into the next pair load.
 */
static volatile uint64_t dep_zero;

static double
now_ns(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	return (double) ts.tv_sec * 1e9 + (double) ts.tv_nsec;
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

/*
 * Independent calls accumulating into a live bitmap and resetting every
 * eight ranges; next-iteration inputs do not depend on prior outputs, so
 * out-of-order execution can overlap calls as in multi-range character
 * class construction.
 */
static double
measure_throughput(bm_range_fn *fn, const struct range_pair *pairs,
	size_t npairs, unsigned long total_ops, uint64_t *checksum_out)
{
	struct bm bm;
	uint64_t acc = 0;
	unsigned long i;
	size_t idx = 0;
	double t0, dt;

	memset(&bm, 0, sizeof bm);

	t0 = now_ns();
	for (i = 0; i < total_ops; i++) {
		const struct range_pair p = pairs[idx];

		if ((i & 7U) == 0) {
			memset(&bm, 0, sizeof bm);
		}
		fn(&bm, p.x, p.y);
		acc = (acc >> 7) ^ (acc << 57)
			^ bm.map[0] ^ (bm.map[1] >> 1)
			^ (bm.map[2] >> 2) ^ (bm.map[3] >> 3);
		if (++idx == npairs) {
			idx = 0;
		}
	}
	dt = now_ns() - t0;

	*checksum_out = acc;
	return dt / (double) total_ops;
}

/*
 * Zeroes a stack bitmap with vector stores, calls fn(), and feeds all four
 * words into the next pair index. Measures full round-trip latency and
 * exposes partial store-to-load forwarding stalls when fn() writes single
 * 64-bit words over 128-bit zero stores.
 */
static double
measure_latency(bm_range_fn *fn, const struct range_pair *pairs, size_t npairs,
	unsigned long total_ops, uint64_t *checksum_out)
{
	struct bm bm;
	uint64_t mask = dep_zero;
	uint64_t acc = 0;
	unsigned long i;
	size_t idx = 0;
	double t0, dt;

	t0 = now_ns();
	for (i = 0; i < total_ops; i++) {
		const struct range_pair p = pairs[idx];
		uint64_t v;

		memset(&bm, 0, sizeof bm);
		fn(&bm, p.x, p.y);
		v = bm.map[0] ^ bm.map[1] ^ bm.map[2] ^ bm.map[3];
		acc = (acc >> 5) ^ (acc << 59) ^ v;
		idx += 1U + (size_t) (v & mask);
		if (idx >= npairs) {
			idx = 0;
		}
	}
	dt = now_ns() - t0;

	*checksum_out = acc;
	return dt / (double) total_ops;
}

/*
 * Interleaves the dependent latency chain with touches across a 48 KiB
 * working set (exceeding a 32 KiB L1d cache) so table-based implementations
 * experience L1d pressure from surrounding NFA/DFA structures.
 */
static double
measure_cache_contended(bm_range_fn *fn, const struct range_pair *pairs,
	size_t npairs, uint64_t *ws, unsigned long total_ops,
	uint64_t *checksum_out)
{
	struct bm bm;
	uint64_t mask = dep_zero;
	uint64_t acc = 0;
	unsigned long i;
	size_t idx = 0;
	size_t widx = 0;
	double t0, dt;

	memset(ws, 0, CACHE_WS * sizeof *ws);

	t0 = now_ns();
	for (i = 0; i < total_ops; i++) {
		const struct range_pair p = pairs[idx];
		uint64_t v;

		ws[widx] ^= i;
		ws[(widx + 8U) % CACHE_WS] ^= p.x;
		ws[(widx + 16U) % CACHE_WS] ^= p.y;
		ws[(widx + 24U) % CACHE_WS] ^= acc;
		widx = (widx + 32U) % CACHE_WS;

		memset(&bm, 0, sizeof bm);
		fn(&bm, p.x, p.y);
		v = bm.map[0] ^ bm.map[1] ^ bm.map[2] ^ bm.map[3];
		acc = (acc >> 5) ^ (acc << 59) ^ v;
		idx += 1U + (size_t) ((v ^ ws[widx]) & mask);
		if (idx >= npairs) {
			idx = 0;
		}
	}
	dt = now_ns() - t0;

	*checksum_out = acc;
	return dt / (double) total_ops;
}

static double
dispatch_mode(enum bench_mode mode, bm_range_fn *fn,
	const struct range_pair *pairs, size_t npairs, uint64_t *ws,
	unsigned long ops, uint64_t *csum_out)
{
	switch (mode) {
	case MODE_THROUGHPUT:
		return measure_throughput(fn, pairs, npairs, ops, csum_out);
	case MODE_LATENCY:
		return measure_latency(fn, pairs, npairs, ops, csum_out);
	case MODE_CACHE:
		return measure_cache_contended(fn, pairs, npairs, ws, ops, csum_out);
	default:
		*csum_out = 0;
		return 0.0;
	}
}

static struct range_pair *
make_cyclic(const struct range_pair *seed, size_t seed_n, size_t dst_n)
{
	struct range_pair *out;
	size_t i;

	out = malloc(dst_n * sizeof *out);
	if (out == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < dst_n; i++) {
		out[i] = seed[i % seed_n];
	}

	return out;
}

static void
add_cyclic(struct pattern_spec *pat, const char *name,
	const struct range_pair *seed, size_t seed_n)
{
	pat->name  = name;
	pat->pairs = make_cyclic(seed, seed_n, RING_LEN);
	pat->n     = RING_LEN;
}

static size_t
build_patterns(struct pattern_spec *pats)
{
	static const struct range_pair s_1bit_fixed[] = { { '_', '_' } };
	static const struct range_pair s_tiny[] = {
		{ '0', '1' }, { 0x0a, 0x0d }, { 'a', 'd' }, { 'A', 'C' },
		{ 0xe1, 0xec }, { 0xf1, 0xf3 }, { 0x80, 0x83 }, { 0x3a, 0x3d }
	};
	static const struct range_pair s_word0[] = {
		{ 0, 31 }, { '0', '9' }, { 33, 47 }, { 58, 63 },
		{ 9, 13 }, { '0', '7' }, { 1, 62 }, { 32, 63 }
	};
	static const struct range_pair s_word1[] = {
		{ 'A', 'Z' }, { 'a', 'z' }, { 'A', 'z' }, { 91, 96 },
		{ 'A', 'F' }, { 'a', 'f' }, { 64, 126 }, { 'g', 'z' }
	};
	static const struct range_pair s_utf8[] = {
		{ 0x80, 0xbf }, { 0x80, 0x9f }, { 0xa0, 0xbf }, { 0xc2, 0xdf },
		{ 0xe0, 0xef }, { 0xf0, 0xf4 }, { 0x80, 0x8f }, { 0x90, 0xbf }
	};
	static const struct range_pair s_aligned[] = {
		{ 0, 63 }, { 64, 127 }, { 128, 191 }, { 192, 255 },
		{ 0, 127 }, { 64, 191 }, { 128, 255 }, { 0, 255 }
	};
	static const struct range_pair s_straddle2[] = {
		{ 63, 64 }, { 62, 65 }, { 127, 128 }, { 126, 129 },
		{ 191, 192 }, { 190, 193 }, { 60, 67 }, { 188, 195 }
	};
	static const struct range_pair s_span2w[] = {
		{ 0x20, 0x7e }, { '0', 'z' }, { 16, 112 }, { 80, 176 },
		{ 140, 250 }, { 1, 126 }, { 65, 190 }, { 129, 254 }
	};
	static const struct range_pair s_span3w[] = {
		{ 10, 150 }, { 48, 185 }, { 63, 192 }, { 70, 240 },
		{ 0, 191 }, { 64, 255 }, { 32, 160 }, { 97, 250 }
	};
	static const struct range_pair s_span4w[] = {
		{ 1, 255 }, { 0, 254 }, { 11, 255 }, { 32, 255 },
		{ 1, 254 }, { 14, 255 }, { 0, 200 }, { 63, 193 }
	};
	static const struct range_pair s_full[] = { { 0x00, 0xff } };

	struct range_pair s_1bit_sweep[UCHAR_MAX + 1];
	size_t i, n = 0;

	for (i = 0; i <= UCHAR_MAX; i++) {
		s_1bit_sweep[i].x = (unsigned char) i;
		s_1bit_sweep[i].y = (unsigned char) i;
	}

	add_cyclic(&pats[n++], "1bit_fixed",   s_1bit_fixed, ARRAY_LEN(s_1bit_fixed));
	add_cyclic(&pats[n++], "1bit_sweep",   s_1bit_sweep, ARRAY_LEN(s_1bit_sweep));
	add_cyclic(&pats[n++], "tiny_2..4b",   s_tiny,       ARRAY_LEN(s_tiny));
	add_cyclic(&pats[n++], "word0_only",   s_word0,      ARRAY_LEN(s_word0));
	add_cyclic(&pats[n++], "word1_ascii",  s_word1,      ARRAY_LEN(s_word1));
	add_cyclic(&pats[n++], "utf8_dfa",     s_utf8,       ARRAY_LEN(s_utf8));
	add_cyclic(&pats[n++], "word_aligned", s_aligned,    ARRAY_LEN(s_aligned));
	add_cyclic(&pats[n++], "straddle_2w",  s_straddle2,  ARRAY_LEN(s_straddle2));
	add_cyclic(&pats[n++], "span_2w_wide", s_span2w,     ARRAY_LEN(s_span2w));
	add_cyclic(&pats[n++], "span_3w",      s_span3w,     ARRAY_LEN(s_span3w));
	add_cyclic(&pats[n++], "span_4w_neg",  s_span4w,     ARRAY_LEN(s_span4w));
	add_cyclic(&pats[n++], "full_00..ff",  s_full,       ARRAY_LEN(s_full));

	{
		static const struct range_pair sw[] = {
			{ '0', '9' }, { 'A', 'Z' }, { 'a', 'z' }, { 0x80, 0xbf },
			{ 0xc2, 0xdf }, { 0xe0, 0xef }, { 0xf0, 0xf4 }, { '_', '_' }
		};
		struct range_pair *p;
		uint32_t st = UINT32_C(0x12345678);

		p = malloc(RING_LEN * sizeof *p);
		if (p == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < RING_LEN; i++) {
			uint32_t r = xs32(&st);

			if ((r % 10U) < 9U) {
				p[i] = sw[(r >> 8) % ARRAY_LEN(sw)];
			} else {
				p[i].x = (unsigned char) (r & 63U);
				p[i].y = (unsigned char) (64U + ((r >> 8) % 192U));
			}
		}
		pats[n].name  = "bimodal_90/10";
		pats[n].pairs = p;
		pats[n].n     = RING_LEN;
		n++;
	}

	{
		struct range_pair *p;
		uint32_t st = UINT32_C(0x9e3779b9);

		p = malloc(RING_LEN * sizeof *p);
		if (p == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < RING_LEN; i++) {
			uint32_t r = xs32(&st);
			unsigned char a = (unsigned char) (r & 0xffU);
			unsigned char b = (unsigned char) ((r >> 8) & 0xffU);

			if (a <= b) {
				p[i].x = a;
				p[i].y = b;
			} else {
				p[i].x = b;
				p[i].y = a;
			}
		}
		pats[n].name  = "uniform_rand";
		pats[n].pairs = p;
		pats[n].n     = RING_LEN;
		n++;
	}

	{
		size_t n_all = ((size_t) UCHAR_MAX + 1) * ((size_t) UCHAR_MAX + 2) / 2;
		struct range_pair *p;
		size_t x, y, k = 0;

		p = malloc(n_all * sizeof *p);
		if (p == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		for (x = 0; x <= UCHAR_MAX; x++) {
			for (y = x; y <= UCHAR_MAX; y++) {
				p[k].x = (unsigned char) x;
				p[k].y = (unsigned char) y;
				k++;
			}
		}
		pats[n].name  = "all_pairs";
		pats[n].pairs = p;
		pats[n].n     = n_all;
		n++;
	}

	return n;
}

static int
verify_mode_checksums(enum bench_mode mode, const size_t *sel, size_t nsel,
	const struct pattern_spec *pats, size_t npats, uint64_t *ws,
	unsigned long vops)
{
	size_t p, s;

	for (p = 0; p < npats; p++) {
		uint64_t ref_csum = 0;

		(void) dispatch_mode(mode, bm_range_loop_set, pats[p].pairs,
			pats[p].n, ws, vops, &ref_csum);

		for (s = 0; s < nsel; s++) {
			const struct bm_range_impl *im = &bm_range_impls[sel[s]];
			uint64_t csum = 0;

			(void) dispatch_mode(mode, im->fn, pats[p].pairs,
				pats[p].n, ws, vops, &csum);
			if (csum != ref_csum) {
				fprintf(stderr, "FAIL %s/%s: checksum mismatch\n",
					pats[p].name, im->name);
				return 0;
			}
			bench_sink ^= csum;
		}
	}

	return 1;
}

static void
print_impl_table(const char *heading, unsigned int modes,
	const size_t *sel, size_t nsel, const double *mode_series[MODE_COUNT],
	double *scratch, size_t trials)
{
	static const struct {
		enum bench_mode flag;
		const char *label;
	} cols[MODE_COUNT] = {
		{ MODE_THROUGHPUT, "throughput"          },
		{ MODE_LATENCY,    "dependent latency"   },
		{ MODE_CACHE,      "L1d cache-contended" }
	};
	size_t s, m;

	printf("\n--- %s ---\n", heading);
	printf("%-14s", "implementation");
	for (m = 0; m < MODE_COUNT; m++) {
		if (modes & cols[m].flag) {
			printf("  %36s", cols[m].label);
		}
	}
	printf("\n");

	for (s = 0; s < nsel; s++) {
		printf("%-14s", bm_range_impls[sel[s]].name);
		for (m = 0; m < MODE_COUNT; m++) {
			char cell[64];

			if (!(modes & cols[m].flag)) {
				continue;
			}
			format_stats(cell, sizeof cell,
				&mode_series[m][s * trials], scratch, trials, 2);
			printf("  %36s", cell);
		}
		printf("\n");
	}
}

static void
usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s [-q] [-l] [-v] [-i substr] [-m throughput|latency|cache|all]"
		" [-t trials] [ops]\n",
		prog);
}

int
main(int argc, char *argv[])
{
	static const enum bench_mode mode_flags[MODE_COUNT] = {
		MODE_THROUGHPUT,
		MODE_LATENCY,
		MODE_CACHE
	};

	const char *prog = argv[0];
	unsigned long ops = DEF_OPS;
	unsigned long trials_ul = DEF_TRIALS;
	unsigned int modes = MODE_ALL;
	const char *impl_filter = NULL;
	int quiet = 0;
	int verbose = 0;
	int rc = EXIT_FAILURE;
	size_t trials;
	size_t sel[MAX_IMPLS];
	size_t order[MAX_IMPLS];
	size_t nsel = 0;
	struct pattern_spec pats[MAX_PATS];
	size_t npats = 0;
	uint64_t *ws = NULL;
	double *pat_samples = NULL;
	double *mean_samples = NULL;
	double *scratch = NULL;
	uint32_t rng = UINT32_C(0x20261123);
	unsigned long vops;
	size_t i, m, p, r, k;

	bm_range_impls_init();

	{
		int c;

		while (c = getopt(argc, argv, "qlvi:m:t:"), c != -1) {
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

			case 'v':
				verbose = 1;
				break;

			case 'i':
				impl_filter = optarg;
				break;

			case 'm':
				if (strcmp(optarg, "throughput") == 0) {
					modes = MODE_THROUGHPUT;
				} else if (strcmp(optarg, "latency") == 0) {
					modes = MODE_LATENCY;
				} else if (strcmp(optarg, "cache") == 0) {
					modes = MODE_CACHE;
				} else if (strcmp(optarg, "all") == 0) {
					modes = MODE_ALL;
				} else {
					usage(prog);
					return EXIT_FAILURE;
				}
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
		ops = v;
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

	ws = calloc(CACHE_WS, sizeof *ws);
	if (ws == NULL) {
		perror("calloc");
		return EXIT_FAILURE;
	}

	npats = build_patterns(pats);
	vops = (ops < VERIFY_OPS) ? ops : VERIFY_OPS;

	for (m = 0; m < MODE_COUNT; m++) {
		if ((modes & mode_flags[m])
			&& !verify_mode_checksums(mode_flags[m], sel, nsel, pats, npats, ws, vops))
		{
			goto done;
		}
	}

	if (quiet) {
		rc = EXIT_SUCCESS;
		goto done;
	}

	trials = (size_t) trials_ul;
	pat_samples  = calloc(MODE_COUNT * npats * nsel * trials, sizeof *pat_samples);
	mean_samples = calloc(MODE_COUNT * nsel * trials, sizeof *mean_samples);
	scratch      = malloc(trials * sizeof *scratch);
	if (pat_samples == NULL || mean_samples == NULL || scratch == NULL) {
		perror("calloc");
		goto done;
	}

	/* Untimed warmup pass so caches and boost clocks stabilize */
	for (m = 0; m < MODE_COUNT; m++) {
		if (!(modes & mode_flags[m])) {
			continue;
		}
		for (k = 0; k < nsel; k++) {
			uint64_t csum = 0;

			(void) dispatch_mode(mode_flags[m], bm_range_impls[sel[k]].fn,
				pats[0].pairs, pats[0].n, ws, VERIFY_OPS, &csum);
			bench_sink ^= csum;
		}
	}

	/*
	 * Interleave candidates in shuffled order on every trial round so slow
	 * clock or thermal drift distributes evenly rather than biasing early
	 * or late implementations.
	 */
	for (r = 0; r < trials; r++) {
		shuffle_indices(order, nsel, &rng);

		for (m = 0; m < MODE_COUNT; m++) {
			if (!(modes & mode_flags[m])) {
				continue;
			}

			for (p = 0; p < npats; p++) {
				uint64_t ref_csum = 0;
				int have_ref = 0;

				for (k = 0; k < nsel; k++) {
					size_t s = order[k];
					const struct bm_range_impl *im = &bm_range_impls[sel[s]];
					unsigned long cur_ops = ops;
					uint64_t csum = 0;
					double ns;

					if (im->fn == bm_range_loop_set && cur_ops >= 20000UL) {
						cur_ops /= 10UL;
					}

					ns = dispatch_mode(mode_flags[m], im->fn, pats[p].pairs,
						pats[p].n, ws, cur_ops, &csum);

					if (cur_ops == ops) {
						if (!have_ref) {
							ref_csum = csum;
							have_ref = 1;
						} else if (csum != ref_csum) {
							fprintf(stderr,
								"FAIL %s/%s: full checksum mismatch\n",
								pats[p].name, im->name);
							goto done;
						}
					}

					bench_sink ^= csum;
					pat_samples[((m * npats + p) * nsel + s) * trials + r] = ns;
					mean_samples[(m * nsel + s) * trials + r] += ns / (double) npats;
				}
			}
		}
	}

	printf("=== BM_RANGE MICROBENCHMARK (ns/op, %lu ops/trial, %zu shuffled trials) ===\n",
		ops, trials);
	printf("format: p50 [p25/p75/max] (sigma)\n");

	if (verbose) {
		for (p = 0; p < npats; p++) {
			const double *pseries[MODE_COUNT];

			for (m = 0; m < MODE_COUNT; m++) {
				pseries[m] = &pat_samples[(m * npats + p) * nsel * trials];
			}
			print_impl_table(pats[p].name, modes, sel, nsel, pseries,
				scratch, trials);
		}
	}

	{
		const double *mseries[MODE_COUNT];

		for (m = 0; m < MODE_COUNT; m++) {
			mseries[m] = &mean_samples[m * nsel * trials];
		}
		print_impl_table("MEAN ACROSS ALL 15 PATTERNS", modes, sel, nsel,
			mseries, scratch, trials);
	}

	rc = EXIT_SUCCESS;

done:
	free(scratch);
	free(mean_samples);
	free(pat_samples);
	for (p = 0; p < npats; p++) {
		free(pats[p].pairs);
	}
	free(ws);
	return rc;
}
