/* $Id$ */

#ifndef TRANS_H
#define TRANS_H

/* TODO: maybe these can be folded into edge.c as static functions */

int
trans_equal(const struct trans_list *a, const struct trans_list *b);

struct trans_list *
trans_add(struct fsm *fsm, enum fsm_edge_type type);

#endif

