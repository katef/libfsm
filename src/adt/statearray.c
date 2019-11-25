#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/statearray.h>

void
state_array_clear(struct state_array *arr)
{
	arr->len = 0;
}

struct state_array *
state_array_add(struct state_array *arr, fsm_state_t state)
{
	if (arr->len >= arr->cap) {
		fsm_state_t *new;
		size_t new_cap;

		if (arr->cap < 16) {
			new_cap = 16;
		} else if (arr->cap < 2048) {
			new_cap = arr->cap * 2;
		} else {
			new_cap = arr->cap + arr->cap/4;
		}

		new = realloc(arr->states, new_cap * sizeof arr->states[0]);
		if (new == NULL) {
			return NULL;
		}

		arr->states = new;
		arr->cap = new_cap;
	}

	assert(arr->len < arr->cap);
	assert(arr->states != NULL);

	arr->states[arr->len] = state;
	arr->len++;
	return arr;
}

struct state_array *
state_array_copy(struct state_array *dst, const struct state_array *src)
{
	if (src->len == 0) {
		dst->states = NULL;
		dst->len = dst->cap = 0;
		return dst;
	}

	dst->states = malloc(src->len * sizeof src->states[0]);
	if (dst->states == NULL) {
		return NULL;
	}

	dst->len = dst->cap = src->len;
	memcpy(dst->states, src->states, src->len * sizeof src->states[0]);

	return dst;
}

struct state_array *
state_array_copy_set(struct state_array *dst, const struct state_set *src)
{
	const fsm_state_t *states;
	size_t n;

	n = state_set_count(src);

	if (dst->cap < n) {
		fsm_state_t *tmp;

		tmp = realloc(dst->states, n * sizeof tmp[0]);
		if (tmp == NULL) {
			return NULL;
		}

		dst->states = tmp;
		dst->cap = n;
	}

	assert(dst->cap >= n);

	if (n > 0) {
		states = state_set_array(src);
		memcpy(dst->states, states, n * sizeof states[0]);
	}

	dst->len = n;

	return dst;
}

