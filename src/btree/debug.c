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
 * Debugging: print out path guided by given key.
 */

static void
btree_print_bnode(FILE *out, BTREE_BNODE *node, const uint8_t count, void *subtree)
{
	void 			*ptr;
	BTREE_KEY 		key;
	uint8_t 		pos;
	uint8_t 		cnt;
	uint8_t 		i;

	fprintf(out, "    BNODE %p ARITY %u\n", node, count);

	ptr = node->bnode_tvect[node->bnode_tperm[0]].voidp;
	cnt = node->bnode_svect[0];

	fprintf(out, "        [00] (%16p %02u)%s\n", ptr, cnt, (ptr == subtree) ? " <--" : "");

	for (i = 1; i <= count; i++) {
		key = node->bnode_kvect[node->bnode_kperm[i]];
		ptr = node->bnode_tvect[node->bnode_tperm[i]].voidp;
		cnt = node->bnode_svect[i];

		fprintf(out, "        [%02u] (%16p %02u) ", i, ptr, cnt);
		btree_format_key(out, key);
		fprintf(out, "%s\n", (ptr == subtree) ? " <--" : "");
	}
}

static void
btree_print_lnode(FILE *out, BTREE_LNODE *node, const uint8_t count, BTREE_KEY key)
{
	BTREE_KEY 		cur;
	uint8_t 		idx;
	uint8_t 		i;

	fprintf(out, "    LNODE %16p SIZE %u (%u)\n", node, count, node->lnode_size);

	for (i = 0; i < count; i++) {
		idx = node->lnode_rperm[i];

		fprintf(out, "        [%02u] ", i);
		btree_format_row(out, node->lnode_rvect, idx);
		fprintf(out, "%s\n", btree_row_eq(node->lnode_rvect, idx, key) ? " <--" : "");
	}
}

/*
 * Public interface.
 */

void
btree_print_path(FILE *out, BTREE_RNODE *tree, const BTREE_KEY key)
{
	BTREE_NODEP 		tnp;
	BTREE_NODEP 		xnp;
	int 			ret;
	uint8_t 		xnt;
	uint8_t 		cnt;
	uint8_t 		pos;

	tnp = tree->rnode_root;
	cnt = tree->rnode_size;

	fprintf(out, "ROOT %p LEVEL %u SIZE %u MINIMUM %p MAXIMUM %p KEY ", tnp.bnode, tree->rnode_level, tree->rnode_size,
	    tree->rnode_minimum, tree->rnode_maximum);
	btree_format_key(out, key);
	fprintf(out, "\n");

	switch (tree->rnode_level) {
	case  9:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  8:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  7:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  6:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  5:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  4:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  3:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  2:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  1:
		pos = btree_bnode_glb(tnp.bnode, cnt, key);
		xnp = tnp.bnode->bnode_tvect[tnp.bnode->bnode_tperm[pos]];
		xnt = tnp.bnode->bnode_svect[pos];

		btree_print_bnode(out, tnp.bnode, cnt, xnp.bnode);
		tnp = xnp;
		cnt = xnt;
	case  0:
		btree_print_lnode(out, tnp.lnode, cnt, key);
		return ;
	}

	abort();
}
