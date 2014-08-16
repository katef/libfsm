/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/exec.h>
#include <fsm/graph.h>

#include "form/comp.h"

int
re_cflags(const char *s, enum re_cflags *f)
{
	const char *p;

	assert(s != NULL);
	assert(f != NULL);

	*f = 0U;

	for (p = s; *p != '\0'; p++) {
		switch (*p) {
		case 'i': *f |= RE_ICASE;   break;
		case 'g': *f |= RE_NEWLINE; break;
		case 'r': *f |= RE_REVERSE; break;

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
	enum re_cflags cflags, struct re_err *err)
{
	struct fsm *new;
	struct fsm *(*comp)(int (*getc)(void *opaque), void *opaque,
		enum re_cflags cflags, struct re_err *err);

	assert(getc != NULL);

	switch (form) {
	case RE_LITERAL: comp = comp_literal; break;
	case RE_GLOB:    comp = comp_glob;    break;
	case RE_SIMPLE:  comp = comp_simple;  break;

	default:
		if (err != NULL) {
			err->e = RE_EBADFORM;
		}
		return NULL;
	}

	new = comp(getc, opaque, cflags, err);
	if (new == NULL) {
		return NULL;
	}

	/*
	 * All cflags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (cflags & RE_REVERSE) {
		if (!fsm_reverse(new)) {
			goto error;
		}
	}

	if (cflags & RE_ICASE) {
		if (!fsm_desensitise(new)) {
			goto error;
		}
	}

	return new;

error:

	fsm_free(new);

	return NULL;
}

const char *
re_strerror(enum re_errno e)
{
	switch (e) {
	case RE_ESUCCESS: return "Success";
	case RE_ENOMEM:   return strerror(ENOMEM);
	case RE_EBADFORM: return "Bad form";
	case RE_EXSUB:    return "Syntax error: expected sub-expression";
	case RE_EXTERM:   return "Syntax error: expected group term";
	case RE_EXGROUP:  return "Syntax error: expected group";
	case RE_EXITEM:   return "Syntax error: expected item";
	case RE_EXCOUNT:  return "Syntax error: expected count";
	case RE_EXITEMS:  return "Syntax error: expected items list";
	case RE_EXALTS:   return "Syntax error: expected alternative list";
	case RE_EXEOF:    return "Syntax error: expected EOF";
	}

	assert(!"unreached");
	return NULL;
}

struct fsm_state *
re_exec(const struct fsm *fsm, const char *s, enum re_eflags eflags)
{
	assert(fsm != NULL);
	assert(s != NULL);

	/* TODO: eflags */

	return fsm_exec(fsm, fsm_sgetc, &s);
}

