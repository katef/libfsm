/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include <adt/bitmap.h>
#include <adt/u64bitset.h>

#include <print/esc.h>

int
bm_get(const struct bm *bm, size_t i)
{
	assert(bm != NULL);
	assert(i <= UCHAR_MAX);

	return 0 != u64bitset_get(bm->map, i);
}

void
bm_set(struct bm *bm, size_t i)
{
	assert(bm != NULL);
	assert(i <= UCHAR_MAX);

	u64bitset_set(bm->map, i);
}

void
bm_unset(struct bm *bm, size_t i)
{
	assert(bm != NULL);
	assert(i <= UCHAR_MAX);

	u64bitset_clear(bm->map, i);
}

/*
 * Suffix masks in .rodata: entry b has bits b..255 set (and entry 256 is
 * all zeros), so the inclusive range x..y is bm_range_lo[x] & ~bm_range_lo[y + 1].
 * Keeping the table const lets compilers prove non-aliasing against *bm and
 * emit wide vector loads and stores without runtime initialization.
 */
#define BM_LO_WORD(b, w) \
	((b) <= (w) * 64 ? ~UINT64_C(0) : \
	((b) >= ((w) + 1) * 64 ? UINT64_C(0) : \
	(~UINT64_C(0) << (((b) - (w) * 64) & 63))))

#define BM_LO_ENTRY(b) { \
	BM_LO_WORD(b, 0), BM_LO_WORD(b, 1), \
	BM_LO_WORD(b, 2), BM_LO_WORD(b, 3)  \
}

#define BM_LUT4(F, b)  F(b), F((b) + 1), F((b) + 2), F((b) + 3)
#define BM_LUT16(F, b) BM_LUT4(F, b), BM_LUT4(F, (b) + 4), BM_LUT4(F, (b) + 8), BM_LUT4(F, (b) + 12)
#define BM_LUT64(F, b) BM_LUT16(F, b), BM_LUT16(F, (b) + 16), BM_LUT16(F, (b) + 32), BM_LUT16(F, (b) + 48)
#define BM_LUT256(F)   BM_LUT64(F, 0), BM_LUT64(F, 64), BM_LUT64(F, 128), BM_LUT64(F, 192)

static const uint64_t bm_range_lo[UCHAR_MAX + 2][4] = {
	BM_LUT256(BM_LO_ENTRY),
	{ 0, 0, 0, 0 }
};

#undef BM_LUT256
#undef BM_LUT64
#undef BM_LUT16
#undef BM_LUT4
#undef BM_LO_ENTRY
#undef BM_LO_WORD

void
bm_range(struct bm *bm, size_t x, size_t y)
{
	assert(bm != NULL);
	assert(x <= y);
	assert(y <= UCHAR_MAX);
	assert(sizeof bm->map / sizeof *bm->map == 4);

	for (size_t i = 0; i < sizeof bm->map / sizeof *bm->map; i++) {
		bm->map[i] |= bm_range_lo[x][i] & ~bm_range_lo[y + 1][i];
	}
}

size_t
bm_next(const struct bm *bm, int i, int value)
{
	size_t n;

	assert(bm != NULL);
	assert(i / CHAR_BIT < UCHAR_MAX);

	/* this could be faster by incrementing per element instead of per bit */
	for (n = i + 1; n <= UCHAR_MAX; n++) {
		if (!u64bitset_get(bm->map, n) == !value) {
			return n;
		}
	}

	return UCHAR_MAX + 1;
}

unsigned int
bm_count(const struct bm *bm)
{
	unsigned int count;
	size_t n;

	assert(bm != NULL);

	count = 0;

	for (n = 0; n < sizeof(bm->map)/sizeof(bm->map[0]); n++) {
		count += u64bitset_popcount(bm->map[n]);
	}

	return count;
}

void
bm_clear(struct bm *bm)
{
	assert(bm != NULL);

	for (size_t n = 0; n < sizeof(bm->map)/sizeof(bm->map[0]); n++) {
		bm->map[n] = 0;
	}
}

void
bm_invert(struct bm *bm)
{
	size_t n;

	assert(bm != NULL);

	for (n = 0; n < sizeof(bm->map)/sizeof(bm->map[0]); n++) {
		bm->map[n] = ~bm->map[n];
	}
}

int
bm_print(FILE *f, const struct fsm_options *opt, const struct bm *bm,
	int boxed,
	escputc *escputc)
{
	unsigned int count;
	int hi, lo;
	int r, n;

	enum {
		MODE_INVERT,
		MODE_SINGLE,
		MODE_ANY,
		MODE_MANY,
		MODE_NONE
	} mode;

	assert(f != NULL);
	assert(bm != NULL);
	assert(escputc != NULL);

	count = bm_count(bm);

	if (count == 0) {
		mode = MODE_NONE;
	} else if (count == 1 && !boxed) {
		mode = MODE_SINGLE;
	} else if (bm_next(bm, UCHAR_MAX, 1) != UCHAR_MAX + 1) {
		mode = MODE_MANY;
	} else if (count == UCHAR_MAX + 1) {
		mode = MODE_ANY;
	} else if (count <= UCHAR_MAX / 2) {
		mode = MODE_MANY;
	} else {
		mode = MODE_INVERT;
	}

	/* TODO: would prefer to show ranges before other characters.
	 * maybe best to do that by splitting the bitmap, and using two passes */
	/* XXX: all literal characters here really should go through escputc */

	if (mode == MODE_ANY && !boxed) {
		return fputs("/./", f);
	} else if (mode == MODE_ANY || mode == MODE_NONE) {
		return fprintf(f, "%s%s\\x00-\\xff%s",
			boxed ? "[" : "",
			mode == MODE_NONE ? "^" : "",
			boxed ? "]" : "");
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

		/* end of range */
		hi = bm_next(bm, lo, mode == MODE_INVERT);

		if (!isalnum((unsigned char) lo) && isalnum((unsigned char) hi)) {
			r = escputc(f, opt, lo);
			if (r == -1) {
				return -1;
			}
			n += r;

			hi = lo;
			continue;
		}

		/* bring down endpoint, if it's past the end of the class */
		if (isalnum((unsigned char) lo)) {
			size_t i;

			const struct {
				int (*is)(int);
				int end;
			} a[] = {
				{ isdigit, '9' },
				{ isupper, 'Z' },
				{ islower, 'z' }
			};

			/* XXX: assumes ASCII */
			for (i = 0; i < sizeof a / sizeof *a; i++) {
				if (a[i].is((unsigned char) lo)) {
					if (!a[i].is((unsigned char) hi)) {
						hi = a[i].end + 1;
					}
					break;
				}
			}

			assert(i < sizeof a / sizeof *a);
		}

		assert(hi > lo);

		switch (hi - lo) {
		case 1:
		case 2:
		case 3:
			r = escputc(f, opt, lo);
			if (r == -1) {
				return -1;
			}
			n += r;

			hi = lo;
			break;

		default:
			r = escputc(f, opt, lo);
			if (r == -1) {
				return -1;
			}
			n += r;

			r = putc('-', f);
			if (r == EOF) {
				return -1;
			}
			n += 1;

			r = escputc(f, opt, hi - 1);
			if (r == -1) {
				return -1;
			}
			n += r;

			/* hi may have been moved down within the set,
			 * this is a cheesy way to ensure we include it
			 * for bm_next() on the next iteration */
			hi--;

			break;
		}
	}

	r = fprintf(f, "%s", mode == MODE_SINGLE ? "" : "]");
	if (r == -1) {
		return -1;
	}
	n += r;

	return n;
}

int
bm_snprint(const struct bm *bm, const struct fsm_options *opt,
	char *s, size_t n,
	int boxed,
	escputc *escputc)
{
	FILE *f;
	int r;

	assert(bm != NULL);
	assert(escputc != NULL);

	if (n == 0) {
		return 0;
	}

	assert(s != NULL);

	/*
	 * I didn't want two sets of escputc functions (for strings and files),
	 * so I've nominated to use FILE * as the common interface,
	 * in order to keep just one kind of escputc.
	 *
	 * I also didn't want to duplicate the contents of bm_print(),
	 * so here I'm opting to print to a temporary file, rewind,
	 * and read it back for string output. This is not ideal.
	 */

	f = tmpfile();
	if (f == NULL) {
		return -1;
	}

	r = bm_print(f, opt, bm, boxed, escputc);
	if (r == -1) {
		goto error;
	}

	assert(r >= 0);

	if (n > (size_t) r + 1) {
		n = (size_t) r + 1;
	}

	rewind(f);

	if (n - 1 != fread(s, 1, n - 1, f)) {
		/* short read or unexpected zero */
		goto error;
	}
	/* expected zero could also be an error, but we don't mind if it is
	 * (although setting errno is bad form) */

#ifndef NDEBUG
	assert(fgetc(f) == EOF);
	if (ferror(f)) {
		goto error;
	}
	assert(feof(f));
#endif

	s[n - 1] = '\0';

	if (EOF == fclose(f)) {
		return -1;
	}

	return r;

error:

	(void) fclose(f);

	return -1;
}

void
bm_copy(struct bm *dst, const struct bm *src)
{
	memcpy(dst, src, sizeof(*src));
}

void
bm_intersect(struct bm *dst, const struct bm *src)
{
	for (size_t i = 0; i < sizeof(src->map)/sizeof(src->map[0]); i++) {
		dst->map[i] &= src->map[i];
	}
}

void
bm_union(struct bm *dst, const struct bm *src)
{
	for (size_t i = 0; i < sizeof(src->map)/sizeof(src->map[0]); i++) {
		dst->map[i] |= src->map[i];
	}
}

int
bm_any(const struct bm *bm)
{
	for (size_t i = 0; i < sizeof(bm->map)/sizeof(bm->map[0]); i++) {
		if (bm->map[i]) { return 1; }
	}
	return 0;
}
