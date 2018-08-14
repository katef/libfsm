/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "buf.h"

#include <assert.h>
#include <stdio.h>

#include <adt/xalloc.h>

#define LOG(...)

#define DEF_CEIL2 4
#define REGEX_BUF_MAX (1024LU * 1024)

static bool needs_escape(char c);

struct buf *
buf_new(void)
{
	struct buf *new;

	new = xmalloc(sizeof *new);

	new->size  = 0;
	new->buf   = xmalloc(1LLU << DEF_CEIL2);
	new->ceil2 = DEF_CEIL2;
	new->stack = NULL;

	LOG("NEW %p(%p)\n", (void *) new, (void *) new->buf);

	return new;
}

void
buf_free(struct buf *buf)
{
	assert(buf->stack == NULL);

	free(buf->buf);
	free(buf);
}

bool
buf_grow(struct buf *buf, size_t size)
{
	uint8_t nceil2;
	size_t nsize;
	uint8_t *nbuf;

	if ((1LLU << buf->ceil2) >= size) {
		return true; /* no change */
	}

	nceil2 = buf->ceil2;
	nsize = 1LLU << nceil2;

	while (nsize < size) {
		nceil2++;
		nsize = 1LLU << nceil2;
		if (nsize > REGEX_BUF_MAX) {
			assert(false);
			return false;
		}
	}

	nbuf = xrealloc(buf->buf, nsize);

	buf->ceil2 = nceil2;
	buf->buf   = nbuf;

	return true;
}

bool
buf_append(struct buf *buf, char c, bool escape)
{
	if (escape && needs_escape(c)) {
		if (!buf_grow(buf, buf->size + 2)) {
			return false;
		}

		buf->buf[buf->size++] = '\\';
		buf->buf[buf->size++] = c;
	} else {
		if (!buf_grow(buf, buf->size + 1)) {
			return false;
		}

		buf->buf[buf->size++] = c;
	}

	return true;
}

/*
 * memcpy raw to buf (growing it as necessary),
 * with any characters that are part of regex syntax escaped.
 */
bool
buf_memcpy(struct buf *buf, uint8_t *raw, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		if (!buf_append(buf, raw[i], true)) {
			return false;
		}
	}

	return true;
}

bool
buf_push(struct buf *buf)
{
	struct buf *nbuf;
	uint8_t *empty;

	nbuf = buf_new();
	if (nbuf == NULL) {
		return false;
	}

	empty = nbuf->buf;
	nbuf->buf   = buf->buf;
	nbuf->size  = buf->size;
	nbuf->ceil2 = buf->ceil2;
	nbuf->stack = buf->stack;

	buf->ceil2 = DEF_CEIL2;
	buf->size  = 0;
	buf->buf   = empty;
	buf->stack = nbuf;

	LOG("PUSHED, now %p[%p] => %p[%p]\n",
		(void *) buf, (void *) buf->buf,
		(void *) buf->stack, (void *) buf->stack->buf);

	return true;
}

struct buf *
buf_pop(struct buf *curr)
{
	struct buf *old;
	size_t osize, oceil2;
	uint8_t *obuf;

	size_t nsize, nceil2;
	uint8_t *nbuf;

	old = curr->stack;

	LOG("BUF_POP: curr %p, curr->stack %p\n",
	    (void *) curr, (void *) old);

	assert(old != NULL);

	/*
	 * Swap fields for the current head and its next link,
	 * then retun the unlinked buffer.
	 */

	osize  = old->size;
	oceil2 = old->ceil2;
	obuf   = old->buf;

	nsize  = curr->size;
	nceil2 = curr->ceil2;
	nbuf   = curr->buf;

	curr->stack = old->stack;

	old->size  = nsize;
	old->ceil2 = nceil2;
	old->buf   = nbuf;
	old->stack = NULL;

	curr->size = osize;
	curr->ceil2 = oceil2;
	curr->buf = obuf;

	LOG("POPPING, now %p[%p], unlinked %p[%p]\n",
	    (void *) curr, (void *) curr->buf,
	    (void *) old, (void *) old->buf);

	return old;
}

static bool
needs_escape(char c)
{
	switch (c) {
	case '[':
	case ']':
	case '?':
	case '+':
	case '*':
	case '.':
	case '-':
	case '|':
	case '(':
	case ')':
	case '{':
	case '}':
	case '\\':

	/* NYI */
	/* case '^': */
	/* case '$': */
		return true;

	default:
		return false;
	}
}

