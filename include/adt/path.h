/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PATH_H
#define PATH_H

struct fsm_state;

struct path {
	struct fsm_state *state;
	struct path *next;
	int type; /* XXX: fsm_edge_type is private */
};

struct path *
path_push(struct path **head, struct fsm_state *state, int type);

void
path_free(struct path *path);

#endif

