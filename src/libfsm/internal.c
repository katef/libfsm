#include "internal.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void
state_array_clear(struct state_array *arr)
{
	arr->len = 0;
}

struct state_array *
state_array_add(struct state_array *arr, struct fsm_state *st)
{
	if (arr->len >= arr->cap) {
		struct fsm_state **new;
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

	arr->states[arr->len] = st;
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

