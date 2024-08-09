/*
 * Copyright 2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_RETLIST_H
#define FSM_INTERNAL_RETLIST_H

struct ir;

struct ret {
	size_t count;
	const fsm_end_id_t *ids;
};

struct ret_list {
	size_t count;
	struct ret *a;
};

struct ret *
find_ret(const struct ret_list *list, const fsm_end_id_t *ids, size_t count);

bool
build_retlist(struct ret_list *list, const struct ir *ir);

void
free_retlist(struct ret_list *list);

#endif

