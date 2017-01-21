/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/set.h>

#define SET_BASE_CAP		8

struct state_set {
	struct fsm_state **states;
	size_t nstates;
	size_t cap;
};

static void
print_set(const char *pref, struct state_set *s)
{
	size_t i;

	fprintf(stderr, "%s (%p):\n", pref, (void *)s);

	if (set_empty(s)) {
		fprintf(stderr, "\tempty\n");
		return;
	}

	fprintf(stderr, "\tnstate: %zu cap: %zu\n\t", s->nstates, s->cap);
	for (i = 0; i < s->nstates; i++) {
		fprintf(stderr, "%p ", (void *)s->states[i]);
	}
	fprintf(stderr, "\n");
}

/* Search returns where a thing would be if it was inserted
 */
static size_t
set_search(struct state_set *set, struct fsm_state *state)
{
	size_t start, end;
	size_t mid;
	void *a = state;

	start = mid = 0;
	end = set->nstates;

	while (start < end) {
		void *b;
		mid = start + (end - start) / 2;
		b = set->states[mid];
		if (a < b) {
			end = mid;
		} else if (a > b) {
			start = mid + 1;
		} else {
			return mid;
		}
	}

	return mid;
}

struct fsm_state *
set_addstate(struct state_set **set, struct fsm_state *state)
{
	struct state_set *s;
	size_t off = 0;

	assert(set != NULL);
	assert(state != NULL);

	s = *set;

	/* If the set is not initialized, go ahead and do that. Insert the new state at
	 * the front.
	 */
	if (s == NULL) {
		s = malloc(sizeof **set);
		if (s == NULL) {
			return NULL;
		}

		s->states = malloc(SET_BASE_CAP * sizeof (*s->states));

		if (s->states == NULL) {
			return NULL;
		}

		s->states[0] = state;
		s->nstates = 1;
		s->cap = SET_BASE_CAP;

		*set = s;

		assert(set_contains(*set, state));
		return state;
	}

	/* If the state already exists in the set, return success.
	 * XXX: Notify on success differently somehow when state was already there than
	 * if we successfully inserted a non-existing state.
	 */
	if (!set_empty(s)) {
		off = set_search(s, state);
		if (s->states[off] == state) {
			return state;
		}
	}

	/* We're at capacity. Get more */
	if (s->cap == s->nstates) {
		struct fsm_state **new;

		new = realloc(s->states, sizeof (*s->states) * (s->cap * 2));
		if (new == NULL) {
			return NULL;
		}

		s->states = new;
		s->cap *= 2;
	}

	if (state > s->states[off]) {
		off++;
	}

	memmove(&s->states[off + 1], &s->states[off],
	    (s->nstates - off) * sizeof (*s->states));

	s->states[off] = state;
	s->nstates++;

	assert(set_contains(s, state));

	return state;
}

void
set_remove(struct state_set **set, struct fsm_state *state)
{
	struct state_set *s = *set;
	size_t off;

	if (set_empty(s)) {
		return;
	}

	assert(state != NULL);

	off = set_search(s, state);
	if (s->states[off] == state) {
		if (off < s->nstates) {
			memmove(&s->states[off], &s->states[off + 1],
			    (s->nstates - off) * sizeof (*s->states));
		}

		s->nstates--;
	}

	assert(!set_contains(s, state));
}

void
set_free(struct state_set *set)
{

	if (set == NULL) {
		return;
	}

	free(set->states);
	free(set);
}

int
set_contains(const struct state_set *set, const struct fsm_state *state)
{
	size_t off;

	if (set_empty(set)) {
		return 0;
	}

	off = set_search(set, state);
	if (set->states[off] == state) {
		return 1;
	}

	return 0;
}

int
subsetof(const struct state_set *a, const struct state_set *b)
{
	size_t off;

	if ((!a && b) || (!b && a)) {
		return 0;
	} else if (!a && !b) {
		return 1;
	}

	for (off = 0; off < a->nstates; off++) {
		if (!set_contains(b, a->states[off])) {
			return 0;
		}
	}

	return 1;
}

int
set_equal(const struct state_set *a, const struct state_set *b)
{
	return subsetof(a, b) && subsetof(b, a);
}

void
set_merge(struct state_set **dst, struct state_set *src)
{
	size_t i;

	if (set_empty(src)) {
		return;
	}

	for (i = 0; i < src->nstates; i++) {
		set_addstate(dst, src->states[i]);
	}
}

int
set_empty(struct state_set *set)
{

	return set == NULL || set->nstates == 0;
}

struct fsm_state *
set_first(struct state_set *set, struct set_iter *i)
{

	assert(i != NULL);

	if (set_empty(set)) {
		i->set = NULL;
		return NULL;
	}

	i->cursor = 0;
	i->set = set;

	return i->set->states[i->cursor];
}

struct fsm_state *
set_next(struct set_iter *i)
{

	assert(i != NULL);

	i->cursor++;
	if (i->cursor >= i->set->nstates) {
		return NULL;
	}

	return i->set->states[i->cursor];
}

int
set_hasnext(struct set_iter *i)
{

	assert(i != NULL);

	return i->set && i->cursor + 1 < i->set->nstates;
}
