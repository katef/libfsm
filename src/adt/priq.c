#include <assert.h>
#include <stdlib.h>

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
	struct priq **p, **pp;

	pp = NULL;
	for (p = priq; *p != NULL; p = &(*p)->next) {
		if ((*p) == s) {
			break;
		}
		pp = p;
	}

	if (!pp) {
		return;
	}

	(*pp)->next = s->next;

	for (pp = priq; *pp != NULL; pp = &(*pp)->next) {
		if ((*pp)->cost >= cost) {
			break;
		}
	}

	s->next = *pp;
	*pp = s;
}

struct priq *
priq_push(struct priq **priq,
	struct fsm_state *state, unsigned int cost)
{
	struct priq *new;
	struct priq **p;

	assert(priq != NULL);
	assert(state != NULL);

	/*
	 * This is more expensive than it could be; I'm
	 * opting for simplicity of implementation here.
	 */

	for (p = priq; *p != NULL; p = &(*p)->next) {
		if ((*p)->cost >= cost) {
			break;
		}
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->cost  = cost;
	new->state = state;
	new->prev  = NULL;
	new->type  = -1; /* XXX: nothing meaningful to set here */

	new->next = *p;
	*p = new;

	return new;
}

struct priq *
priq_find(struct priq *priq, const struct fsm_state *state)
{
	struct priq *p;

	assert(state != NULL);

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
priq_free(struct priq *priq)
{
	struct priq *p;
	struct priq *next;

	for (p = priq; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

