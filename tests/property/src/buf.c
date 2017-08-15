#include "buf.h"

#include <assert.h>
#include <stdio.h>

#define LOG(...)

#define DEF_CEIL2 4
#define REGEX_BUF_MAX (1024LU * 1024)

static bool needs_escape(char c);

struct buf *buf_new(void)
{
	struct buf *res = calloc(1, sizeof(*res));
	if (res != NULL) {
		res->buf = malloc(1LLU << DEF_CEIL2);
		if (res->buf == NULL) {
			free(res);
			return NULL;
		}
		res->ceil2 = DEF_CEIL2;
	}
	LOG("NEW %p(%p)\n", (void *)res, (void *)res->buf);
	return res;
}
	
void buf_free(struct buf *buf)
{
	assert(buf->stack == NULL);
	free(buf->buf);
	free(buf);
}

bool buf_grow(struct buf *buf, size_t size)
{
	if ((1LLU << buf->ceil2) < size) {
		uint8_t nceil2 = buf->ceil2;
		size_t nsize = (1LLU << nceil2);
		while (nsize < size) {
			nceil2++;
			nsize = (1LLU << nceil2);
			if (nsize > REGEX_BUF_MAX) {
				assert(false);
				return false;
			}
		}

		uint8_t *nbuf = realloc(buf->buf, nsize);
		if (nbuf == NULL) {
			return false;
		} else {
			buf->ceil2 = nceil2;
			buf->buf = nbuf;
			return true;
		}		
	} else {
		return true;	/* no change */
	}
}

bool buf_append(struct buf *buf, char c, bool escape)
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

/* Memcpy RAW to BUF (growing it as necessary), with any characters
 * that are part of RE syntax escaped. */
bool buf_memcpy(struct buf *buf, uint8_t *raw, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (!buf_append(buf, raw[i], true)) { return false; }
	}
	return true;
}


bool buf_push(struct buf *buf)
{
	struct buf *nbuf = buf_new();
	if (nbuf == NULL) {
		return false;
	} else {
		uint8_t *empty = nbuf->buf;
		nbuf->buf = buf->buf;
		nbuf->size = buf->size;
		nbuf->ceil2 = buf->ceil2;
		nbuf->stack = buf->stack;
		
		buf->ceil2 = DEF_CEIL2;
		buf->size = 0;
		buf->buf = empty;
		buf->stack = nbuf;
		LOG("PUSHED, now %p[%p] => %p[%p]\n",
		    (void *)buf, (void *)buf->buf,
		    (void *)buf->stack, (void *)buf->stack->buf);
		return true;
	}
}

struct buf *buf_pop(struct buf *cur)
{
	struct buf *old_head = cur->stack;
	LOG("BUF_POP: cur %p, cur->stack %p\n",
	    (void *)cur, (void *)old_head);
	assert(old_head != NULL);

	/* Swap fields for the current head and its next link,
	 * then retun the unlinked buffer. */
	size_t osize = old_head->size;
	size_t oceil2 = old_head->ceil2;
	uint8_t *obuf = old_head->buf;
	
	size_t nsize = cur->size;
	size_t nceil2 = cur->ceil2;
	uint8_t *nbuf = cur->buf;

	cur->stack = old_head->stack;
	
	old_head->size = nsize;
	old_head->ceil2 = nceil2;
	old_head->buf = nbuf;
	old_head->stack = NULL;

	cur->size = osize;
	cur->ceil2 = oceil2;
	cur->buf = obuf;
	LOG("POPPING, now %p[%p], unlinked %p[%p]\n",
	    (void *)cur, (void *)cur->buf,
	    (void *)old_head, (void *)old_head->buf);
	return old_head;
}

static bool needs_escape(char c)
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
	default: return false;
	}
}
