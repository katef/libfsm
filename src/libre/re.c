/* $Id$ */

/*
 * TODO: libre.so API frontend
 * TODO: (other libraries go under form/ somewhere for libre_whatever.so
 * and depend on libre.so.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
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
	enum re_cflags cflags, enum re_err *err)
{
	struct fsm *new;
	struct fsm *(*comp)(int (*getc)(void *opaque), void *opaque,
		enum re_cflags cflags, enum re_err *err);

	assert(getc != NULL);

	switch (form) {
	case RE_LITERAL: comp = comp_literal; break;
	case RE_GLOB:    comp = comp_glob;    break;
	case RE_SIMPLE:  comp = comp_simple;  break;

	default:
		*err = RE_EBADFORM;
		return NULL;
	}

	new = comp(getc, opaque, cflags, err);
	if (new == NULL) {
		return NULL;
	}

	return new;
}

const char *
re_strerror(enum re_err err)
{
	switch (err) {
	case RE_ESUCCESS: return "success";
	case RE_ENOMEM:   return strerror(ENOMEM);
	case RE_EBADFORM: return "bad form";
	case RE_EXSUB:    return "syntax error: expected sub-expression";
	case RE_EXTERM:   return "syntax error: expected group term";
	case RE_EXGROUP:  return "syntax error: expected group";
	case RE_EXITEM:   return "syntax error: expected item";
	case RE_EXCOUNT:  return "syntax error: expected count";
	case RE_EXITEMS:  return "syntax error: expected items list";
	case RE_EXALTS:   return "syntax error: expected alternative list";
	case RE_EXEOF:    return "syntax error: expected EOF";
	}

	assert(!"unreached");
	return NULL;
}

void
re_exec(const struct fsm *fsm, const char *s, enum re_eflags eflags)
{
	/* TODO */
}

