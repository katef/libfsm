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

#include "internal.h"

#include "form/comp.h"

struct re *
re_new_empty(void)
{
	struct re *new;
	struct fsm_state *start;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->fsm = fsm_new();
	if (new->fsm == NULL) {
		free(new);
		return NULL;
	}

	start = fsm_addstate(new->fsm);
	if (start == NULL) {
		free(new);
		return NULL;
	}

	fsm_setstart(new->fsm, start);

	return new;
}

struct re *
re_new_comp(enum re_form form, int (*getc)(void *opaque), void *opaque,
	enum re_cflags cflags, enum re_err *err)
{
	struct re *new;
	struct re *(*comp)(int (*getc)(void *opaque), void *opaque, enum re_err *err);

	assert(getc != NULL);

	switch (form) {
	case RE_LITERAL: comp = comp_literal; break;
	case RE_GLOB:    comp = comp_glob;    break;
	case RE_SIMPLE:  comp = comp_simple;  break;

	default:
		*err = RE_EBADFORM;
		return NULL;
	}

	new = comp(getc, opaque, err);
	if (new == NULL) {
		return NULL;
	}

	return new;
}

void
re_free(struct re *re)
{
	fsm_free(re->fsm);
	free(re);
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

int
re_union(struct re *re, struct re *new)
{
	assert(re != NULL);
	assert(new != NULL);

	re->fsm = fsm_union(re->fsm, new->fsm);
	if (re->fsm == NULL) {
		return 0;
	}

	return 1;
}

int
re_concat(struct re *re, struct re *new)
{
	assert(re != NULL);
	assert(new != NULL);

	re->fsm = fsm_concat(re->fsm, new->fsm);
	if (re->fsm == NULL) {
		return 0;
	}

	return 1;
}

void
re_exec(const struct re *re, const char *s, enum re_eflags eflags)
{
	/* TODO */
}

