/*
 * Copyright 2023 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_LITERAL_H
#define RE_LITERAL_H

struct re_err;

enum re_literal_category {
	RE_LITERAL_UNANCHORED    = 0,
	RE_LITERAL_ANCHOR_START  = 1 << 0,
	RE_LITERAL_ANCHOR_END    = 1 << 1,
	RE_LITERAL_ANCHOR_BOTH   = 0x3,
	RE_LITERAL_UNSATISFIABLE = 4,
};

int
re_is_literal(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err,
	enum re_literal_category *category, char **s, size_t *n);

#endif

