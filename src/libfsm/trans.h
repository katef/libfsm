/* $Id$ */

#ifndef TRANS_H
#define TRANS_H

/* TODO: maybe these can be folded into edge.c as static functions */

int
trans_equal(enum fsm_edge_type type, union trans_value *a, union trans_value *b);

struct trans_list *
trans_add(struct fsm *fsm, enum fsm_edge_type type, union trans_value *u);

int
trans_copyvalue(enum fsm_edge_type type, const union trans_value *from,
	union trans_value *to);

#endif

