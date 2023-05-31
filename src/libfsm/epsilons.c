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
#define LOG_RM_EPSILONS_CAPTURES 0
#define LOG_COPYING 0
#define LOG_RESULT 0

/* #define DEF_CARRY_ENDIDS_COUNT 2 */
/* #define DEF_CARRY_CAPTUREIDS_COUNT 2 */

#if LOG_RESULT
#include <fsm/print.h>
#endif

#define DEF_END_METADATA_ENDIDS_CEIL 4
#define DEF_END_METADATA_CAPTUREIDS_CEIL 4
#define DEF_END_METADATA_PROGRAMIDS_CEIL 4
struct carry_end_metadata_env {
	struct fsm *fsm;
	const struct fsm_alloc *alloc;

	struct {
		size_t ceil;
		fsm_end_id_t *ids;
	} end;
	struct {
		int ok;
		size_t count;
		size_t ceil;
		unsigned *ids;
	} capture;
	struct {
		int ok;
		size_t count;
		size_t ceil;
		uint32_t *ids;
	} program;
};

static int
carry_end_metadata(struct carry_end_metadata_env *env,
    fsm_state_t end_state, fsm_state_t dst_state);

int
fsm_remove_epsilons(struct fsm *nfa)
{
#if LOG_RESULT
	fprintf(stderr, "==== before\n");
	fsm_print_fsm(stderr, nfa);
	fsm_capture_dump(stderr, "#### before_remove_epsilons", nfa);
	fprintf(stderr, "====\n");
#endif

	const size_t state_count = fsm_countstates(nfa);
	int res = 0;
	struct state_set **eclosures = NULL;
	fsm_state_t s, start_id;
	const struct fsm_alloc *alloc = nfa->opt->alloc;

	INIT_TIMERS();

	struct carry_end_metadata_env em_env = { 0 };
	em_env.fsm = nfa;
	em_env.alloc = alloc;

	assert(nfa != NULL);

	if (!fsm_getstart(nfa, &start_id)) {
		goto cleanup;
	}

	/* TODO: This could successfully exit early if none of the
	 * states have epsilon edges. */

	TIME(&pre);
	eclosures = fsm_epsilon_closure(nfa);
	TIME(&post);
	DIFF_MSEC("epsilon_closure", pre, post, NULL);

	if (eclosures == NULL) {
		goto cleanup;
	}

#if DUMP_EPSILON_CLOSURES
	{
		struct state_iter kt;
		fsm_state_t es;
		fprintf(stderr, "# ==== epsilon_closures\n");
		for (s = 0; s < nfa->statecount; s++) {
			if (eclosures[s] == NULL) { continue; }
			fprintf(stderr, " -- %u:", s);
			for (state_set_reset(eclosures[s], &kt); state_set_next(&kt, &es); ) {
				fprintf(stderr, " %u", es);
			}
			fprintf(stderr, "\n");
		}
	}
#endif

	for (s = 0; s < state_count; s++) {
		struct state_iter si;
		fsm_state_t es_id;

		struct edge_group_iter egi;
		struct edge_group_iter_info info;

		/* Process the epsilon closure. */
		state_set_reset(eclosures[s], &si);
		while (state_set_next(&si, &es_id)) {
			struct fsm_state *es = &nfa->states[es_id];

			/* The current NFA state is an end state if any
			 * of its associated epsilon-clousure states are
			 * end states.
			 *
			 * Similarly, any end state metadata on states
			 * in its epsilon-closure is copied to it. */
			if (fsm_isend(nfa, es_id)) {
#if LOG_COPYING
				fprintf(stderr, "remove_epsilons: setting end on %d (due to %d)\n", s, es_id);
#endif
				fsm_setend(nfa, s, 1);

				if (!carry_end_metadata(&em_env, es_id, s)) {
					goto cleanup;
				}
			}

			/* For every state in this state's transitive
			 * epsilon closure, add all of their sets of
			 * labeled edges. */
			edge_set_group_iter_reset(es->edges, EDGE_GROUP_ITER_ALL, &egi);
			while (edge_set_group_iter_next(&egi, &info)) {
#if LOG_COPYING
				fprintf(stderr, "%s: bulk-copying edges leading to state %d onto state %d (from state %d)\n",
				    __func__, info.to, s, es_id);
#endif
				if (!edge_set_add_bulk(&nfa->states[s].edges, nfa->opt->alloc,
					info.symbols, info.to)) {
					goto cleanup;
				}
			}
		}
	}

	/* Remove the epsilon-edge state sets from everything.
	 * This can make states unreachable. */
	for (s = 0; s < state_count; s++) {
		struct fsm_state *state = &nfa->states[s];
		state_set_free(state->epsilons);
		state->epsilons = NULL;
	}

	fsm_capture_integrity_check(nfa);

#if LOG_RESULT
	fsm_print_fsm(stderr, nfa);
	fsm_capture_dump(stderr, "#### post_remove_epsilons", nfa);
#endif

	res = 1;
cleanup:
	if (eclosures != NULL) {
		fsm_closure_free(eclosures, state_count);
	}
	if (em_env.end.ids != NULL) {
		f_free(alloc, em_env.end.ids);
	}
	if (em_env.program.ids != NULL) {
		f_free(alloc, em_env.program.ids);
	}
	if (em_env.capture.ids != NULL) {
		f_free(alloc, em_env.capture.ids);
	}

	return res;
}

static int
collect_captureid_cb(fsm_state_t state, unsigned id, void *opaque)
{
	struct carry_end_metadata_env *env = opaque;
	(void)state;

	if (env->capture.count == env->capture.ceil) {
		const size_t nceil = (env->capture.ceil == 0)
		    ? DEF_END_METADATA_CAPTUREIDS_CEIL
		    : 2 * env->capture.ceil;
		unsigned *nids;
		assert(nceil > env->capture.ceil);
		nids = f_realloc(env->alloc, env->capture.ids,
		    nceil * sizeof(env->capture.ids[0]));
		if (nids == NULL) {
			env->capture.ok = 0;
			return 0;
 		}
		env->capture.ceil = nceil;
		env->capture.ids = nids;
 	}

	env->capture.ids[env->capture.count] = id;
	env->capture.count++;
	return 1;
}

static int
collect_progid_cb(fsm_state_t state, unsigned id, void *opaque)
{
	struct carry_end_metadata_env *env = opaque;
	uint32_t prog_id = (uint32_t)id;
 	(void)state;

	if (env->program.count == env->program.ceil) {
		const size_t nceil = (env->program.ceil == 0)
		    ? DEF_END_METADATA_PROGRAMIDS_CEIL
		    : 2 * env->program.ceil;
		unsigned *nids;
		assert(nceil > env->program.ceil);
		nids = f_realloc(env->alloc, env->program.ids,
		    nceil * sizeof(env->program.ids[0]));
 		if (nids == NULL) {
			env->program.ok = 0;
			return 0;
 		}
		env->program.ceil = nceil;
		env->program.ids = nids;
 	}

	env->program.ids[env->program.count] = prog_id;
	env->program.count++;
	return 1;
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

	env->capture.ok = 1;
	env->capture.count = 0;
	fsm_capture_iter_active_for_end_state(env->fsm, end_state,
	    collect_captureid_cb, env);
	if (!env->capture.ok) {
		return 0;
	}
	for (i = 0; i < env->capture.count; i++) {
		if (!fsm_capture_set_active_for_end(env->fsm,
			env->capture.ids[i], dst_state)) {
			return 0;
		}
	}

	env->program.count = 0;
	fsm_capture_iter_program_ids_for_end_state(env->fsm, end_state,
	    collect_progid_cb, env);
	for (i = 0; i < env->program.count; i++) {
		if (!fsm_capture_associate_program_with_end_state(env->fsm,
			env->program.ids[i], dst_state)) {
			return 0;
		}
	}

	return 1;
}
