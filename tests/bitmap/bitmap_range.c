/*
 * Copyright 2026 Devon H. O'Dell
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/bitmap.h>
#include <print/esc.h>

#include "bitmap_impls.h"

static int
test_escputc(FILE *f, const struct fsm_options *opt, char c)
{
	unsigned char uc = (unsigned char) c;

	(void) opt;

	if (uc >= 0x20 && uc <= 0x7e && uc != '\\' && uc != ']' && uc != '-') {
		return putc(uc, f) == EOF ? -1 : 1;
	}

	return fprintf(f, "\\x%02x", (unsigned int) uc);
}

/*
 * Exhaustively test all [x, y] pairs in 0..UCHAR_MAX against bit-by-bit
 * bm_set() across empty, full, alternating, and word-boundary-sentinel
 * initial bitmaps so out-of-range bit clobbering and off-by-one shifts
 * at 64-bit seams are caught.
 */
static void
verify_exhaustive_single_range(bm_range_fn *fn)
{
	static const uint64_t seeds[][4] = {
		{ 0, 0, 0, 0 },
		{
			~UINT64_C(0), ~UINT64_C(0),
			~UINT64_C(0), ~UINT64_C(0)
		},
		{
			UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210),
			UINT64_C(0x55555555aaaaaaaa), UINT64_C(0xaaaa5555aaaa5555)
		},
		{
			UINT64_C(0x8000000000000001), UINT64_C(0x8000000000000001),
			UINT64_C(0x8000000000000001), UINT64_C(0x8000000000000001)
		}
	};
	size_t s, x, y;

	for (s = 0; s < sizeof seeds / sizeof *seeds; s++) {
		struct bm seed_bm;

		memcpy(seed_bm.map, seeds[s], sizeof seed_bm.map);

		for (x = 0; x <= UCHAR_MAX; x++) {
			for (y = x; y <= UCHAR_MAX; y++) {
				struct bm exp, got;

				exp = seed_bm;
				got = seed_bm;

				bm_range_loop_set(&exp, x, y);
				fn(&got, x, y);

				assert(memcmp(&got, &exp, sizeof got) == 0);
				assert(bm_count(&got) == bm_count(&exp));
				assert(bm_any(&got) == 1);
				assert(bm_get(&got, x) == 1);
				assert(bm_get(&got, y) == 1);

				if (x > 0) {
					assert(bm_get(&got, x - 1) == bm_get(&seed_bm, x - 1));
				}
				if (y < UCHAR_MAX) {
					assert(bm_get(&got, y + 1) == bm_get(&seed_bm, y + 1));
				}

				if (s == 0) {
					assert(bm_count(&got) == y - x + 1);
					assert(bm_next(&got, -1, 1) == x);
					assert(bm_next(&got, (int) x - 1, 1) == x);
					assert(bm_next(&got, (int) x, 0) == y + 1);
					assert(bm_next(&got, (int) y, 1) == UCHAR_MAX + 1);
				}
			}
		}
	}
}

/*
 * Range setting forms a join-semilattice: setting [x, y] is idempotent,
 * and partitioning [x, y] into [x, mid] U [mid + 1, y] across word seams
 * must produce the same bitmap as setting [x, y] at once.
 */
static void
verify_split_and_idempotence(bm_range_fn *fn)
{
	static const size_t pts[] = {
		0, 1, 2, 31, 32, 62, 63, 64, 65, 95, 96,
		126, 127, 128, 129, 159, 160, 190, 191, 192, 193,
		223, 224, 253, 254, 255
	};
	const size_t npts = sizeof pts / sizeof *pts;
	size_t i, j, k;

	for (i = 0; i < npts; i++) {
		for (k = i; k < npts; k++) {
			size_t x = pts[i];
			size_t y = pts[k];
			struct bm once, twice;

			bm_clear(&once);
			fn(&once, x, y);

			twice = once;
			fn(&twice, x, y);
			assert(memcmp(&once, &twice, sizeof once) == 0);

			for (j = i; j < k; j++) {
				size_t mid = pts[j];
				struct bm split;

				if (mid >= y) {
					continue;
				}

				bm_clear(&split);
				fn(&split, x, mid);
				fn(&split, mid + 1, y);
				assert(memcmp(&once, &split, sizeof once) == 0);
			}
		}
	}
}

static void
verify_accumulation(bm_range_fn *fn)
{
	static const struct {
		size_t x, y;
	} ranges[] = {
		{ '0', '9' },
		{ 'A', 'Z' },
		{ '_', '_' },
		{ 'a', 'z' },
		{ 'a', 'z' },
		{ 0,   63  },
		{ 62,  65  },
		{ 127, 128 },
		{ 191, 193 },
		{ 120, 200 },
		{ 0,   255 }
	};
	struct bm got, exp;
	size_t i;

	bm_clear(&got);
	bm_clear(&exp);

	for (i = 0; i < sizeof ranges / sizeof *ranges; i++) {
		fn(&got, ranges[i].x, ranges[i].y);
		bm_range_loop_set(&exp, ranges[i].x, ranges[i].y);
		assert(memcmp(&got, &exp, sizeof got) == 0);
		assert(bm_count(&got) == bm_count(&exp));
	}

	assert(bm_count(&got) == UCHAR_MAX + 1);
}

static void
verify_algebra_and_print(bm_range_fn *fn)
{
	struct bm got_id, exp_id;
	struct bm got_hex, exp_hex;
	struct bm got_neg, exp_neg;
	struct bm got_tmp, exp_tmp;
	char got_buf[256], exp_buf[256];
	int got_n, exp_n;

	bm_clear(&got_id);
	bm_clear(&exp_id);
	fn(&got_id, '0', '9');
	fn(&got_id, 'A', 'Z');
	fn(&got_id, '_', '_');
	fn(&got_id, 'a', 'z');
	bm_unset(&got_id, 'O');
	bm_unset(&got_id, '0');

	bm_range_loop_set(&exp_id, '0', '9');
	bm_range_loop_set(&exp_id, 'A', 'Z');
	bm_range_loop_set(&exp_id, '_', '_');
	bm_range_loop_set(&exp_id, 'a', 'z');
	bm_unset(&exp_id, 'O');
	bm_unset(&exp_id, '0');
	assert(memcmp(&got_id, &exp_id, sizeof got_id) == 0);

	bm_clear(&got_hex);
	bm_clear(&exp_hex);
	fn(&got_hex, '0', '9');
	fn(&got_hex, 'A', 'F');
	fn(&got_hex, 'a', 'f');
	bm_range_loop_set(&exp_hex, '0', '9');
	bm_range_loop_set(&exp_hex, 'A', 'F');
	bm_range_loop_set(&exp_hex, 'a', 'f');
	assert(memcmp(&got_hex, &exp_hex, sizeof got_hex) == 0);

	bm_copy(&got_tmp, &got_id);
	bm_copy(&exp_tmp, &exp_id);
	bm_intersect(&got_tmp, &got_hex);
	bm_intersect(&exp_tmp, &exp_hex);
	assert(memcmp(&got_tmp, &exp_tmp, sizeof got_tmp) == 0);
	assert(bm_count(&got_tmp) == 21); /* 22 hex digits minus '0' */

	bm_clear(&got_neg);
	bm_clear(&exp_neg);
	fn(&got_neg, 0x00, 0x08);
	fn(&got_neg, '\n', '\n');
	fn(&got_neg, '\r', '\r');
	fn(&got_neg, 0x80, 0xff);
	bm_invert(&got_neg);

	bm_range_loop_set(&exp_neg, 0x00, 0x08);
	bm_range_loop_set(&exp_neg, '\n', '\n');
	bm_range_loop_set(&exp_neg, '\r', '\r');
	bm_range_loop_set(&exp_neg, 0x80, 0xff);
	bm_invert(&exp_neg);
	assert(memcmp(&got_neg, &exp_neg, sizeof got_neg) == 0);

	bm_union(&got_tmp, &got_neg);
	bm_union(&exp_tmp, &exp_neg);
	assert(memcmp(&got_tmp, &exp_tmp, sizeof got_tmp) == 0);

	got_n = bm_snprint(&got_id, NULL, got_buf, sizeof got_buf, 1, test_escputc);
	exp_n = bm_snprint(&exp_id, NULL, exp_buf, sizeof exp_buf, 1, test_escputc);
	assert(got_n > 0 && got_n == exp_n);
	assert(strcmp(got_buf, exp_buf) == 0);
}

int
main(void)
{
	size_t i;

	bm_range_impls_init();

	for (i = 0; i < bm_range_impl_count; i++) {
		bm_range_fn *fn = bm_range_impls[i].fn;

		verify_exhaustive_single_range(fn);
		verify_split_and_idempotence(fn);
		verify_accumulation(fn);
		verify_algebra_and_print(fn);
	}

	return EXIT_SUCCESS;
}
