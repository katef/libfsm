#ifndef ADT_STATEARRAY_H
#define ADT_STATEARRAY_H

#include <stddef.h>

struct fsm_state;
struct state_set;

struct state_array {
	struct fsm_state **states;
	size_t len;
	size_t cap;
};

void
state_array_clear(struct state_array *arr);

struct state_array *
state_array_add(struct state_array *arr, struct fsm_state *st);

struct state_array *
state_array_copy(struct state_array *dst, const struct state_array *src);

struct state_array *
state_array_copy_set(struct state_array *dst, const struct state_set *src);

#endif /* ADT_STATEARRAY_H */

