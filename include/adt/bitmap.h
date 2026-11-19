/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_BITMAP_H
#define ADT_BITMAP_H

#include <stdint.h>
#include <limits.h>

#include <stdio.h>
#include "print/esc.h"

struct fsm_state;
struct fsm_options;

struct bm {
	uint64_t map[(UCHAR_MAX + 1) / (sizeof (uint64_t) * CHAR_BIT)];
};

int
bm_get(const struct bm *bm, size_t i);

void
bm_set(struct bm *bm, size_t i);

void
bm_unset(struct bm *bm, size_t i);

void
bm_range(struct bm *bm, size_t x, size_t y);

size_t
bm_next(const struct bm *bm, int i, int value);

unsigned int
bm_count(const struct bm *bm);

void
bm_clear(struct bm *bm);

void
bm_invert(struct bm *bm);

int
bm_print(FILE *f, const struct fsm_options *opt, const struct bm *bm,
	int boxed,
	escputc *escputc);

int
bm_snprint(const struct bm *bm, const struct fsm_options *opt,
	char *s, size_t sz,
	int boxed,
	escputc *escputc);

void
bm_copy(struct bm *dst, const struct bm *src);

void
bm_intersect(struct bm *dst, const struct bm *src);

void
bm_union(struct bm *dst, const struct bm *src);

int
bm_any(const struct bm *bm);

#endif

