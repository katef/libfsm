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

#define NO_END_DISTANCE ((unsigned)-1)

#define LOG_TRIM 0

struct edge {
	fsm_state_t from;
	fsm_state_t to;
};

static void
compact_shortest_end_distance(const struct fsm *fsm,
    unsigned *sed);

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
mark_states(struct fsm *fsm, enum fsm_trim_mode mode,
    unsigned *sed)
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
	fsm_state_t max_end;

	const size_t state_count = fsm->statecount;

	size_t *offsets = NULL;

	if (!fsm_getstart(fsm, &start)) {
		return 1;	/* nothing is reachable */
	}

	q = queue_new(fsm->opt->alloc, state_count);
	if (q == NULL) {
		goto cleanup;
	}

	if (LOG_TRIM > 0) {
		fprintf(stderr, "mark_states: mode %d\n", mode);
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
	if (LOG_TRIM > 0) {
		fprintf(stderr, "mark_states: pushing %d (start)\n", start);
	}
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

		if (LOG_TRIM > 0) {
			fprintf(stderr, "mark_states: popped %d\n", s_id);
		}

		assert(s_id < state_count);
		assert(fsm->states[s_id].visited);

		if (ends && fsm_isend(fsm, s_id)) {
			if (end_count == end_ceil) {
				if (!grow_ends(fsm->opt->alloc,
					&end_ceil, &ends)) {
					goto cleanup;
				}
			}
			if (LOG_TRIM > 0) {
				fprintf(stderr, "mark_states: ends[%zu] = %d\n",
				    end_count, s_id);
			}
			ends[end_count] = s_id;
			end_count++;
		}

		for (state_set_reset(fsm->states[s_id].epsilons, &state_iter);
		     state_set_next(&state_iter, &next); ) {
			assert(next < state_count);
			if (LOG_TRIM > 0) {
				fprintf(stderr, "mark_states: epsilon to %d, visited? %d\n",
				    next, fsm->states[next].visited);
			}

			if (!fsm->states[next].visited) {
				if (LOG_TRIM > 0) {
					fprintf(stderr, "mark_states: pushing %d (epsilon)\n", next);
				}
				if (!queue_push(q, next)) {
					goto cleanup;
				}
				fsm->states[next].visited = 1;
			}

			if (edges == NULL) {
				continue;
			}
			if (!save_edge(fsm->opt->alloc,
				&edge_count, &edge_ceil, &edges,
				s_id, next)) {
				goto cleanup;
			}
		}

		for (edge_set_reset(fsm->states[s_id].edges, &edge_iter);
		     edge_set_next(&edge_iter, &e); ) {
			next = e.state;
			if (LOG_TRIM > 0) {
				fprintf(stderr, "mark_states: edge: 0x%x to %d, visited? %d\n",
				    e.symbol, next, fsm->states[next].visited);
			}

			if (!fsm->states[next].visited) {
				if (LOG_TRIM > 0) {
					fprintf(stderr, "mark_states: pushing %d (labeled edge)\n", next);
				}
				if (!queue_push(q, next)) {
					goto cleanup;
				}
				fsm->states[next].visited = 1;
			}

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

	if (LOG_TRIM > 0) {
		fprintf(stderr, "mark_states: done tracking from start\n");
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

	max_end = 0;

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

			if (end_id > max_end) {
				max_end = end_id;
			}

			if (LOG_TRIM > 0) {
				fprintf(stderr, "mark_states: seeding with end %d, marking visited\n",
				    end_id);
			}

			if (!queue_push(q, end_id)) {
				goto cleanup;
			}
			fsm->states[end_id].visited = 1;

			/* end states have an end distance of 0 */
			if (sed != NULL) {
				sed[end_id] = 0;
			}
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
		const size_t offset_count = fsm_countstates(fsm);

		offsets = f_calloc(fsm->opt->alloc,
		    offset_count, sizeof(offsets[0]));
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

			if (LOG_TRIM > 1) {
				fprintf(stderr, "mark_states: offsets[%zu]: %zu\n",
				    i, offsets[i]);
			}
		}
		assert(offsets[max_to] == edge_count);
	}

	if (LOG_TRIM > 1) {
		size_t i;
		for (i = 0; i < edge_count; i++) {
		fprintf(stderr, "mark_states: edges[%zu]: %d -> %d\n",
		    i, edges[i].from, edges[i].to);
		}
	}

	/* Walk breadth-first from ends and mark reachable states. */
	while (queue_pop(q, &s_id)) {
		size_t base, limit, e_i;
		assert(fsm->states[s_id].visited);

		base = (s_id == 0 ? 0 : offsets[s_id - 1]);
		limit = offsets[s_id];

		if (LOG_TRIM > 0) {
			fprintf(stderr, "mark_states: popped %d, offsets [%zu, %zu]\n",
			    s_id, base, limit);
		}

		for (e_i = base; e_i < limit; e_i++) {
			const fsm_state_t from = edges[e_i].from;
			const unsigned end_distance = (sed == NULL
			    ? 0 : sed[s_id]);
			assert(from < state_count);

			if (LOG_TRIM > 0) {
				fprintf(stderr, "mark_states: edges[%zu]: from: %d, visited? %d\n",
				    e_i, from, fsm->states[from].visited);
			}

			/* Update the end distance -- one more step
			 * removed from the nearest end state. */
			if (sed != NULL) {
				if (sed[from] == NO_END_DISTANCE
				    || end_distance < sed[from]) {
					sed[from] = end_distance + 1;
				}
			}

			if (!fsm->states[from].visited) {
				fsm->states[from].visited = 1;
				if (LOG_TRIM > 0) {
					fprintf(stderr, "mark_states: pushing %d (reachable from ends)\n", from);
				}
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

	if (LOG_TRIM > 0) {
		fprintf(stderr, "save_edge: %d -> %d\n",
		    from, to);
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

	if (LOG_TRIM > 1) {
		size_t i;
		for (i = 0; i < fsm->statecount; i++) {
			fprintf(stderr, "sweep_states: state[%zu]: visited? %d\n",
			    i, fsm->states[i].visited);
		}
	}

	if (!fsm_compact_states(fsm, marks_filter_cb, fsm, &swept)) {
		return -1;
	}

	return (long)swept;
}

static void
integrity_check(const char *descr, const struct fsm *fsm)
{
	struct state_iter state_iter;
	fsm_state_t to;
	const size_t count = fsm->statecount;
	size_t s_id;
	struct edge_iter edge_iter;
	struct fsm_edge e;

#ifdef NDEBUG
	return;
#endif

	if (LOG_TRIM > 1) {
		fprintf(stderr, "integrity check: %s...\n", descr);
	}

	for (s_id = 0; s_id < count; s_id++) {
		for (state_set_reset(fsm->states[s_id].epsilons, &state_iter);
		     state_set_next(&state_iter, &to); ) {
			if (to >= count) {
				fprintf(stderr, "FAILURE (state_set): s_id %zu, to %u, count %zu\n", s_id, to, count);
				assert(to < count);
			}
		}

		for (edge_set_reset(fsm->states[s_id].edges, &edge_iter);
		     edge_set_next(&edge_iter, &e); ) {
			const fsm_state_t to = e.state;
			if (to >= count) {
				fprintf(stderr, "FAILURE (edge_set): s_id %zu, to %u, count %zu\n", s_id, to, count);
				assert(to < count);
			}
		}
	}

	if (LOG_TRIM > 1) {
		fprintf(stderr, "integrity check: %s...PASS\n", descr);
	}
}

long
fsm_trim(struct fsm *fsm, enum fsm_trim_mode mode,
	unsigned **shortest_end_distance)
{
	long ret;
	unsigned char *marks = NULL;
	unsigned *sed = NULL;

	fsm_state_t i;
	assert(fsm != NULL);

	if (fsm->statecount == 0) {
		return 1;
	}

	if (shortest_end_distance != NULL
		&& mode == FSM_TRIM_START_AND_END_REACHABLE) {
		size_t s_i;
		sed = f_malloc(fsm->opt->alloc,
		    fsm->statecount * sizeof(sed[0]));
		if (sed == NULL) {
			goto cleanup;
		}
		for (s_i = 0; s_i < fsm->statecount; s_i++) {
			sed[s_i] = NO_END_DISTANCE;
		}
	}

	if (!mark_states(fsm, mode, sed)) {
		goto cleanup;
	}

	if (sed != NULL) {
		compact_shortest_end_distance(fsm, sed);
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
	if (LOG_TRIM > 0) {
		fprintf(stderr, "sweep_states: returned %ld\n", ret);
	}

	/* Clear the marks on remaining states, since .visited is
	 * only meaningful within a single operation. */
	for (i = 0; i < fsm->statecount; i++) {
		fsm->states[i].visited = 0;
	}

	if (ret < 0) {
		if (sed != NULL) {
			f_free(fsm->opt->alloc, sed);
		}
		return ret;
	}

	if (sed != NULL) {
		assert(shortest_end_distance != NULL);
		*shortest_end_distance = sed;
	}

	integrity_check("post", fsm);

	return ret;

cleanup:
	if (marks != NULL) {
		f_free(fsm->opt->alloc, marks);
	}
	if (sed != NULL) {
		f_free(fsm->opt->alloc, sed);
	}
	return -1;
}

/* Since fsm is about to be compacted (several states will
 * be trimmed away), compact the corresponding end depths
 * so that sed[i] will correspond to fsm->states[i] after
 * trimming completes. */
static void
compact_shortest_end_distance(const struct fsm *fsm,
    unsigned *sed)
{
	size_t dst = 0, src;

	for (src = 0; src < fsm->statecount; src++) {
		if (fsm->states[src].visited) {
			if (dst < src) {
				sed[dst] = sed[src];
			}
			dst++;
		} else {
			assert(sed[src] == NO_END_DISTANCE);
		}
	}

	/* Anything after sed[dst] is now garbage. If dst is
	 * significantly less than fsm->statecount, we could
	 * realloc to shrink it here. */
}
