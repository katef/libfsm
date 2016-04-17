/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "../libfsm/internal.h" /* XXX */

#include <adt/bitmap.h>

int
bm_get(const struct bm *bm, size_t i)
{
	assert(bm != NULL);
	assert(i <= FSM_EDGE_MAX);

	return bm->map[i / CHAR_BIT] & (1 << i % CHAR_BIT);
}

void
bm_set(struct bm *bm, size_t i)
{
	assert(bm != NULL);
	assert(i <= FSM_EDGE_MAX);

	bm->map[i / CHAR_BIT] |=  (1 << i % CHAR_BIT);
}

size_t
bm_next(const struct bm *bm, int i, int value)
{
	size_t n;

	assert(bm != NULL);
	assert(i / CHAR_BIT < FSM_EDGE_MAX);

	/* this could be faster by incrementing per element instead of per bit */
	for (n = i + 1; n <= FSM_EDGE_MAX; n++) {
		/* ...and this could be faster by using peter wegner's method */
		if (!(bm->map[n / CHAR_BIT] & (1 << n % CHAR_BIT)) == !value) {
			return n;
		}
	}

	return FSM_EDGE_MAX + 1;
}

unsigned int
bm_count(const struct bm *bm)
{
	unsigned char c;
	unsigned int count;
	size_t n;

	assert(bm != NULL);

	count = 0;

	/* this could be faster using richard hamming's method */
	for (n = 0; n < sizeof bm->map; n++) {
		/* counting bits set for an element, peter wegner's method */
		for (c = bm->map[n]; c != 0; c &= c - 1) {
			count++;
		}
	}

	return count;
}

void
bm_clear(struct bm *bm)
{
	static const struct bm bm_empty;

	assert(bm != NULL);

	*bm = bm_empty;
}

void
bm_invert(struct bm *bm)
{
	size_t n;

	assert(bm != NULL);

	for (n = 0; n < sizeof bm->map; n++) {
		bm->map[n] = ~bm->map[n];
	}
}

int
bm_print(FILE *f, const struct bm *bm,
	int (*escputc)(int c, FILE *f))
{
	int count; /* TODO: unsigned? */
	int hi, lo;
	int r, n;

	enum {
		MODE_INVERT,
		MODE_SINGLE,
		MODE_ANY,
		MODE_MANY
	} mode;

	assert(f != NULL);
	assert(bm != NULL);
	assert(escputc != NULL);

	count = bm_count(bm);

	if (count == 1) {
		mode = MODE_SINGLE;
	} else if (bm_next(bm, UCHAR_MAX, 1) != FSM_EDGE_MAX + 1) {
		mode = MODE_MANY;
	} else if (count == UCHAR_MAX + 1) {
		mode = MODE_ANY;
	} else if (count <= UCHAR_MAX / 2) {
		mode = MODE_MANY;
	} else {
		mode = MODE_INVERT;
	}

	/* TODO: optionally disable MODE_ANY. maybe provide a mask of modes to allow */
	/* XXX: all literal characters here really should go through escputc */

	if (mode == MODE_ANY) {
		return fputs("/./", f);
	}

	n = 0;

	r = fprintf(f, "%s%s",
		mode == MODE_SINGLE ? "" : "[",
		mode != MODE_INVERT ? "" : "^");
	if (r == -1) {
		return -1;
	}
	n += r;

	hi = -1;
	for (;;) {
		/* start of range */
		lo = bm_next(bm, hi, mode != MODE_INVERT);
		if (lo > UCHAR_MAX) {
			break;
		}

		/* XXX: break range if endpoint belongs to different ctype.h class */
		/* or: only handle ranges for A-Z, a-z and 0-9 */

		/* end of range */
		hi = bm_next(bm, lo, mode == MODE_INVERT);
		if (hi > UCHAR_MAX) {
			hi = UCHAR_MAX;
		}

		assert(hi > lo);

		switch (hi - lo) {
		case 1:
		case 2:
		case 3:
			r = escputc(lo, f);
			if (r == -1) {
				return -1;
			}
			n += r;

			hi = lo;
			break;

		default:
			r = escputc(lo, f);
			if (r == -1) {
				return -1;
			}
			n += r;

			r = putc('-', f);
			if (r == -1) {
				return -1;
			}
			n += r;

			r = escputc(hi - 1, f);
			if (r == -1) {
				return -1;
			}
			n += r;
			break;
		}
	}

	if (bm_get(bm, FSM_EDGE_EPSILON)) {
		r = escputc(FSM_EDGE_EPSILON, f);
		if (r == -1) {
			return -1;
		}
		n += r;
	}

	r = fprintf(f, "%s", mode == MODE_SINGLE ? "" : "]");
	if (r == -1) {
		return -1;
	}
	n += r;

	return n;
}

