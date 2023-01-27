/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_BITMAP_H
#define ADT_BITMAP_H

#include <stdint.h>
#include "print/esc.h"

struct fsm_state;
struct fsm_options;

struct bm {
	uint64_t map[(UCHAR_MAX + 1)/sizeof(uint64_t)];
};

int
bm_get(const struct bm *bm, size_t i);

void
bm_set(struct bm *bm, size_t i);

/* Get a writeable pointer to the Nth word of the char set bitmap,
 * or NULL if out of bounds. */
uint64_t *
bm_nth_word(struct bm *bm, size_t n);

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

#endif

