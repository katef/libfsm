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
 * Utilities: Prefetch branch or leaf node given a branch node and subtree position.
 */

static inline BTREE_NODEP
btree_prefetch_lnode(BTREE_BNODE *node, const uint8_t pos)
{
	BTREE_NODEP 		snp = node->bnode_tvect[node->bnode_tperm[pos]];
#if BTREE_OPT_ENABLE_PREFETCH == 1
	int 			i;

	for (i = 0; i < sizeof(BTREE_LNODE); i += 64)
		//_mm_prefetch((char const *)((uintptr_t)snp.lnode + i), _MM_HINT_T0);
		//_mm_prefetch((char const *)((uintptr_t)snp.lnode + i), _MM_HINT_T1);
		//_mm_prefetch((char const *)((uintptr_t)snp.lnode + i), _MM_HINT_T2);
		_mm_prefetch((char const *)((uintptr_t)snp.lnode + i), _MM_HINT_NTA);
#endif

	return snp;
}

static inline BTREE_NODEP
btree_prefetch_bnode(BTREE_BNODE *node, const uint8_t pos)
{
	BTREE_NODEP 		snp = node->bnode_tvect[node->bnode_tperm[pos]];
#if BTREE_OPT_ENABLE_PREFETCH == 1
	int 			i;

	//_mm_prefetch((char const *)(uintptr_t)snp.bnode, _MM_HINT_T0);

	for (i = 64; i < sizeof(BTREE_BNODE); i += 64)
		//_mm_prefetch((char const *)((uintptr_t)snp.bnode + i), _MM_HINT_T0);
		//_mm_prefetch((char const *)((uintptr_t)snp.bnode + i), _MM_HINT_T1);
		//_mm_prefetch((char const *)((uintptr_t)snp.bnode + i), _MM_HINT_T2);
		_mm_prefetch((char const *)((uintptr_t)snp.bnode + i), _MM_HINT_NTA);
#endif

	return snp;
}

/*
 *
 */

static BTREE_BNODE *
btree_initroot_bnode(BTREE_BNODE *node)
{
	static const uint8_t 	root_tperm[16] = {  0,  1, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2 };
	static const uint8_t 	root_kperm[16] = {  0,  0, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1 };

	memcpy(node->bnode_tperm, root_tperm, sizeof(root_tperm));
	memcpy(node->bnode_kperm, root_kperm, sizeof(root_kperm));
	memset(node->bnode_svect, 0, sizeof(root_tperm));

	return node;
}

static void
btree_drop_bnode(BTREE_RNODE *tree, BTREE_BNODE *node, const uint8_t level, const uint8_t size)
{
	uint8_t 		idx;
	uint8_t 		i;

	if (level > 1) {
		for (i = 0; i <= size; i++) {
			idx = node->bnode_tperm[i];

			btree_drop_bnode(tree, node->bnode_tvect[idx].bnode, level - 1, node->bnode_svect[i]);
		}
	}

	btree_release_bnode(tree, node);
}

static BTREE_LNODE *
btree_drop_lnode(BTREE_RNODE *tree, BTREE_LNODE *node)
{
	BTREE_LNODE * 		next = node->lnode_next;
	const uint8_t 		size = node->lnode_size;
	uint8_t 		i;

	for (i = 0; i < size; i++)
		btree_release_row(node->lnode_rvect, node->lnode_rperm[i]);

	btree_release_lnode(tree, node);
	return next;
}

/*
 * Public interface:
 */

BTREE_RNODE *
btree_init(BTREE_RNODE *tree)
{
	static const uint8_t 	identity[16] = { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0 };
	BTREE_LNODE * 		node;

	node = btree_request_lnode(tree);
	if (node == NULL)
		return NULL;

	memcpy(node->lnode_rperm, identity, sizeof(identity));
	node->lnode_next = NULL;
	node->lnode_size = 0;

	tree->rnode_root.lnode = node;
	tree->rnode_count = 0;
	tree->rnode_level = 0;
	tree->rnode_size = 0;
	tree->rnode_minimum = node;
	tree->rnode_maximum = node;

	return tree;
}

void
btree_free(BTREE_RNODE *tree)
{
	BTREE_LNODE * 		node;

	/* Drop branch levels. */
	if (tree->rnode_level > 0)
		btree_drop_bnode(tree, tree->rnode_root.bnode, tree->rnode_level, tree->rnode_size);

	tree->rnode_root.bnode = NULL;
	tree->rnode_level = UINT8_MAX;
	tree->rnode_size = UINT8_MAX;

	/* Drop leaf level. */
	for (node = tree->rnode_minimum; node != NULL; node = btree_drop_lnode(tree, node))
		;

	tree->rnode_minimum = NULL;
	tree->rnode_maximum = NULL;
}

/*
 * Public interface: Point lookups and updates.
 */

int
btree_lookup(BTREE_RNODE *tree, const BTREE_KEY key, BTREE_RECORD *rec)
{
	BTREE_NODEP 		tnp;
	BTREE_NODEP 		xnp;
	uint8_t 		pos;
	uint8_t 		cnt;
	uint8_t 		xnt;

	tnp = tree->rnode_root;
	cnt = tree->rnode_size;

	switch (tree->rnode_level) {
	case  9:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  8:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  7:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  6:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  5:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  4:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  3:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  2:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_bnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  1:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = btree_prefetch_lnode(tnp.bnode, pos);
		xnt = tnp.bnode->bnode_svect[pos];
		tnp = xnp;
		cnt = xnt;
	case  0:
		return btree_lookup_lnode(tnp.lnode, cnt, key, rec);
	}

	abort();
}

int
btree_update(BTREE_RNODE * const restrict tree, const BTREE_KEY key, BTREE_RECORD * restrict rec)
{
	BTREE_USLOT 		dsp[BTREE_STACK_DEPTH];
	BTREE_KEY 		pivot;
	BTREE_BNODE * 		root;
	BTREE_LNODE * 		node;
	BTREE_LNODE * 		next;
	uint8_t * 		new_szp;
	uint8_t * 		szp;
	uint8_t * 		top;
	const uint64_t 		level = tree->rnode_level;
	BTREE_NODEP 		new_tnp;
	BTREE_NODEP 		tnp;
	int 			ret;
	uint8_t 		pos;
	uint8_t 		new_cnt;
	uint8_t 		cnt;
	uint8_t 		idx;
	uint8_t 		nth;
	uint8_t 		lsz;
	uint8_t 		rsz;
	bool 			root_split;

	/*
	 * Traverse branches down to a leaf that `key` belongs to.
	 */

	tnp = tree->rnode_root;
	cnt = tree->rnode_size;

	top = &tree->rnode_size;
	szp = &tree->rnode_size;
	idx = 0;

	switch (level) {
	case  9:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  8:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  7:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  6:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  5:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  4:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  3:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  2:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_bnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 15) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;

	case  1:
		idx = (cnt == 15) ? idx + 1 : 0;
		pos = btree_bnode_glb(tnp.bnode, cnt, key);

		new_tnp = btree_prefetch_lnode(tnp.bnode, pos);
		new_cnt = tnp.bnode->bnode_svect[pos];
		new_szp = &tnp.bnode->bnode_svect[pos];

		top = (cnt < 15 && new_cnt == 16) ? szp : top;

		dsp[idx].node = tnp;
		dsp[idx].pos = pos;

		tnp = new_tnp;
		cnt = new_cnt;
		szp = new_szp;
	}
	node = tnp.lnode;

	/*
	 * Attempt update or insert within leaf.
	 */

	ret = btree_update_lnode(node, cnt, key, rec, &nth);
	if (ret != BTREE_RESULT_DIVIDE_LNODE) {
		const uint8_t 	inc = (uint8_t)(rec->status == BTREE_STATUS_INSERTED);

		tree->rnode_count += inc;
		*szp += inc;

		return ret;
	}
	tree->rnode_count += 1;

	if (__btree_predict_false(idx == BTREE_MAX_LEVEL))
		return BTREE_RESULT_DEPTH_LIMIT;

	root_split = (level == idx);

	/*
	 * Allocate overflow nodes for division.
	 */

	next = btree_request_lnode(tree);
	if (__btree_predict_false(next == NULL))
		return BTREE_RESULT_ALLOCL_FAILURE;

	ret = btree_request_bstack(tree, idx, root_split, dsp);
	if (__btree_predict_false(ret != BTREE_RESULT_OK)) {
		btree_release_lnode(tree, next);

		return ret;
	}

	/*
	 * Perform divisions along applicable path prefix.
	 */

	ret = btree_divide_lnode(node, next, nth, key, rec);
	if (__btree_predict_false(ret != BTREE_RESULT_OK)) {
		btree_release_bstack(tree, idx, dsp);
		btree_release_lnode(tree, next);

		return ret;
	}
	tree->rnode_maximum = (next->lnode_next == NULL) ? next : tree->rnode_maximum;
	dsp[idx + 1].dest.lnode = next;

	pivot = btree_get_key(&next->lnode_rvect[0], next->lnode_rperm[0]);
	lsz = 9;
	rsz = 8;

	switch (idx) {
	case 8:
		dsp[8].node.bnode->bnode_svect[dsp[8].pos] = lsz;
		pivot = btree_divide_bnode(dsp[8].node.bnode, dsp[8].dest.bnode, dsp[8].pos, pivot, dsp[9].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 7:
		dsp[7].node.bnode->bnode_svect[dsp[7].pos] = lsz;
		pivot = btree_divide_bnode(dsp[7].node.bnode, dsp[7].dest.bnode, dsp[7].pos, pivot, dsp[8].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 6:
		dsp[6].node.bnode->bnode_svect[dsp[6].pos] = lsz;
		pivot = btree_divide_bnode(dsp[6].node.bnode, dsp[6].dest.bnode, dsp[6].pos, pivot, dsp[7].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 5:
		dsp[5].node.bnode->bnode_svect[dsp[5].pos] = lsz;
		pivot = btree_divide_bnode(dsp[5].node.bnode, dsp[5].dest.bnode, dsp[5].pos, pivot, dsp[6].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 4:
		dsp[4].node.bnode->bnode_svect[dsp[4].pos] = lsz;
		pivot = btree_divide_bnode(dsp[4].node.bnode, dsp[4].dest.bnode, dsp[4].pos, pivot, dsp[5].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 3:
		dsp[3].node.bnode->bnode_svect[dsp[3].pos] = lsz;
		pivot = btree_divide_bnode(dsp[3].node.bnode, dsp[3].dest.bnode, dsp[3].pos, pivot, dsp[4].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 2:
		dsp[2].node.bnode->bnode_svect[dsp[2].pos] = lsz;
		pivot = btree_divide_bnode(dsp[2].node.bnode, dsp[2].dest.bnode, dsp[2].pos, pivot, dsp[3].dest, rsz);
		lsz = 8;
		rsz = 7;
	case 1:
		dsp[1].node.bnode->bnode_svect[dsp[1].pos] = lsz;
		pivot = btree_divide_bnode(dsp[1].node.bnode, dsp[1].dest.bnode, dsp[1].pos, pivot, dsp[2].dest, rsz);
		lsz = 8;
		rsz = 7;
	}

	/*
	 * Populate new root with one key separating two subtrees upon root division and update bookkeeping.
	 */

	if (root_split) {
		root = btree_initroot_bnode(dsp[0].node.bnode);

		root->bnode_kvect[0] = pivot;
		root->bnode_svect[0] = lsz;
		root->bnode_svect[1] = rsz;

		root->bnode_tvect[0] = (idx == 0) ? BTREE_LNODEP(node) : dsp[1].node;
		root->bnode_tvect[1] = (idx == 0) ? BTREE_LNODEP(next) : dsp[1].dest;

		tree->rnode_root.bnode = root;
		tree->rnode_level = level + 1;
		tree->rnode_size = 1;

		return BTREE_RESULT_OK;
	}

	/*
	 * In the common case some branch will accomodate subtree division in place.
	 */

	dsp[0].node.bnode->bnode_svect[dsp[0].pos] = lsz;
	btree_insert_bnode(dsp[0].node.bnode, dsp[0].pos, pivot, dsp[1].dest, rsz);
	*top += 1;

	return BTREE_RESULT_OK;
}

int
btree_delete(BTREE_RNODE * const restrict tree, const BTREE_KEY key, BTREE_RECORD * restrict rec)
{
	/* XXXtodo */
}
