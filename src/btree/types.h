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

BTREE_NODEP {
	BTREE_BNODE *		bnode;
	BTREE_LNODE * 		lnode;
	void * 			voidp;
};

BTREE_RNODE {
	BTREE_NODEP 		rnode_root;
	BTREE_LNODE * 		rnode_minimum;
	BTREE_LNODE * 		rnode_maximum;
	size_t 			rnode_count;
	uint8_t 		rnode_level;
	uint8_t 		rnode_size;
};

BTREE_LNODE {
	uint8_t 		lnode_rperm[16] __btree_simd_aligned;
	BTREE_LNODE * 		lnode_next;
	uint8_t 		lnode_size;
	BTREE_ROW 		lnode_rvect[16];
} __btree_cache_aligned;

BTREE_BNODE {
	uint8_t 		bnode_tperm[16] __btree_simd_aligned;
	uint8_t 		bnode_kperm[16] __btree_simd_aligned;
	uint8_t 		bnode_svect[16] __btree_simd_aligned;
	BTREE_KEY 		bnode_kvect[15];
	BTREE_NODEP 		bnode_tvect[16];
} __btree_cache_aligned;

BTREE_RECORD {
	BTREE_ROW 		*array;
	int 			status;
	uint8_t 		index;
	/* XXX also leaf address + nth, to facilitate iterations? */
};

/* XXX */
BTREE_USLOT {
	BTREE_NODEP 		node;
	BTREE_NODEP 		dest;
	uint8_t 		pos;
};
