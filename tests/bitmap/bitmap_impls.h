/*
 * Copyright 2026 Devon H. O'Dell
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef BITMAP_IMPLS_H
#define BITMAP_IMPLS_H

#include <stddef.h>

struct bm;

typedef void (bm_range_fn)(struct bm *bm, size_t x, size_t y);

struct bm_range_impl {
	const char *name;
	const char *desc;
	bm_range_fn *fn;
};

extern const struct bm_range_impl bm_range_impls[];
extern const size_t bm_range_impl_count;

void bm_range_impls_init(void);

bm_range_fn bm_range_loop_set;
bm_range_fn bm_range_word_naive;
bm_range_fn bm_range_word_uchar;
bm_range_fn bm_range_word_bounded;
bm_range_fn bm_range_word_duff;
bm_range_fn bm_range_word_fast_duff;
bm_range_fn bm_range_mask4_unroll;
bm_range_fn bm_range_mask4_loop;
bm_range_fn bm_range_clamped_2shift;
bm_range_fn bm_range_active_4bit;
bm_range_fn bm_range_hybrid_idx;
bm_range_fn bm_range_lut_ro8k;
bm_range_fn bm_range_lut_ro16k;
bm_range_fn bm_range_lut_16kb;
#ifdef __SIZEOF_INT128__
bm_range_fn bm_range_u128_pair;
#endif

#endif
