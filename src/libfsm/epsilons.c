/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/alloc.h>
#include <adt/queue.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"

#define DUMP_EPSILON_CLOSURES 0
#define LOG_RM_EPSILONS_CAPTURES 1
#define LOG_COPYING 0
#define LOG_RESULT 0

#define DEF_PENDING_CAPTURE_ACTIONS_CEIL 2
#define DEF_CARRY_ENDIDS_COUNT 2
#define DEF_CARRY_CAPTUREIDS_COUNT 2

#if LOG_RESULT
#include <fsm/print.h>
#endif

#define DEF_END_METADATA_ENDIDS_CEIL 4
#define DEF_END_METADATA_CAPTUREIDS_CEIL 4
struct carry_end_metadata_env {
	struct fsm *fsm;
	const struct fsm_alloc *alloc;

	struct {
		size_t ceil;
		fsm_end_id_t *ids;
	} end;
};

static int
carry_end_metadata(struct carry_end_metadata_env *env,
    fsm_state_t end_state, fsm_state_t dst_state);

int
fsm_remove_epsilons(struct fsm *fsm)
{
#if LOG_RESULT
	fprintf(stderr, "==== before\n");
	fsm_print_fsm(stderr, nfa);
	fprintf(stderr, "====\n");
#endif

	const size_t state_count = fsm_countstates(fsm);
	int res = 0;
	struct state_set **eclosures = NULL;
	fsm_state_t s_id, start_id;
	const struct fsm_alloc *alloc = fsm->opt->alloc;
	struct queue *q = NULL;

	struct carry_end_metadata_env em_env = { 0 };
	em_env.fsm = fsm;
	em_env.alloc = alloc;

	if (!fsm_getstart(fsm, &start_id)) {
		goto cleanup;
	}

	/* TODO: This could successfully exit early if none of the
	 * states have epsilon edges. */

	eclosures = fsm_epsilon_closure(fsm);
	if (eclosures == NULL) {
		goto cleanup;
	}

#if DUMP_EPSILON_CLOSURES
	{
		struct state_iter kt;
		fsm_state_t es;
		fprintf(stderr, "# ==== epsilon_closures\n");
		for (s_id = 0; s_id < fsm->statecount; s_id++) {
			if (eclosures[s_id] == NULL) { continue; }
			fprintf(stderr, " -- %u:", s_id);
			for (state_set_reset(eclosures[s_id], &kt); state_set_next(&kt, &es); ) {
				fprintf(stderr, " %u", es);
			}
			fprintf(stderr, "\n");
		}
	}
#endif

	q = queue_new(fsm->opt->alloc, state_count);
	if (q == NULL) {
		goto cleanup;
	}

	if (!queue_push(q, start_id)) {
		goto cleanup;
	}
	{
		struct fsm_state *s = &fsm->states[start_id];
		s->visited = 1;
	}

	while (queue_pop(q, &s_id)) {
		struct state_iter si;
		fsm_state_t es_id;
		struct fsm_state *s = &fsm->states[s_id];

		struct edge_group_iter egi;
		struct edge_group_iter_info info;

		/* Enqueue all states reachable via labeled edges */
		edge_set_group_iter_reset(s->edges, EDGE_GROUP_ITER_ALL, &egi);
		while (edge_set_group_iter_next(&egi, &info)) {
			struct fsm_state *es = &fsm->states[info.to];

			if (!es->visited) {
				if (!queue_push(q, info.to)) {
					goto cleanup;
				}
				es->visited = 1;
			}
		}

		/* Process the epsilon closure. */
		state_set_reset(eclosures[s_id], &si);
		while (state_set_next(&si, &es_id)) {
			struct fsm_state *es = &fsm->states[es_id];

			if (!es->visited) {
				if (!queue_push(q, es_id)) {
					goto cleanup;
				}
				es->visited = 1;
			}

			/* The current NFA state is an end state if any
			 * of its associated epsilon-clousure states are
			 * end states.
			 *
			 * Similarly, any end state metadata on states
			 * in its epsilon-closure is copied to it.
			 *
			 * Capture actions are copied in a later pass. */
			if (fsm_isend(fsm, es_id)) {
#if LOG_COPYING
				fprintf(stderr, "remove_epsilons: setting end on %d (due to %d)\n", s_id, es_id);
#endif
				fsm_setend(fsm, s_id, 1);

				/* Carry through end metadata, if present.
				 * This isn't anything to do with the NFA conversion;
				 * it's meaningful only to the caller. */
				if (!carry_end_metadata(&em_env, es_id, s_id)) {
					goto cleanup;
				}
			}

			/* For every state in this state's transitive
			 * epsilon closure, add all of their sets of
			 * labeled edges. */
			edge_set_group_iter_reset(es->edges, EDGE_GROUP_ITER_ALL, &egi);
			while (edge_set_group_iter_next(&egi, &info)) {
#if LOG_COPYING
				fprintf(stderr, "remove_epsilons: bulk-copying edges leading to state %d onto state %d (from state %d)\n",
				    info.to, s_id, es_id);
#endif
				if (!edge_set_add_bulk(&s->edges, alloc,
					info.symbols, info.to)) {
					goto cleanup;
				}
			}
		}
	}

#if LOG_RESULT
	fprintf(stderr, "=== %s: about to update capture actions\n", __func__);
	fsm_print_fsm(stderr, fsm);
#endif

#if 0	/* Not integrated yet */
	/* Copy over the capture actions from the epsilon closure. */
	if (!fsm_capture_update_capture_actions_when_removing_epsilons(fsm,
		(const struct state_set **)eclosures)) {
		goto cleanup;
	}
#endif

	/* Remove the epsilon-edge state sets from everything.
	 * This can make states unreachable. */
	for (s_id = 0; s_id < state_count; s_id++) {
		struct fsm_state *s = &fsm->states[s_id];
		state_set_free(s->epsilons);
		s->epsilons = NULL;
		s->visited = 0;
	}

#if LOG_RESULT
	fsm_print_fsm(stderr, fsm);
#endif

	res = 1;
cleanup:
	if (em_env.end.ids != NULL) {
		f_free(alloc, em_env.end.ids);
	}

	if (eclosures != NULL) {
		fsm_closure_free(eclosures, state_count);
	}
	if (q != NULL) {
		queue_free(q);
	}

	return res;
}

/* Because we're modifying the FSM in place, we can't iterate and add
 * new entries -- it could lead to the underlying hash table resizing.
 * Instead, collect, then add in a second pass. */
static int
carry_end_metadata(struct carry_end_metadata_env *env,
    fsm_state_t end_state, fsm_state_t dst_state)
{
	size_t i;
	const size_t id_count = fsm_getendidcount(env->fsm, end_state);
	if (id_count > 0) { /* copy end IDs */
		enum fsm_getendids_res id_res;
		size_t written;
		if (id_count > env->end.ceil) { /* grow buffer */
			const size_t nceil = (env->end.ceil == 0)
			    ? DEF_END_METADATA_ENDIDS_CEIL
			    : 2*env->end.ceil;
			assert(nceil > 0);
			env->end.ids = f_realloc(env->alloc,
			    env->end.ids, nceil * sizeof(env->end.ids[0]));
			if (env->end.ids == NULL) {
				return 0;
			}
		}

		id_res = fsm_getendids(env->fsm, end_state,
		    id_count, env->end.ids, &written);
		assert(id_res != FSM_GETENDIDS_ERROR_INSUFFICIENT_SPACE);

		for (i = 0; i < id_count; i++) {
#if LOG_COPYING
			fprintf(stderr, "carry_end_metadata: setting end ID %u on %d (due to %d)\n",
			    env->end.ids[i], dst_state, end_state);
#endif
			if (!fsm_setendid_state(env->fsm, dst_state, env->end.ids[i])) {
				return 0;
			}
		}
	}

	return 1;
}
