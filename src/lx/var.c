/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "var.h"

struct var {
	const char *name;
	struct fsm *fsm;
	struct var *next;
};

static struct var *
find(const struct var *head, const char *name)
{
	const struct var *p;

	assert(name != NULL);

	for (p = head; p != NULL; p = p->next) {
		if (0 == strcmp(p->name, name)) {
			return (struct var *) p;
		}
	}

	return NULL;
}

struct var *
var_bind(struct var **head, const char *name, struct fsm *fsm)
{
	struct var *new;

	assert(head != NULL);
	assert(name != NULL);
	assert(fsm != NULL);

	new = find(*head, name);
	if (new != NULL) {
		new->fsm = fsm;
		return new;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->name = name;
	new->fsm  = fsm;

	new->next = *head;
	*head = new;

	return new;
}

struct fsm *
var_find(const struct var *head, const char *name)
{
	struct var *p;

	assert(name != NULL);

	p = find(head, name);
	if (p == NULL) {
		return NULL;
	}

	return p->fsm;
}

