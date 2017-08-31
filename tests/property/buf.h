/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef BUF_H
#define BUF_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct buf {
	uint8_t ceil2;
	size_t size;
	uint8_t  *buf;
	struct buf *stack;
};

struct buf *buf_new(void);
void buf_free(struct buf *buf);

/* Grow the buffer so it can fit at least size bytes. */
bool buf_grow(struct buf *buf, size_t size);

/* Append a single char (with optional escaping). */
bool buf_append(struct buf *buf, char c, bool escape);

/* Memcpy RAW to BUF (growing it as necessary), with any characters
 * that are part of RE syntax escaped. */
bool buf_memcpy(struct buf *buf, uint8_t *raw, size_t size);

/* Push the current buffer on a stack, getting a new empty buffer. */
bool buf_push(struct buf *buf);

/* Pop the buffer stack. */
struct buf *buf_pop(struct buf *buf);

#endif
