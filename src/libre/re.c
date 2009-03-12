/* $Id$ */

/*
 * TODO: libre.so API frontend
 * TODO: (other libraries go under form/ somewhere for libre_whatever.so
 * and depend on libre.so.
 */

#include <stdio.h>	/* XXX */
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <re/re.h>
#include <fsm/fsm.h>
#include <fsm/out.h>	/* XXX */

#include "internal.h"

#include "../form/comp.h"

struct re *
re_new_empty(void)
{
	struct re *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->fsm = fsm_new();
	if (new->fsm == NULL) {
		free(new);
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

struct re *
re_new_comp(enum re_form form, const char *s, void *opaque, enum re_cflags cflags, enum re_err *err)
{
	struct re *new;
	enum re_err e;
	struct re *(*comp)(const char *s, enum re_err *err);

	assert(s != NULL);

	switch (form) {
	case RE_GLOB:   comp = comp_glob;   break;
	case RE_SIMPLE: comp = comp_simple; break;
	default: e = RE_EBADFORM;           goto error;
	}

	new = comp(s, err);
	if (new == NULL) {
		return NULL;
	}

/* XXX */
fsm_print(new->fsm, stdout, FSM_OUT_FSM);

	return new;

error:

	if (err != NULL) {
		*err = e;
	}

	return NULL;
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

void *
re_merge(struct re *re, const struct re *new, void *opaque)
{
	/* TODO */
	return re;
}

void *
re_exec(const struct re *re, const char *s, enum re_eflags eflags)
{
	/* TODO */
	return NULL;
}

