/*
 * Copyright © 2019 Jáchym Holeček <freza@circlewave.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *
 */

#ifndef BTREE_TMPL_H
#error "Unbalanced inclusion of <btree/close.h>."
#endif

/*
 * Local utilities.
 */

static inline const uint8_t
btree_extract_epi8(const __m128i vec, const uint8_t nth)
{
	switch (nth) {
	case 15:
		return (uint8_t)_mm_extract_epi8(vec, 15);
	case 14:
		return (uint8_t)_mm_extract_epi8(vec, 14);
	case 13:
		return (uint8_t)_mm_extract_epi8(vec, 13);
	case 12:
		return (uint8_t)_mm_extract_epi8(vec, 12);
	case 11:
		return (uint8_t)_mm_extract_epi8(vec, 11);
	case 10:
		return (uint8_t)_mm_extract_epi8(vec, 10);
	case  9:
		return (uint8_t)_mm_extract_epi8(vec,  9);
	case  8:
		return (uint8_t)_mm_extract_epi8(vec,  8);
	case  7:
		return (uint8_t)_mm_extract_epi8(vec,  7);
	case  6:
		return (uint8_t)_mm_extract_epi8(vec,  6);
	case  5:
		return (uint8_t)_mm_extract_epi8(vec,  5);
	case  4:
		return (uint8_t)_mm_extract_epi8(vec,  4);
	case  3:
		return (uint8_t)_mm_extract_epi8(vec,  3);
	case  2:
		return (uint8_t)_mm_extract_epi8(vec,  2);
	case  1:
		return (uint8_t)_mm_extract_epi8(vec,  1);
	case  0:
		return (uint8_t)_mm_extract_epi8(vec,  0);
	}
}

/*
 * Import implementations.
 */

#include "btree/alloc.c"
#include "btree/bnode.c"
#include "btree/lnode.c"
#include "btree/tree.c"

#if BTREE_OPT_DEBUG_LOOKUP == 1
#include "btree/debug.c"
#endif

/*
 * Namespace cleanup.
 */

#undef btree_extract_epi8

#undef BTREE_BNODEP
#undef BTREE_LNODEP

#undef BTREE_M128CP
#undef BTREE_M128P

#undef btree_request_lnode
#undef btree_request_bnode
#undef btree_release_lnode
#undef btree_release_bnode
#undef btree_request_bstack
#undef btree_release_bstack

#undef btree_bnode_glb
#undef btree_insert_bnode
#undef btree_delete_bnode
#undef btree_divide_bnode

#undef btree_lnode_lub
#undef btree_lookup_lnode
#undef btree_delete_lnode
#undef btree_update_lnode
#undef btree_divide_lnode

#undef btree_init
#undef btree_free
#undef btree_lookup
#undef btree_update
#undef btree_delete
#undef btree_drop_lnode
#undef btree_drop_bnode
#undef btree_initroot_bnode
#undef btree_prefetch_lnode
#undef btree_prefetch_bnode

#undef btree_print_path
#undef btree_print_bnode
#undef btree_print_lnode

#undef BTREE_RNODE
#undef BTREE_RECORD

#undef BTREE_NODEP
#undef BTREE_BNODE
#undef BTREE_LNODE
#undef BTREE_USLOT

#undef btree_row_lt
#undef btree_row_eq
#undef btree_key_lt
#undef btree_set_key
#undef btree_get_key
#undef btree_release_row
#undef btree_format_key
#undef btree_format_row

#undef __btree_predict_true
#undef __btree_predict_false

#undef __btree_cache_aligned
#undef __btree_simd_aligned
#undef __btree_packed
#undef __btree_unused

#undef BTREE_OPT_DEBUG_LOOKUP
#undef BTREE_OPT_CUSTOM_ALLOC

#undef BTREE_NAME
#undef BTREE_ROW
#undef BTREE_KEY

#undef BTREE_TMPL_H
