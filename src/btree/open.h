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

/*
 *     bool
 *     <btree name>_row_lt(const <key type> key, const <row type> rows[], uint8_t index)
 *
 *     bool
 *     <btree name>_row_eq(const <key type> key, const <row type> rows[], uint8_t index)
 *
 *     bool
 *     <btree name>_key_lt(const <key type> key, const <key type> keys[], uint8_t index)
 *
 *     void
 *     <btree name>_set_key(<row type> rows[], uint8_t index, const <key type> key)
 *
 *     <key type>
 *     <btree name>_get_key(const <row type> rows[], uint8_t index)
 *
 *     void
 *     <btree name>_release_row(<row type> rows[], uint8_t index)
 */

#ifdef BTREE_TMPL_H
#error "Duplicate or unterminated inclusion of <btree/btree_open.h>."
#endif

#define BTREE_TMPL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__SSE__) || !defined(__SSE2__) || !defined(__SSE3__) || !defined(__SSE4_1__)
#error "Current implementations requires SSE3, SSE3 and SSE4.1 extensions."
#endif

#include <emmintrin.h> 	/* [SSE2]   _mm_load_si128(), _mm_store_si128() */
#include <smmintrin.h> 	/* [SSE4.1] _mm_extract_epi8() */
#include <tmmintrin.h> 	/* [SSE3]   _mm_shuffle_epi8() */
#include <xmmintrin.h> 	/* [SSE]    _mm_prefetch() */

#ifndef BTREE_H
#include "btree/btree.h"
#endif

/*
 *
 */

#if !defined(BTREE_NAME)
#error "Macro BTREE_NAME must be defined to namespace prefix."
#endif

#if !defined(BTREE_ROW)
#error "Macro BTREE_ROW must be defined to row type."
#endif

#if !defined(BTREE_KEY)
#error "Macro BTREE_KEY must be defined to key type."
#endif

/*
 * Normalize compile time options.
 */

#if !defined(BTREE_OPT_DEBUG_LOOKUP) || BTREE_OPT_DEBUG_LOOKUP != 1
#define BTREE_OPT_DEBUG_LOOKUP 		0
#endif

#if !defined(BTREE_OPT_CUSTOM_ALLOC) || BTREE_OPT_CUSTOM_ALLOC != 1
#define BTREE_OPT_CUSTOM_ALLOC 		0
#endif

#if !defined(BTREE_OPT_ENABLE_PREFETCH) || BTREE_OPT_ENABLE_PREFETCH != 0
#define BTREE_OPT_ENABLE_PREFETCH 	1
#endif

#if !defined(BTREE_OPT_BNODE_SEARCH) || (BTREE_OPT_BNODE_SEARCH != 1 && BTREE_OPT_BNODE_SEARCH != 4)
#define BTREE_OPT_BNODE_SEARCH 		1
#endif

/*
 * Use standard compiler extensions.
 */

#define __btree_cache_aligned 	__attribute__((aligned(64)))
#define __btree_simd_aligned 	__attribute__((aligned(16)))
#define __btree_packed 		__attribute__((packed))
#define __btree_unused 		__attribute__((unused))

#define __btree_predict_false(e)	__builtin_expect((e) != 0, 0)
#define __btree_predict_true(e) 	__builtin_expect((e) != 0, 1)

/*
 *
 */

#define __MKNAME(x, y) 		x ## _ ## y
#define __MKSYM2(x, y) 		__MKNAME(x, y)
#define __MKSYM(x) 		__MKSYM2(BTREE_NAME, x)

/* Functions supplied by user. */
#define btree_row_lt 		__MKSYM(row_lt)
#define btree_row_eq 		__MKSYM(row_eq)
#define btree_key_lt 		__MKSYM(key_lt)
#define btree_set_key 		__MKSYM(set_key)
#define btree_get_key 		__MKSYM(get_key)
#define btree_release_row 	__MKSYM(release_row)
#define btree_format_key 	__MKSYM(format_key)
#define btree_format_row 	__MKSYM(format_row)

/* Public types. */
#define BTREE_RNODE 		struct __MKSYM(btree)
#define BTREE_RECORD 		struct __MKSYM(record)

/* Private types. */
#define BTREE_NODEP 		union __MKSYM(nodep)
#define BTREE_BNODE 		struct __MKSYM(bnode)
#define BTREE_LNODE 		struct __MKSYM(lnode)
#define BTREE_USLOT 		struct __MKSYM(uslot)

/* XXX remove these */
#define BTREE_FANOUT 		15
/* Leaf nodes at level 0 and branch node at levels [1, 9] for the total of 10 levels. */
#define BTREE_MAX_LEVEL 	9
#define BTREE_STACK_DEPTH 	10

/*
 * Symbol renaming.
 */

#define btree_request_lnode 	__MKSYM(request_lnode)
#define btree_request_bnode 	__MKSYM(request_bnode)
#define btree_release_lnode 	__MKSYM(release_lnode)
#define btree_release_bnode 	__MKSYM(release_bnode)
#define btree_request_bstack 	__MKSYM(request_bstack)
#define btree_release_bstack 	__MKSYM(release_bstack)

#define btree_bnode_glb 	__MKSYM(bnode_glb)
#define btree_insert_bnode 	__MKSYM(insert_bnode)
#define btree_delete_bnode 	__MKSYM(delete_bnode)
#define btree_divide_bnode 	__MKSYM(divide_bnode)

#define btree_lnode_lub 	__MKSYM(lnode_lub)
#define btree_lookup_lnode 	__MKSYM(lookup_lnode)
#define btree_delete_lnode 	__MKSYM(delete_lnode)
#define btree_update_lnode 	__MKSYM(update_lnode)
#define btree_divide_lnode 	__MKSYM(divide_lnode)

#define btree_init 		__MKSYM(init)
#define btree_free 		__MKSYM(free)
#define btree_lookup 		__MKSYM(lookup)
#define btree_update 		__MKSYM(update)
#define btree_delete 		__MKSYM(delete)
#define btree_drop_lnode 	__MKSYM(drop_lnode)
#define btree_drop_bnode 	__MKSYM(drop_bnode)
#define btree_initroot_bnode 	__MKSYM(initroot_bnode)
#define btree_prefetch_lnode 	__MKSYM(prefetch_lnode)
#define btree_prefetch_bnode 	__MKSYM(prefetch_bnode)

#define btree_print_path 	__MKSYM(print_path)
#define btree_print_bnode 	__MKSYM(print_bnode)
#define btree_print_lnode 	__MKSYM(print_lnode)

#define btree_extract_epi8 	__MKSYM(extract_epi8)

/*
 *
 */

#include "btree/types.h"

/*
 *
 */

#define BTREE_BNODEP(p) 	(BTREE_NODEP) { .bnode = (p) }
#define BTREE_LNODEP(p) 	(BTREE_NODEP) { .lnode = (p) }

#define BTREE_M128CP(p) 	((const __m128i *)(uintptr_t)(p))
#define BTREE_M128P(p) 		((__m128i *)(uintptr_t)(p))

/*
 *
 */

/* alloc.c */

static BTREE_LNODE * 		btree_request_lnode(BTREE_RNODE *);
static BTREE_BNODE * 		btree_request_bnode(BTREE_RNODE *);
static void 			btree_release_lnode(BTREE_RNODE *, BTREE_LNODE *);
static void 			btree_release_bnode(BTREE_RNODE *, BTREE_BNODE *);

static int 			btree_request_bstack(BTREE_RNODE *, const uint8_t, const bool, BTREE_USLOT []);
static void 			btree_release_bstack(BTREE_RNODE *, const uint8_t, BTREE_USLOT []);

/* bnode.c */

static const uint8_t 		btree_bnode_glb_01(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_02(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_03(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_04(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_05(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_06(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_07(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_08(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_09(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_10(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_11(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_12(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_13(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_14(BTREE_BNODE *, const BTREE_KEY);
static const uint8_t 		btree_bnode_glb_15(BTREE_BNODE *, const BTREE_KEY);

static const uint8_t 		btree_bnode_glb(BTREE_BNODE *, const uint8_t, const BTREE_KEY);
static void 			btree_insert_bnode(BTREE_BNODE *, const uint8_t, const BTREE_KEY, BTREE_NODEP, const uint8_t);
static void 			btree_delete_bnode(BTREE_BNODE *, const uint8_t);
static BTREE_KEY 		btree_divide_bnode(BTREE_BNODE * restrict, BTREE_BNODE * restrict, const uint8_t, BTREE_KEY,
				    BTREE_NODEP, const uint8_t);

/* lnode.c */

static const uint8_t 		btree_lnode_lub(BTREE_LNODE *, const uint8_t, const BTREE_KEY);
static int 			btree_lookup_lnode(BTREE_LNODE *, const uint8_t, const BTREE_KEY, BTREE_RECORD *);
static int 			btree_delete_lnode(BTREE_LNODE *, const uint8_t, const BTREE_KEY, BTREE_RECORD *);
static int 			btree_update_lnode(BTREE_LNODE *, const uint8_t, const BTREE_KEY, BTREE_RECORD *, uint8_t *);
static int 			btree_divide_lnode(BTREE_LNODE * restrict, BTREE_LNODE * restrict, const uint8_t, const BTREE_KEY,
				    BTREE_RECORD * restrict);

/* tree.c */

static BTREE_LNODE * 		btree_drop_lnode(BTREE_RNODE *, BTREE_LNODE *);
static void 			btree_drop_bnode(BTREE_RNODE *, BTREE_BNODE *, const uint8_t, const uint8_t);
static BTREE_BNODE * 		btree_initroot_bnode(BTREE_BNODE *);

static inline BTREE_NODEP 	btree_prefetch_lnode(BTREE_BNODE *, const uint8_t);
static inline BTREE_NODEP 	btree_prefetch_bnode(BTREE_BNODE *, const uint8_t);

BTREE_RNODE * 			btree_init(BTREE_RNODE *);
void 				btree_free(BTREE_RNODE *);
int 				btree_lookup(BTREE_RNODE *, const BTREE_KEY, BTREE_RECORD *);
int 				btree_update(BTREE_RNODE *, const BTREE_KEY, BTREE_RECORD *);
int 				btree_delete(BTREE_RNODE *, const BTREE_KEY, BTREE_RECORD *);

/* debug.c */

#if BTREE_OPT_DEBUG_LOOKUP == 1

void 				btree_print_path(FILE *, BTREE_RNODE *, const BTREE_KEY);

static void 			btree_print_bnode(FILE *, BTREE_BNODE *, const uint8_t, void *);
static void 			btree_print_lnode(FILE *, BTREE_LNODE *, const uint8_t, BTREE_KEY);

#endif /* BTREE_OPT_DEBUG_LOOKUP == 1 */
