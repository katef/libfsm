/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/priq.h>

struct priq *
priq_pop(struct priq **priq)
{
	struct priq *p;

	if (*priq == NULL) {
		return NULL;
	}

	/*
	 * The minimum cost is always the first node, since this is
	 * a priority priq and therefore nodes are inserted in order.
	 */

	p = *priq;

	*priq = p->next;
	p->next = NULL;

	return p;
}

void
priq_update(struct priq **priq, struct priq *s, unsigned int cost)
{
	struct priq **p;
#if 0
	struct priq *next;
#endif

	assert(priq != NULL);
	assert(s != NULL);

#if 0
	p = priq;

	while (*p != NULL) {
		if (*p == s) {
			*p = (*p)->next;
			continue;
		}

		if ((*p)->cost >= cost) {
			next = *p;
			*p = s;
			p = &next->next;
			continue;
		}

		p = &(*p)->next;
	}

	s->next = next;
#endif

	for (p = priq; *p != NULL; p = &(*p)->next) {
		if (*p == s) {
			*p = (*p)->next;
			break;
		}
	}

	for (p = priq; *p != NULL; p = &(*p)->next) {
		if ((*p)->cost >= cost) {
			break;
		}
	}

	s->next = *p;
	*p = s;
}

struct priq *
priq_push(const struct fsm_alloc *a, struct priq **priq,
	fsm_state_t state, unsigned int cost)
{
	struct priq *new;
	struct priq **p;

	assert(priq != NULL);

	/*
	 * This is more expensive than it could be; I'm
	 * opting for simplicity of implementation here.
	 */

	for (p = priq; *p != NULL; p = &(*p)->next) {
		if ((*p)->cost >= cost) {
			break;
		}
	}

	new = f_malloc(a, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->cost  = cost;
	new->state = state;
	new->prev  = NULL;
	new->c     = '\0'; /* XXX: nothing meaningful to set here */

	new->next = *p;
	*p = new;

	return new;
}

struct priq *
priq_find(struct priq *priq, fsm_state_t state)
{
	struct priq *p;

	for (p = priq; p != NULL; p = p->next) {
		if (p->state == state) {
			return p;
		}
	}

	return NULL;
}

void
priq_move(struct priq **priq, struct priq *new)
{
	assert(priq != NULL);
	assert(new->next == NULL);

	new->next = *priq;
	*priq = new;
}

void
priq_free(const struct fsm_alloc *a, struct priq *priq)
{
	struct priq *p;
	struct priq *next;

	for (p = priq; p != NULL; p = next) {
		next = p->next;
		f_free(a, p);
	}
}

