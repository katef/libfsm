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
 * Leaf nodes store up to 16 rows.
 */

/* XXX also need to store `count` within leaf nodes, for ordered iteration */

extern const uint8_t btree_shuffle_modify_lnode[16][2][16] __attribute__((aligned(64)));
extern const uint8_t btree_shuffle_divide_lnode[17][2][16] __attribute__((aligned(64)));

/*
 * 
 */

#if 1
/* XXX binary search */
static const uint8_t
btree_lnode_lub(BTREE_LNODE *node, const uint8_t count, const BTREE_KEY key)
{
        uint8_t                 n;
        uint8_t                 i;

        for (i = 0; i < count; i++) {
                if (btree_row_lt(node->lnode_rvect, node->lnode_rperm[i], key))
                        return i;
        }

        return count;
}
#endif

/*
 *
 */

static int
btree_lookup_lnode(BTREE_LNODE *node, const uint8_t count, const BTREE_KEY key, BTREE_RECORD *rec)
{
	uint8_t 		pos;
	uint8_t 		idx;

	pos = btree_lnode_lub(node, count, key);

	if (pos > 0 && btree_row_eq(node->lnode_rvect, idx = node->lnode_rperm[pos - 1], key)) {
		rec->status = BTREE_STATUS_EXISTING;
		rec->array = &node->lnode_rvect[0];
		rec->index = idx;

                return BTREE_RESULT_OK;
	}

	rec->status = BTREE_STATUS_NOTFOUND;
	rec->array = NULL;
	rec->index = UINT8_MAX;

	return BTREE_RESULT_OK;
}

/* XXX *countp -= (rec.status == BTREE_STATUS_EXISTING) */
static int
btree_delete_lnode(BTREE_LNODE *node, const uint8_t count, const BTREE_KEY key, BTREE_RECORD *rec)
{
	uint8_t 		pos;
	uint8_t 		idx;

	pos = btree_lnode_lub(node, count, key);

	if (pos == 0 || !btree_row_eq(node->lnode_rvect, idx = node->lnode_rperm[--pos], key)) {
		rec->status = BTREE_STATUS_NOTFOUND;
		rec->array = NULL;
		rec->index = UINT8_MAX;

		return BTREE_RESULT_OK;
	}

	const __m128i 		orig_rperm = _mm_load_si128(BTREE_M128CP(&node->lnode_rperm[0]));
	const __m128i 		shuf_rperm = _mm_load_si128(BTREE_M128CP(&btree_shuffle_modify_lnode[pos][1]));
	const __m128i 		node_rperm = _mm_shuffle_epi8(orig_rperm, shuf_rperm);

	_mm_store_si128(BTREE_M128P(&node->lnode_rperm[0]), node_rperm);
	node->lnode_size -= 1;

	rec->status = BTREE_STATUS_EXISTING;
	rec->array = &node->lnode_rvect[0];
	rec->index = idx;

	return BTREE_RESULT_OK;
}

static int
btree_update_lnode(BTREE_LNODE *node, const uint8_t count, const BTREE_KEY key, BTREE_RECORD *rec, uint8_t *ins_pos)
{
	int 			ret;
	uint8_t 		pos;
	uint8_t 		idx;

	pos = btree_lnode_lub(node, count, key);

	if (pos > 0 && btree_row_eq(node->lnode_rvect, idx = node->lnode_rperm[pos - 1], key)) {
		rec->status = BTREE_STATUS_EXISTING;
		rec->array = &node->lnode_rvect[0];
		rec->index = idx;

                return BTREE_RESULT_OK;
	}
	if (count == 16) {
		*ins_pos = pos;

		return BTREE_RESULT_DIVIDE_LNODE;
	}

	const __m128i 		orig_rperm = _mm_load_si128(BTREE_M128CP(&node->lnode_rperm[0]));
	const __m128i 		shuf_rperm = _mm_load_si128(BTREE_M128CP(&btree_shuffle_modify_lnode[pos][0]));
	const __m128i 		node_rperm = _mm_shuffle_epi8(orig_rperm, shuf_rperm);

	ret = btree_set_key(node->lnode_rvect, idx = node->lnode_rperm[15], key);
	if (ret != BTREE_RESULT_OK)
		return ret;

	_mm_store_si128(BTREE_M128P(&node->lnode_rperm[0]), node_rperm);
	node->lnode_size += 1;

	rec->status = BTREE_STATUS_INSERTED;
	rec->array = &node->lnode_rvect[0];
	rec->index = idx;

	return BTREE_RESULT_OK;
}

/* XXX fused split + insert, always produces 9:8 split, ie. count(node) = 9 and count(dest) = 8, remember that ins_pos <- [0, 16] */
static int
btree_divide_lnode(BTREE_LNODE * restrict node, BTREE_LNODE * restrict dest, const uint8_t ins_pos, const BTREE_KEY key,
    BTREE_RECORD * restrict rec)
{
	BTREE_LNODE * 		new;
	int 			ret;
	uint8_t 		idx;

	const __m128i 		orig_rperm = _mm_load_si128(BTREE_M128CP(&node->lnode_rperm[0]));
	const __m128i 		node_shufb = _mm_load_si128(BTREE_M128CP(&btree_shuffle_divide_lnode[ins_pos][0]));
	const __m128i 		dest_shufb = _mm_load_si128(BTREE_M128CP(&btree_shuffle_divide_lnode[ins_pos][1]));

	const __m128i 		node_rperm = _mm_shuffle_epi8(orig_rperm, node_shufb);
	const __m128i 		dest_rperm = _mm_shuffle_epi8(orig_rperm, dest_shufb);

	memcpy(dest, node, sizeof(BTREE_LNODE));

	new = (ins_pos > 8) ? dest : node;
	idx = (ins_pos > 8) ? btree_extract_epi8(dest_rperm, ins_pos - 9) : btree_extract_epi8(node_rperm, ins_pos);

	ret = btree_set_key(new->lnode_rvect, idx, key);
	if (__btree_predict_false(ret != BTREE_RESULT_OK))
		return ret;

	_mm_store_si128(BTREE_M128P(&node->lnode_rperm[0]), node_rperm);
	_mm_store_si128(BTREE_M128P(&dest->lnode_rperm[0]), dest_rperm);

	node->lnode_next = dest;
	node->lnode_size = 9;
	dest->lnode_size = 8;

	rec->status = BTREE_STATUS_INSERTED;
	rec->array = &new->lnode_rvect[0];
	rec->index = idx;

	return BTREE_RESULT_OK;
}
