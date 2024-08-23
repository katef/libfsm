/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fsm/alloc.h>
#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/queue.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

static int
epsilon_closure_single(const struct fsm *fsm, struct state_set **closures, fsm_state_t s)
{
	struct state_iter it;
	struct queue *q;
	fsm_state_t p;

	assert(fsm != NULL);
	assert(closures != NULL);
	assert(s < fsm->statecount);

	q = queue_new(fsm->alloc, fsm->statecount);
	if (q == NULL) {
		return 1;
	}

	fsm->states[s].visited = 1;

	p = s;

	goto start;

	for (;;) {
		struct state_iter state_iter;
		fsm_state_t es;

		/* XXX: don't need a queue, a set would suffice */
		if (!queue_pop(q, &p)) {
			break;
		}

start:

		assert(p < fsm->statecount);

		/*
		 * If the closure for this state is already done (i.e. non-NULL),
		 * we can use it as part of the closure we're computing.
		 * For single-threaded traversal, this situation only happens on edges
		 * to lower-numbered states.
		 *
		 * Because an epsilon closure always includes its own state, we know
		 * done sets will never be NULL.
		 *
		 * Note we will never populate (or create the set for) any state other
		 * than closures[s] here; we only ever read from closures[p], not write.
		 *
		 * TODO: for a multithreaded implementation, this needs to be atomic.
		 * TODO: use partially-constructed (i.e. not readable) sets, and finalize later
		 */
		if (p != s && closures[p] != NULL) {
			if (!state_set_copy(&closures[s], fsm->alloc, closures[p])) {
				goto error;
			}
		}

		/*
		 * ... otherwise we have to traverse s ourselves, but we can't store the
		 * result of that in closures[s], because of cycles in the graph which
		 * include both states (that is, s and the origional state for which we
		 * are computing a closure).
		 *
		 * The exception to this would be when s does not reach the origional
		 * state, but this is expensive to compute without undertaking the same
		 * traversal we're doing in the first place. So we live with potentially
		 * multiple threads repeating some of the same work here.
		 */

		if (!state_set_add(&closures[s], fsm->alloc, p)) {
			goto error;
		}

		for (state_set_reset(fsm->states[p].epsilons, &state_iter); state_set_next(&state_iter, &es); ) {
			if (fsm->states[es].visited) {
				continue;
			}

			if (!queue_push(q, es)) {
				goto error;
			}

			fsm->states[es].visited = 1;
		}
	}

	/* TODO: when we allow partially-constructed sets, then sort closures[s] here and finalize the set */

	/*
	 * Clear .visited because any particular state may belong to multiple closures,
	 * and so we may need to visit this state again.
	 */
	for (state_set_reset(closures[s], &it); state_set_next(&it, &p); ) {
		fsm->states[p].visited = 0;
	}

	queue_free(q);

	return 1;

error:

	queue_free(q);

	return 0;
}

struct state_set **
epsilon_closure(struct fsm *fsm)
{
	struct state_set **closures;
	fsm_state_t s;

	assert(fsm != NULL);
	assert(fsm->statecount > 0);

	closures = f_malloc(fsm->alloc, fsm->statecount * sizeof *closures);
	if (closures == NULL) {
		return NULL;
	}

	for (s = 0; s < fsm->statecount; s++) {
		fsm->states[s].visited = 0;

		closures[s] = NULL;

		/*
		 * In the simplest case of having no epsilon transitions, the
		 * epsilon closure contains only itself. We populate that case
		 * here because it doesn't need any traversal and so we can
		 * avoid the bookkeeping overhead needed for general case.
		 */
		if (fsm->states[s].epsilons == NULL) {
			if (!state_set_add(&closures[s], fsm->alloc, s)) {
				goto error;
			}
		}
	}

	/*
	 * TODO: Here we would arbitrarily partition the state array and iterate
	 * over states in parallel. Because of the contention on "done" states,
	 * it might be better to visit states randomly rather than iterating
	 * in order, because nearby states tend to share overlap in their closures.
	 *
	 * We might prefer to have threads re-do the same work rather than blocking
	 * on a mutex, and use an atomic read for reading .visited and similar.
	 */
	for (s = 0; s < fsm->statecount; s++) {
		if (closures[s] != NULL) {
			continue;
		}

		epsilon_closure_single(fsm, closures, s);
	}

	return closures;

error:

	for (s = 0; s < fsm->statecount; s++) {
		state_set_free(closures[s]);
	}

	f_free(fsm->alloc, closures);

	return NULL;
}

void
closure_free(struct fsm *fsm, struct state_set **closures, size_t n)
{
	fsm_state_t s;

	for (s = 0; s < n; s++) {
		state_set_free(closures[s]);
	}

	f_free(fsm->alloc, closures);
}

