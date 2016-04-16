/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include "../libfsm/internal.h" /* XXX */

#include <adt/bitmap.h>

int
bm_get(struct bm *bm, size_t i)
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

