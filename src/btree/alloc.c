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
 * Default leaf and branch node allocation.
 */

#if BTREE_OPT_CUSTOM_ALLOC == 0

static BTREE_BNODE *
btree_request_bnode(BTREE_RNODE *tree __btree_unused)
{
	BTREE_BNODE * 		node;

	return aligned_alloc(64, ((sizeof(BTREE_BNODE) + 63) / 64) * 64);
}

static void
btree_release_bnode(BTREE_RNODE *tree __btree_unused, BTREE_BNODE *node)
{
	free(node);
}

static BTREE_LNODE *
btree_request_lnode(BTREE_RNODE *tree __btree_unused)
{
	BTREE_LNODE * 		node;

	return aligned_alloc(64, ((sizeof(BTREE_LNODE) + 63) / 64) * 64);
}

static void
btree_release_lnode(BTREE_RNODE *tree __btree_unused, BTREE_LNODE *node)
{
	free(node);
}

#endif /* BTREE_CUSTOM_ALLOC == 0 */

/*
 * Node division stack allocation.
 */

static int
btree_request_bstack(BTREE_RNODE *tree, const uint8_t frame, const bool root_split, BTREE_USLOT stack[])
{
	BTREE_BNODE * 		node;
	const uint8_t 		lo = root_split ? 0 : 1;
	uint8_t 		i;
	uint8_t 		j;

	/* Allocate sequence of overflow branches. */
	for (i = lo; i <= frame; i++) {
		node = btree_request_bnode(tree);
		if (node == NULL) {
			for (j = lo; j < i; j++)
				btree_release_bnode(tree, stack[i].dest.bnode);

			return BTREE_RESULT_ALLOCB_FAILURE;
		}

		/* XXX prefetch for write? */
		stack[i].dest.bnode = node;
	}

	/*
	 * Upon root split install new empty branch at path stack `0` and also record
	 * it on splitting stack in case we abort and release overflow branches. Else
	 * path stack position `0` is pointing to a branch that we know can accomodate
	 * in-place insert and splitting stack position `0` is marked unused in that
	 * case.
	 */
	if (root_split) {
		stack[0].node = stack[0].dest;
	} else
		stack[0].dest.bnode = NULL;

	return BTREE_RESULT_OK;
}

static void
btree_release_bstack(BTREE_RNODE *tree, const uint8_t frame, BTREE_USLOT stack[])
{
	uint8_t 		i;

	for (i = 1; i <= frame; i++)
		btree_release_bnode(tree, stack[i].dest.bnode);

	if (stack[0].dest.bnode != NULL)
		btree_release_bnode(tree, stack[0].dest.bnode);
}
