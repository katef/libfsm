/*
 * Copyright 2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_RETLIST_H
#define FSM_INTERNAL_RETLIST_H

struct ret {
	size_t count;
	const fsm_end_id_t *ids;
};

struct ret_list {
	size_t count;
	struct ret *a;
};

int
cmp_ret_by_endid(const void *pa, const void *pb);

struct ret *
find_ret(const struct ret_list *list, const struct dfavm_op_ir *op,
	int (*cmp)(const void *pa, const void *pb));

bool
build_retlist(struct ret_list *list, const struct dfavm_op_ir *a);

void
free_retlist(struct ret_list *list);

#endif

