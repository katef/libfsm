/*
 * Copyright 2026 Devon H. O'Dell
 *
 * See LICENCE for the full copyright terms.
 */

/*
 * Compiles src/adt/bitmap.c with its native bm_range() renamed to
 * bm_range_orig() and interposes bm_range() through an indirect function
 * pointer so libfsm and libre callers can be tested and timed against
 * every candidate implementation without relinking.
 */

#include <adt/bitmap.h>

#include "bitmap_shim.h"

bm_range_fn bm_range_orig;

#define bm_range bm_range_orig
#include "../../src/adt/bitmap.c"
#undef bm_range

static bm_range_fn *bm_range_active = bm_range_orig;

void
bm_range_shim_set(bm_range_fn *fn)
{
	/*
	 * bm_range_impls[] stores &bm_range for the live implementation,
	 * which resolves to the wrapper below; redirect it to bm_range_orig
	 * to avoid infinite recursion.
	 */
	bm_range_active = (fn == NULL || fn == bm_range) ? bm_range_orig : fn;
}

void
bm_range(struct bm *bm, size_t x, size_t y)
{
	bm_range_active(bm, x, y);
}
