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

/* See `btree_shuffle.c`. */
extern const uint8_t btree_shuffle_modify_bnode[16][4][16] __attribute__((aligned(64)));
extern const uint8_t btree_shuffle_divide_bnode[17][4][16] __attribute__((aligned(64)));
extern const uint8_t btree_shuffle_extend_perm[16][16] __attribute__((aligned(64)));

/*
 * Branch node search.
 */

#if BTREE_OPT_BNODE_SEARCH == 1 
static const uint8_t
btree_bnode_glb(BTREE_BNODE *node, const uint8_t size, const BTREE_KEY key)
{
	__m128i 		kperm = _mm_load_si128(BTREE_M128CP(node->bnode_kperm));
	uint8_t			i;

	for (i = 0; i < size; i++) {
		if (btree_key_lt(node->bnode_kvect, _mm_extract_epi8(kperm, 1), key))
			return i;

		kperm = _mm_bsrli_si128(kperm, 1);
	}

	return size;
}
#endif

#if BTREE_OPT_BNODE_SEARCH == 4
const uint8_t
btree_bnode_glb(BTREE_BNODE *node, const uint8_t size, const BTREE_KEY key)
{
	const __m128i 		shuf_kperm = _mm_load_si128(BTREE_M128CP(btree_shuffle_extend_perm[size]));
	const __m128i 		orig_kperm = _mm_load_si128(BTREE_M128CP(node->bnode_kperm));
	const __m128i 		node_kperm = _mm_shuffle_epi8(orig_kperm, shuf_kperm);

	const uint8_t 		pos1 = btree_key_lt(node->bnode_kvect, _mm_extract_epi8(node_kperm,  4), key) ?    2 :  6;
	const uint8_t 		pos2 = btree_key_lt(node->bnode_kvect, _mm_extract_epi8(node_kperm,  8), key) ? pos1 : 10;
	const uint8_t 		pos3 = btree_key_lt(node->bnode_kvect, _mm_extract_epi8(node_kperm, 12), key) ? pos2 : 14;

	const uint8_t 		pos4 = btree_key_lt(node->bnode_kvect, btree_extract_epi8(node_kperm, pos3), key) ? pos3 - 1 : pos3 + 1;
	const uint8_t 		pos5 = btree_key_lt(node->bnode_kvect, btree_extract_epi8(node_kperm, pos4), key) ? pos4 - 1 : pos4;

	return (pos5 > size) ? size : pos5;
}
#endif

/*
 * Branch node modifications.
 */

static void
btree_insert_bnode(BTREE_BNODE *node, const uint8_t pos, const BTREE_KEY key, BTREE_NODEP root_node, const uint8_t root_size)
{
	const uint8_t 		ins_pos = pos + 1;
	uint8_t 		tidx;
	uint8_t 		kidx;

	const __m128i 		orig_tperm = _mm_load_si128(BTREE_M128CP(&node->bnode_tperm[0]));
	const __m128i 		orig_kperm = _mm_load_si128(BTREE_M128CP(&node->bnode_kperm[0]));
	const __m128i 		orig_svect = _mm_load_si128(BTREE_M128CP(&node->bnode_svect[0]));

	const __m128i 		shuf_tperm = _mm_load_si128(BTREE_M128CP(&btree_shuffle_modify_bnode[ins_pos][0]));
	const __m128i 		shuf_kperm = _mm_load_si128(BTREE_M128CP(&btree_shuffle_modify_bnode[ins_pos][1]));

	const __m128i 		node_tperm = _mm_shuffle_epi8(orig_tperm, shuf_tperm);
	const __m128i 		node_kperm = _mm_shuffle_epi8(orig_kperm, shuf_kperm);
	const __m128i 		node_svect = _mm_shuffle_epi8(orig_svect, shuf_tperm);

	_mm_store_si128(BTREE_M128P(&node->bnode_tperm[0]), node_tperm);
	_mm_store_si128(BTREE_M128P(&node->bnode_kperm[0]), node_kperm);
	_mm_store_si128(BTREE_M128P(&node->bnode_svect[0]), node_svect);

	tidx = btree_extract_epi8(node_tperm, ins_pos);
	kidx = btree_extract_epi8(node_kperm, ins_pos);

	node->bnode_tvect[tidx] = root_node;
	node->bnode_kvect[kidx] = key;
	node->bnode_svect[ins_pos] = root_size;
}

static void
btree_delete_bnode(BTREE_BNODE *node, const uint8_t pos)
{
	const __m128i 		orig_tperm = _mm_load_si128(BTREE_M128CP(&node->bnode_tperm[0]));
	const __m128i 		orig_kperm = _mm_load_si128(BTREE_M128CP(&node->bnode_kperm[0]));
	const __m128i 		orig_svect = _mm_load_si128(BTREE_M128CP(&node->bnode_svect[0]));

	const __m128i 		shuf_tperm = _mm_load_si128(BTREE_M128CP(&btree_shuffle_modify_bnode[pos][2]));
	const __m128i 		shuf_kperm = _mm_load_si128(BTREE_M128CP(&btree_shuffle_modify_bnode[pos][3]));

	const __m128i 		node_tperm = _mm_shuffle_epi8(orig_tperm, shuf_tperm);
	const __m128i 		node_kperm = _mm_shuffle_epi8(orig_kperm, shuf_kperm);
	const __m128i 		node_svect = _mm_shuffle_epi8(orig_svect, shuf_tperm);

	_mm_store_si128(BTREE_M128P(&node->bnode_tperm[0]), node_tperm);
	_mm_store_si128(BTREE_M128P(&node->bnode_kperm[0]), node_kperm);
	_mm_store_si128(BTREE_M128P(&node->bnode_svect[0]), node_svect);
}

/* fused split + insert, always produces a N = (K: 8, T: 9) and  D = (K: 7, T: 8) split, remember that ins_pos <- [1, 16] */
/* assume count(node, ins_pos) has been adjusted, only do count(some, new_pos) = root_size here */
static BTREE_KEY
btree_divide_bnode(BTREE_BNODE * restrict node, BTREE_BNODE * restrict dest, const uint8_t pos, BTREE_KEY key, BTREE_NODEP root_node,
    const uint8_t root_size)
{
	BTREE_KEY 		pivot;
	BTREE_KEY 		scratch;
	BTREE_BNODE * 		temp;
	BTREE_KEY * 		kptr;
	uint8_t 		tidx;
	uint8_t 		kidx;
	uint8_t 		xpos;

	const uint8_t 		ins_pos = pos + 1;

	const __m128i 		orig_tperm = _mm_load_si128(BTREE_M128CP(&node->bnode_tperm[0]));
	const __m128i 		orig_kperm = _mm_load_si128(BTREE_M128CP(&node->bnode_kperm[0]));
	const __m128i 		orig_svect = _mm_load_si128(BTREE_M128CP(&node->bnode_svect[0]));

	const __m128i 		shuf_tperm_l = _mm_load_si128(BTREE_M128CP(&btree_shuffle_divide_bnode[ins_pos][0]));
	const __m128i 		shuf_kperm_l = _mm_load_si128(BTREE_M128CP(&btree_shuffle_divide_bnode[ins_pos][1]));
	const __m128i 		shuf_tperm_r = _mm_load_si128(BTREE_M128CP(&btree_shuffle_divide_bnode[ins_pos][2]));
	const __m128i 		shuf_kperm_r = _mm_load_si128(BTREE_M128CP(&btree_shuffle_divide_bnode[ins_pos][3]));

	const __m128i 		node_tperm = _mm_shuffle_epi8(orig_tperm, shuf_tperm_l);
	const __m128i 		node_kperm = _mm_shuffle_epi8(orig_kperm, shuf_kperm_l);
	const __m128i 		node_svect = _mm_shuffle_epi8(orig_svect, shuf_tperm_l);
	const __m128i 		dest_tperm = _mm_shuffle_epi8(orig_tperm, shuf_tperm_r);
	const __m128i 		dest_kperm = _mm_shuffle_epi8(orig_kperm, shuf_kperm_r);
	const __m128i 		dest_svect = _mm_shuffle_epi8(orig_svect, shuf_tperm_r);

	memcpy(dest, node, sizeof(BTREE_BNODE));

	pivot = (ins_pos > 8) ? node->bnode_kvect[_mm_extract_epi8(orig_kperm, 9)] : node->bnode_kvect[_mm_extract_epi8(orig_kperm, 8)];
	pivot = (ins_pos == 9) ? key : pivot;

	_mm_store_si128(BTREE_M128P(&node->bnode_tperm[0]), node_tperm);
	_mm_store_si128(BTREE_M128P(&node->bnode_kperm[0]), node_kperm);
	_mm_store_si128(BTREE_M128P(&node->bnode_svect[0]), node_svect);

	_mm_store_si128(BTREE_M128P(&dest->bnode_tperm[0]), dest_tperm);
	_mm_store_si128(BTREE_M128P(&dest->bnode_kperm[0]), dest_kperm);
	_mm_store_si128(BTREE_M128P(&dest->bnode_svect[0]), dest_svect);

	xpos = (ins_pos > 8) ? ins_pos - 9 : ins_pos;
	temp = (ins_pos > 8) ? dest : node;
	tidx = (ins_pos > 8) ? btree_extract_epi8(dest_tperm, xpos) : btree_extract_epi8(node_tperm, xpos);
	kidx = (ins_pos > 8) ? btree_extract_epi8(dest_kperm, xpos) : btree_extract_epi8(node_kperm, xpos);
	kptr = (ins_pos == 9) ? &scratch : &temp->bnode_kvect[kidx];

	temp->bnode_tvect[tidx] = root_node;
	temp->bnode_svect[xpos] = root_size;
	*kptr = key;

	return pivot;
}
