/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <re/re.h>
#include <re/group.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/graph.h>

#include "../libfsm/internal.h" /* XXX */

#include <adt/bitmap.h>

#include "internal.h"

#include "form/group.h"
#include "form/comp.h"

static const struct form {
	enum re_form form;
	struct fsm *(*comp)(int (*f)(void *opaque), void *opaque,
		enum re_flags flags, struct re_err *err);
	int
	(*group)(int (*f)(void *opaque), void *opaque,
		enum re_flags flags, struct re_err *err, struct re_grp *g);
} re_form[] = {
	{ RE_LITERAL, comp_literal, NULL         },
	{ RE_GLOB,    comp_glob,    NULL         },
	{ RE_SIMPLE,  comp_simple,  group_simple }
};

int
re_flags(const char *s, enum re_flags *f)
{
	const char *p;

	assert(s != NULL);
	assert(f != NULL);

	*f = 0U;

	/* defaults */
	*f |= RE_ZONE;

	for (p = s; *p != '\0'; p++) {
		if (*p & RE_ANCHOR) {
			*f &= ~RE_ANCHOR;
		}

		switch (*p) {
		case 'i': *f |= RE_ICASE;   break;
		case 'g': *f |= RE_TEXT;    break;
		case 'm': *f |= RE_MULTI;   break;
		case 'r': *f |= RE_REVERSE; break;
		case 's': *f |= RE_SINGLE;  break;
		case 'z': *f |= RE_ZONE;    break;

		default:
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

struct fsm *
re_new_empty(void)
{
	struct fsm *new;
	struct fsm_state *start;

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	start = fsm_addstate(new);
	if (start == NULL) {
		goto error;
	}

	fsm_setstart(new, start);

	return new;

error:

	fsm_free(new);

	return NULL;
}

struct fsm *
re_new_comp(enum re_form form, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err)
{
	const struct form *m;
	struct fsm *new;
	size_t i;

	assert(getc != NULL);

	for (i = 0; i < sizeof re_form / sizeof *re_form; i++) {
		if (re_form[i].form == form) {
			m = &re_form[i];
			break;
		}
	}

	if (i == sizeof re_form / sizeof *re_form) {
		if (err != NULL) {
			err->e = RE_EBADFORM;
		}
		return NULL;
	}

	new = m->comp(getc, opaque, flags, err);
	if (new == NULL) {
		return NULL;
	}

	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (flags & RE_REVERSE) {
		if (!fsm_reverse(new)) {
			goto error;
		}
	}

	if (flags & RE_ICASE) {
		if (!fsm_desensitise(new)) {
			goto error;
		}
	}

	return new;

error:

	fsm_free(new);

	if (err != NULL) {
		err->e = RE_EERRNO;
	}

	return NULL;
}

int
re_group_print(enum re_form form, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err,
	FILE *f,
	int (*escputc)(int c, FILE *f))
{
	const struct form *m;
	struct re_grp g;
	size_t i;
	int r;

	assert(getc != NULL);
	assert(escputc != NULL);
	assert(f != NULL);

	for (i = 0; i < sizeof re_form / sizeof *re_form; i++) {
		if (re_form[i].form == form) {
			m = &re_form[i];
			break;
		}
	}

	if (i == sizeof re_form / sizeof *re_form) {
		if (err != NULL) {
			err->e = RE_EBADFORM;
		}
		return -1;
	}

	if (m->group == NULL) {
		if (err != NULL) {
			/* TODO: specific error for not supported by this dialect */
			err->e = RE_EBADFORM;
		}
		return -1;
	}

	if (-1 == m->group(getc, opaque, flags, err, &g)) {
		return -1;
	}

	if (flags & RE_ICASE) {
		/* TODO: bm_desensitise */
	}

	/* TODO: populate re_err error for non-empty conflict set */

	r = bm_print(f, &g.set, escputc);
	if (r == -1) {
		goto error;
	}

	return r;

error:

	if (err != NULL) {
		err->e = RE_EERRNO;
	}

	return -1;
}

int
re_group_snprint(enum re_form form, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err,
	char *s, size_t n,
	int (*escputc)(int c, FILE *f))
{
	const struct form *m;
	struct re_grp g;
	size_t i;
	int r;

	assert(getc != NULL);
	assert(escputc != NULL);

	if (n == 0) {
		return 0;
	}

	assert(s != NULL);

	for (i = 0; i < sizeof re_form / sizeof *re_form; i++) {
		if (re_form[i].form == form) {
			m = &re_form[i];
			break;
		}
	}

	if (i == sizeof re_form / sizeof *re_form) {
		if (err != NULL) {
			err->e = RE_EBADFORM;
		}
		return -1;
	}

	if (m->group == NULL) {
		if (err != NULL) {
			/* TODO: specific error for not supported by this dialect */
			err->e = RE_EBADFORM;
		}
		return -1;
	}

	if (-1 == m->group(getc, opaque, flags, err, &g)) {
		return -1;
	}

	if (flags & RE_ICASE) {
		/* TODO: bm_desensitise */
	}

	/* TODO: populate re_err error for non-empty conflict set */

	r = bm_snprint(&g.set, s, n, escputc);
	if (r == -1) {
		goto error;
	}

	return r;

error:

	if (err != NULL) {
		err->e = RE_EERRNO;
	}

	return -1;
}

