/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "../libfsm/internal.h" /* XXX */

#include "dialect/comp.h"

struct dialect {
	enum re_dialect dialect;
	struct fsm *(*comp)(int (*f)(void *opaque), void *opaque,
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err);
	int overlap;
};

static const struct dialect *
re_dialect(enum re_dialect dialect)
{
	size_t i;

	static const struct dialect a[] = {
		{ RE_LIKE,    comp_like,    0 },
		{ RE_LITERAL, comp_literal, 0 },
		{ RE_GLOB,    comp_glob,    0 },
		{ RE_NATIVE,  comp_native,  0 },
		{ RE_PCRE,    comp_pcre,    0 },
		{ RE_SQL,     comp_sql,     1 }
	};

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].dialect == dialect) {
			return &a[i];
		}
	}

	return NULL;
}

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
re_comp(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err)
{
	const struct dialect *m;
	struct fsm *new;

	assert(getc != NULL);

	m = re_dialect(dialect);
	if (m == NULL) {
		if (err != NULL) {
			err->e = RE_EBADDIALECT;
		}
		return NULL;
	}

	new = m->comp(getc, opaque, opt, flags, m->overlap, err);
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

	return new;

error:

	fsm_free(new);

	if (err != NULL) {
		err->e = RE_EERRNO;
	}

	return NULL;
}

