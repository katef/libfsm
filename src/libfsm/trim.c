/*
 * Copyright 2019 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/queue.h>

#include "internal.h"

#define DEF_EDGES_CEIL 8
#define DEF_ENDS_CEIL 8

struct edge {
	fsm_state_t from;
	fsm_state_t to;
};

static int
grow_ends(const struct fsm_alloc *alloc, size_t *ceil, fsm_state_t **ends);

static int
save_edge(const struct fsm_alloc *alloc,
    size_t *count, size_t *ceil, struct edge **edges,
    fsm_state_t from, fsm_state_t to);

static int
cmp_edges_by_to(const void *pa, const void *pb)
{
	const struct edge *a = (const struct edge *)pa;
	const struct edge *b = (const struct edge *)pb;

	return a->to < b->to ? -1
	    : a->to > b->to ? 1
	    : a->from < b->from ? -1
	    : a->from > b->from ? 1
	    : 0;
}

static int
mark_states(struct fsm *fsm, enum fsm_trim_mode mode)
{
	/* Use a queue to walk breath-first over all states reachable
	 * from the start state. Note all end states. Collect all the
	 * edges, then sort them by the note they lead to, to convert it
	 * to a reverse edge index. Then, enqueue all the end states,
	 * and again use the queue to walk the graph breadth-first, but
	 * this time iterating bottom-up from the end states, and mark
	 * all reachable states (on fsm_state.visited).
	 *
	 * This marks all states which are both reachable from the start
	 * state and have a path to an end state. */
	int res = 0;
	fsm_state_t start, s_id;
	struct queue *q = NULL;

	struct edge *edges = NULL;
	size_t edge_count = 0, edge_ceil = DEF_EDGES_CEIL;

	fsm_state_t *ends = NULL;
	size_t end_count = 0, end_ceil = DEF_ENDS_CEIL;
	const size_t state_count = fsm->statecount;

	size_t *offsets = NULL;

	if (!fsm_getstart(fsm, &start)) {
		return 1;	/* nothing is reachable */
	}

	q = queue_new(fsm->opt->alloc, state_count);
	if (q == NULL) {
		goto cleanup;
	}

	if (mode == FSM_TRIM_START_AND_END_REACHABLE) {
		edges = f_malloc(fsm->opt->alloc,
		    edge_ceil * sizeof(edges[0]));
		if (edges == NULL) {
			goto cleanup;
		}

		ends = f_malloc(fsm->opt->alloc,
		    end_ceil * sizeof(ends[0]));
		if (ends == NULL) {
			goto cleanup;
		}
	}

	fsm->states[start].visited = 1;
	if (!queue_push(q, start)) {
		goto cleanup;
	}
	assert(start < state_count);

	/* Breadth-first walk, mark states reachable from start and (if
	 * checking whether states can reach end states later) also
	 * collect edges & end states. */
	while (queue_pop(q, &s_id)) {
		fsm_state_t next;
		struct fsm_edge e;
		struct edge_iter edge_iter;
		struct state_iter state_iter;

		assert(s_id < state_count);
		assert(fsm->states[s_id].visited);

		if (ends && fsm_isend(fsm, s_id)) {
			if (end_count == end_ceil) {
				if (!grow_ends(fsm->opt->alloc,
					&end_ceil, &ends)) {
					goto cleanup;
				}
			}
			ends[end_count] = s_id;
			end_count++;
		}

		for (state_set_reset(fsm->states[s_id].epsilons, &state_iter);
		     state_set_next(&state_iter, &next); ) {
			assert(next < state_count);
			if (!fsm->states[next].visited) {
				if (!queue_push(q, next)) {
					goto cleanup;
				}
				fsm->states[next].visited = 1;

				if (edges == NULL) {
					continue;
				}
				if (!save_edge(fsm->opt->alloc,
					&edge_count, &edge_ceil, &edges,
					s_id, next)) {
					goto cleanup;
				}
			}
		}

		for (edge_set_reset(fsm->states[s_id].edges, &edge_iter);
		     edge_set_next(&edge_iter, &e); ) {
			next = e.state;
			if (!fsm->states[next].visited) {
				if (!queue_push(q, next)) {
					goto cleanup;
				}
				fsm->states[next].visited = 1;

				if (edges == NULL) {
					continue;
				}
				if (!save_edge(fsm->opt->alloc,
					&edge_count, &edge_ceil, &edges,
					s_id, next)) {
					goto cleanup;
				}
			}
		}
	}

	/* Only tracking reachability from start, so we're done. */
	if (mode == FSM_TRIM_START_REACHABLE) {
		queue_free(q);
		return 1;
	}

	/* Clear mark for second pass. */
	for (s_id = 0; s_id < state_count; s_id++) {
		fsm->states[s_id].visited = 0;
	}

	/* Sort edges by state they lead to, inverting the index. */
	qsort(edges, edge_count, sizeof(edges[0]), cmp_edges_by_to);

	/* Reuse the existing queue for a second breadth-first walk.
	 * This assumes the queue can be reused once empty; if the
	 * implementation changes, it may need to reset the queue or
	 * construct a new one.
	 *
	 * Initialize the queue with the end states. */
	{
		size_t e_i;
		for (e_i = 0; e_i < end_count; e_i++) {
			const fsm_state_t end_id = ends[e_i];
			assert(end_id < state_count);
			if (!queue_push(q, end_id)) {
				goto cleanup;
			}
			fsm->states[end_id].visited = 1;
		}

		/* The ends are no longer needed. */
		f_free(fsm->opt->alloc, ends);
		ends = NULL;
	}

	if (edge_count == 0) {	/* done */
		res = 1;
		goto cleanup;
	}

	/* Build the offset table into the edge index, so we
	 * can map from s_id to the consecutive edges leading
	 * to it with random access.
	 *
	 * offsets[s_id] contains the offset of the first edge after
	 * s_id, so that offsets[s_id - 1] can be used as the starting
	 * offset, or 0 when s_id is 0. Since states may not appear in
	 * the table, any case where offsets[i] == 0 is set to
	 * offsets[i - 1], to represent zero entries. */
	{
		size_t i;
		const fsm_state_t max_to = edges[edge_count - 1].to;
		offsets = f_calloc(fsm->opt->alloc,
		    (max_to + 1), sizeof(offsets[0]));
		if (offsets == NULL) {
			goto cleanup;
		}

		for (i = 0; i < edge_count; i++) {
			const fsm_state_t to = edges[i].to;
			offsets[to] = i + 1;
		}

		for (i = 0; i <= max_to; i++) { /* fill in gaps */
			if (i > 0 && offsets[i] == 0) {
				offsets[i] = offsets[i - 1];
			}
		}
		assert(offsets[max_to] == edge_count);
	}


	/* Walk breadth-first from ends and mark reachable states. */
	while (queue_pop(q, &s_id)) {
		size_t base, limit, e_i;
		assert(fsm->states[s_id].visited);

		base = (s_id == 0 ? 0 : offsets[s_id - 1]);
		limit = offsets[s_id];
		for (e_i = base; e_i < limit; e_i++) {
			const fsm_state_t from = edges[e_i].from;
			assert(from < state_count);
			if (!fsm->states[from].visited) {
				fsm->states[from].visited = 1;
				if (!queue_push(q, from)) {
					goto cleanup;
				}
			}
		}
	}


	/* All start- and end-reachable states are now marked. */
	res = 1;

cleanup:
	if (edges != NULL) { f_free(fsm->opt->alloc, edges); }
	if (ends != NULL) { f_free(fsm->opt->alloc, ends); }
	if (offsets != NULL) { f_free(fsm->opt->alloc, offsets); }
	if (q != NULL) { queue_free(q); }

	return res;
}

static int
grow_ends(const struct fsm_alloc *alloc, size_t *ceil, fsm_state_t **ends)
{
	const size_t nceil = 2 * (*ceil);
	fsm_state_t *nends = f_realloc(alloc,
	    *ends, nceil * sizeof((*ends)[0]));
	if (nends == NULL) {
		return 0;
	}
	*ceil = nceil;
	*ends = nends;
	return 1;
}

static int
save_edge(const struct fsm_alloc *alloc,
    size_t *count, size_t *ceil, struct edge **edges,
    fsm_state_t from, fsm_state_t to)
{
	struct edge *e;
	if (*count == *ceil) {
		const size_t nceil = 2 * (*ceil);
		struct edge *nedges = f_realloc(alloc,
		    *edges, nceil * sizeof((*edges)[0]));
		if (nedges == NULL) {
			return 0;
		}
		*ceil = nceil;
		*edges = nedges;
	}

	e = &(*edges)[*count];
	e->from = from;
	e->to = to;
	(*count)++;
	return 1;
}

static int
marks_filter_cb(fsm_state_t id, void *opaque)
{
	const struct fsm *fsm = opaque;
	assert(id < fsm->statecount);
	return fsm->states[id].visited;
}

long
sweep_states(struct fsm *fsm)
{
	size_t swept;

	assert(fsm != NULL);

	if (!fsm_compact_states(fsm, marks_filter_cb, fsm, &swept)) {
		return -1;
	}

	return (long)swept;
}

long
fsm_trim(struct fsm *fsm, enum fsm_trim_mode mode)
{
	long ret;
	unsigned char *marks = NULL;
	fsm_state_t i;
	assert(fsm != NULL);

	if (!mark_states(fsm, mode)) {
		goto cleanup;
	}

	{
		/* - XXX -
		 * this is a hack to make sure that we leave at least the start state.
		 * otherwise a bunch of tests break...
		 */
		fsm_state_t start;
		if (fsm_getstart(fsm, &start)) {
			fsm->states[start].visited = 1;
		}
	}

	/*
	 * Remove all states which are unreachable from the start state
	 * or have no path to an end state. These are a trailing suffix
	 * which will never accept.
	 *
	 * sweep_states returns a negative value on error, otherwise it returns
	 * the number of states swept.
	 */
	ret = sweep_states(fsm);

	/* Clear the marks on remaining states, since .visited is
	 * only meaningful within a single operation. */
	for (i = 0; i < fsm->statecount; i++) {
		fsm->states[i].visited = 0;
	}

	if (ret < 0) {
		return ret;
	}

	return ret;

cleanup:
	if (marks != NULL) {
		f_free(fsm->opt->alloc, marks);
	}
	return -1;
}
