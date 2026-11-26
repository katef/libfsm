/*
 * Copyright 2026 Devon H. O'Dell
 *
 * See LICENCE for the full copyright terms.
 */

/*
 * Candidate bm_range() implementations compiled in their own translation
 * unit so callers cross the same external call boundary as src/adt/bitmap.c
 * without constant propagation of [x, y] from test or benchmark harnesses.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <adt/bitmap.h>

#include "bitmap_impls.h"

void
bm_range_loop_set(struct bm *bm, size_t x, size_t y)
{
	size_t i;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);

	for (i = x; i <= y; i++) {
		bm_set(bm, i);
	}
}

/*
 * When NDEBUG elides the UCHAR_MAX assertion, size_t word indices leave
 * the trip count of the interior fill loop unbounded to the optimizer,
 * which can lower it to a memset() call or vector loop prologue despite
 * at most two interior words existing in a 256-bit bitmap.
 */
void
bm_range_word_naive(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy, w;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

	if (wx == wy) {
		bm->map[wx] |= lo & hi;
	} else {
		bm->map[wx] |= lo;
		for (w = wx + 1; w < wy; w++) {
			bm->map[w] = ~UINT64_C(0);
		}
		bm->map[wy] |= hi;
	}
}

/*
 * Narrowing endpoints to unsigned char bounds word indices to 0..3 even
 * under NDEBUG, allowing the optimizer to unroll the interior fill inline
 * instead of emitting a memset() call.
 */
void
bm_range_word_uchar(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy, w;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);

	wx = (unsigned char) x / 64U;
	wy = (unsigned char) y / 64U;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

	if (wx == wy) {
		bm->map[wx] |= lo & hi;
	} else {
		bm->map[wx] |= lo;
		for (w = wx + 1; w < wy; w++) {
			bm->map[w] = ~UINT64_C(0);
		}
		bm->map[wy] |= hi;
	}
}

/*
 * Touches a single word when wx == wy and bounds interior fills to the
 * two middle words of a 4-word bitmap. Variable-offset stores to map[wx]
 * can stall store-to-load forwarding when preceded by wide vector stores
 * in bm_clear() and followed by fixed 4-word reads.
 */
void
bm_range_word_bounded(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

	if (wx == wy) {
		bm->map[wx] |= lo & hi;
		return;
	}

	bm->map[wx] |= lo;
	if (wx + 1 < wy) {
		bm->map[wx + 1] = ~UINT64_C(0);
		if (wx + 2 < wy) {
			bm->map[2] = ~UINT64_C(0);
		}
	}
	bm->map[wy] |= hi;
}

/*
 * Dispatches on word span (0..3) so multi-word fills fall through at most
 * two interior word stores without a loop induction variable.
 */
void
bm_range_word_duff(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

	switch (wy - wx) {
	case 0:
		bm->map[wx] |= lo & hi;
		break;
	case 3:
		bm->map[wx + 2] = ~UINT64_C(0);
		/* fallthrough */
	case 2:
		bm->map[wx + 1] = ~UINT64_C(0);
		/* fallthrough */
	default:
		bm->map[wx] |= lo;
		bm->map[wy] |= hi;
		break;
	}
}

/*
 * Hoists the single-word case ahead of the span switch to keep intra-word
 * ranges off jump-table dispatch when most calls fall within one word.
 */
void
bm_range_word_fast_duff(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

	if (wx == wy) {
		bm->map[wx] |= lo & hi;
		return;
	}

	switch (wy - wx) {
	case 3:
		bm->map[wx + 2] = ~UINT64_C(0);
		/* fallthrough */
	case 2:
		bm->map[wx + 1] = ~UINT64_C(0);
		/* fallthrough */
	default:
		bm->map[wx] |= lo;
		bm->map[wy] |= hi;
		break;
	}
}

/*
 * Unrolled 4-word branchless mask. Updating all four words at fixed
 * offsets avoids branch mispredictions on mixed spans and avoids partial
 * store-to-load forwarding stalls after vector stores in bm_clear().
 */
void
bm_range_mask4_unroll(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

#define BM_MASK(c) (~UINT64_C(0) * (c))
	bm->map[0] |= (lo & BM_MASK(wx == 0U)) & (hi | BM_MASK(wy > 0U));
	bm->map[1] |= ((lo & BM_MASK(wx == 1U)) | BM_MASK(wx < 1U))
		& ((hi & BM_MASK(wy == 1U)) | BM_MASK(wy > 1U));
	bm->map[2] |= ((lo & BM_MASK(wx == 2U)) | BM_MASK(wx < 2U))
		& ((hi & BM_MASK(wy == 2U)) | BM_MASK(wy > 2U));
	bm->map[3] |= (lo | BM_MASK(wx < 3U)) & (hi & BM_MASK(wy == 3U));
#undef BM_MASK
}

/*
 * Loop form of the 4-word branchless mask; relies on the compiler to
 * unroll and cannot fold the map[3] upper-bound check under NDEBUG
 * when wy is an unbounded size_t.
 */
void
bm_range_mask4_loop(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy, w;
	uint64_t lo, hi;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));

#define BM_MASK(c) (~UINT64_C(0) * (c))
	for (w = 0; w < 4; w++) {
		uint64_t lw = (lo & BM_MASK(wx == w)) | BM_MASK(wx < w);
		uint64_t hw = (hi & BM_MASK(wy == w)) | BM_MASK(wy > w);

		bm->map[w] |= lw & hw;
	}
#undef BM_MASK
}

/*
 * Computes each word's mask directly from bit offsets clamped to [0, 64],
 * splitting each shift into two [0, 32] steps so a 64-bit full-word shift
 * is well-defined in C99 without word-index comparisons.
 */
void
bm_range_clamped_2shift(struct bm *bm, size_t x, size_t y)
{
	size_t w;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	for (w = 0; w < 4; w++) {
		size_t base = w * 64;
		size_t top  = base + 63;
		size_t l    = (x > base) ? (x - base) : 0;
		size_t r    = (top > y) ? (top - y) : 0;
		uint64_t lw, rw;

		if (l > 64) {
			l = 64;
		}
		if (r > 64) {
			r = 64;
		}

		lw = (~UINT64_C(0) << (l >> 1)) << (l - (l >> 1));
		rw = (~UINT64_C(0) >> (r >> 1)) >> (r - (r >> 1));
		bm->map[w] |= lw & rw;
	}
}

/*
 * Represents overlapping words as a 4-bit mask ((0xf << wx) & (0xf >> (3 - wy)))
 * and applies endpoint masks where w matches wx or wy.
 */
void
bm_range_active_4bit(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy, w;
	uint64_t lo, hi;
	unsigned int active;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = (unsigned char) x / 64U;
	wy = (unsigned char) y / 64U;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));
	active = (0xfU << wx) & (0xfU >> (3U - wy));

	for (w = 0; w < 4; w++) {
		uint64_t m = ~UINT64_C(0) * ((active >> w) & 1U);

		m &= (wx == w) ? lo : ~UINT64_C(0);
		m &= (wy == w) ? hi : ~UINT64_C(0);
		bm->map[w] |= m;
	}
}

/*
 * Combines indexed stores to the two endpoint words (coalescing wx == wy
 * via a branchless select) with fixed-offset masks on the two middle words.
 */
void
bm_range_hybrid_idx(struct bm *bm, size_t x, size_t y)
{
	size_t wx, wy;
	uint64_t lo, hi, same;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	wx = x / 64;
	wy = y / 64;
	lo = ~UINT64_C(0) << (x & 63);
	hi = ~UINT64_C(0) >> (63 - (y & 63));
	same = lo & hi;

	bm->map[wx] |= (wx == wy) ? same : lo;
	bm->map[wy] |= (wx == wy) ? same : hi;
	bm->map[1]  |= (wx < 1U && wy > 1U) ? ~UINT64_C(0) : 0;
	bm->map[2]  |= (wx < 2U && wy > 2U) ? ~UINT64_C(0) : 0;
}

/*
 * Compile-time constant suffix/prefix tables in read-only .rodata.
 * Because const tables cannot be aliased by struct bm *, compilers can load
 * and combine 128-bit/256-bit vectors without scalar serialization.
 *
 * lut_ro8k stores a single 257-entry suffix table (8,224 B, row 256 = 0)
 * and forms [x, y] as lo[x] & ~lo[y + 1]; lut_ro16k stores separate suffix
 * and prefix tables (16,384 B) to avoid the y + 1 offset and complement.
 */
#define BM_LO_WORD(b, w) \
	((b) <= (w) * 64 ? ~UINT64_C(0) : \
	((b) >= ((w) + 1) * 64 ? UINT64_C(0) : \
	(~UINT64_C(0) << (((b) - (w) * 64) & 63))))

#define BM_HI_WORD(b, w) \
	((b) < (w) * 64 ? UINT64_C(0) : \
	((b) >= (w) * 64 + 63 ? ~UINT64_C(0) : \
	(~UINT64_C(0) >> ((63 - ((b) - (w) * 64)) & 63))))

#define BM_LO_ENTRY(b) { \
	BM_LO_WORD(b, 0), BM_LO_WORD(b, 1), \
	BM_LO_WORD(b, 2), BM_LO_WORD(b, 3)  \
}

#define BM_HI_ENTRY(b) { \
	BM_HI_WORD(b, 0), BM_HI_WORD(b, 1), \
	BM_HI_WORD(b, 2), BM_HI_WORD(b, 3)  \
}

#define BM_LUT4(F, b)   F(b), F((b) + 1), F((b) + 2), F((b) + 3)
#define BM_LUT16(F, b)  BM_LUT4(F, b), BM_LUT4(F, (b) + 4), BM_LUT4(F, (b) + 8), BM_LUT4(F, (b) + 12)
#define BM_LUT64(F, b)  BM_LUT16(F, b), BM_LUT16(F, (b) + 16), BM_LUT16(F, (b) + 32), BM_LUT16(F, (b) + 48)
#define BM_LUT256(F)    BM_LUT64(F, 0), BM_LUT64(F, 64), BM_LUT64(F, 128), BM_LUT64(F, 192)

static const uint64_t bm_ro8k_lo[UCHAR_MAX + 2][4] = {
	BM_LUT256(BM_LO_ENTRY),
	{ 0, 0, 0, 0 }
};

static const uint64_t bm_ro16k_lo[UCHAR_MAX + 1][4] = {
	BM_LUT256(BM_LO_ENTRY)
};

static const uint64_t bm_ro16k_hi[UCHAR_MAX + 1][4] = {
	BM_LUT256(BM_HI_ENTRY)
};

#undef BM_LUT256
#undef BM_LUT64
#undef BM_LUT16
#undef BM_LUT4
#undef BM_HI_ENTRY
#undef BM_LO_ENTRY
#undef BM_HI_WORD
#undef BM_LO_WORD

void
bm_range_lut_ro8k(struct bm *bm, size_t x, size_t y)
{
	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	bm->map[0] |= bm_ro8k_lo[x][0] & ~bm_ro8k_lo[y + 1][0];
	bm->map[1] |= bm_ro8k_lo[x][1] & ~bm_ro8k_lo[y + 1][1];
	bm->map[2] |= bm_ro8k_lo[x][2] & ~bm_ro8k_lo[y + 1][2];
	bm->map[3] |= bm_ro8k_lo[x][3] & ~bm_ro8k_lo[y + 1][3];
}

void
bm_range_lut_ro16k(struct bm *bm, size_t x, size_t y)
{
	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	bm->map[0] |= bm_ro16k_lo[x][0] & bm_ro16k_hi[y][0];
	bm->map[1] |= bm_ro16k_lo[x][1] & bm_ro16k_hi[y][1];
	bm->map[2] |= bm_ro16k_lo[x][2] & bm_ro16k_hi[y][2];
	bm->map[3] |= bm_ro16k_lo[x][3] & bm_ro16k_hi[y][3];
}

/*
 * Runtime-initialized suffix (lo[x]) and prefix (hi[y]) lookup tables in .bss.
 * Passing table entries to external bm_clear() during initialization causes
 * conservative alias analysis to treat them as potentially aliased by bm.
 */
static struct bm bm_lo_lut[UCHAR_MAX + 1];
static struct bm bm_hi_lut[UCHAR_MAX + 1];
static int bm_luts_ready;

void
bm_range_impls_init(void)
{
	size_t i;

	if (bm_luts_ready) {
		return;
	}

	for (i = 0; i <= UCHAR_MAX; i++) {
		bm_clear(&bm_lo_lut[i]);
		bm_clear(&bm_hi_lut[i]);
		bm_range_loop_set(&bm_lo_lut[i], i, UCHAR_MAX);
		bm_range_loop_set(&bm_hi_lut[i], 0, i);
	}

	bm_luts_ready = 1;
}

void
bm_range_lut_16kb(struct bm *bm, size_t x, size_t y)
{
	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);
	assert(bm_luts_ready);

	bm->map[0] |= bm_lo_lut[x].map[0] & bm_hi_lut[y].map[0];
	bm->map[1] |= bm_lo_lut[x].map[1] & bm_hi_lut[y].map[1];
	bm->map[2] |= bm_lo_lut[x].map[2] & bm_hi_lut[y].map[2];
	bm->map[3] |= bm_lo_lut[x].map[3] & bm_hi_lut[y].map[3];
}

#ifdef __SIZEOF_INT128__
/*
 * Treats the 256-bit bitmap as two 128-bit halves using GCC/Clang's
 * unsigned __int128 extension; uses memcpy() to respect uint64_t
 * effective-type aliasing and 8-byte struct bm alignment.
 */
void
bm_range_u128_pair(struct bm *bm, size_t x, size_t y)
{
	__extension__ typedef unsigned __int128 bm_u128_t;
	bm_u128_t all, lo, hi, m0, m1, c0, c1;
	size_t hx, hy;

	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	all = ((bm_u128_t) ~UINT64_C(0) << 64) | ~UINT64_C(0);
	hx  = x / 128;
	hy  = y / 128;
	lo  = all << (x & 127);
	hi  = all >> (127 - (y & 127));
	m0  = (hx == 0 ? lo : 0) & (hy > 0 ? all : hi);
	m1  = (hx < 1 ? all : lo) & (hy == 1 ? hi : 0);

	memcpy(&c0, &bm->map[0], sizeof c0);
	memcpy(&c1, &bm->map[2], sizeof c1);
	c0 |= m0;
	c1 |= m1;
	memcpy(&bm->map[0], &c0, sizeof c0);
	memcpy(&bm->map[2], &c1, sizeof c1);
}
#endif

const struct bm_range_impl bm_range_impls[] = {
	{ "bm_range",     "live src/adt/bitmap.c implementation",               bm_range               },
	{ "loop_set",     "bit-by-bit bm_set() loop",                           bm_range_loop_set      },
	{ "word_naive",   "word-wise masks with unbounded interior loop",       bm_range_word_naive    },
	{ "word_uchar",   "word-wise masks with byte-bounded interior loop",    bm_range_word_uchar    },
	{ "word_bounded", "single-word fast path with bounded interior words",  bm_range_word_bounded  },
	{ "word_duff",    "Duff-style switch on word span",                     bm_range_word_duff     },
	{ "word_fduff",   "single-word fast path with Duff switch on span",     bm_range_word_fast_duff},
	{ "mask4_unr",    "unrolled 4-word branchless mask",                    bm_range_mask4_unroll  },
	{ "mask4_loop",   "looped 4-word branchless mask",                      bm_range_mask4_loop    },
	{ "clamp_2shf",   "per-word clamped two-step shifts",                   bm_range_clamped_2shift},
	{ "active_4b",    "4-bit active-word mask with endpoint masks",         bm_range_active_4bit   },
	{ "hybrid_idx",   "indexed endpoint words with branchless interiors",   bm_range_hybrid_idx    },
	{ "lut_ro8k",     "8 KiB static const .rodata suffix lookup table",     bm_range_lut_ro8k      },
	{ "lut_ro16k",    "16 KiB static const .rodata lo/hi lookup tables",    bm_range_lut_ro16k     },
	{ "lut_16kb",     "16 KiB runtime-initialized .bss lookup tables",      bm_range_lut_16kb      },
#ifdef __SIZEOF_INT128__
	{ "u128_pair",    "128-bit pair mask via __int128",                     bm_range_u128_pair     },
#endif
};

const size_t bm_range_impl_count = sizeof bm_range_impls / sizeof *bm_range_impls;
