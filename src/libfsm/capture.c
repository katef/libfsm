/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>

#include "capture_internal.h"

int
fsm_capture_init(struct fsm *fsm)
{
	struct fsm_capture_info *ci = NULL;
	size_t i;

	ci = f_malloc(fsm->alloc, sizeof(*ci));
	if (ci == NULL) {
		goto cleanup;
	}

	ci->max_capture_id = 0;
	ci->bucket_count   = 0;
	ci->buckets_used   = 0;
	ci->buckets        = NULL;

	fsm->capture_info = ci;

	for (i = 0; i < fsm->statealloc; i++) {
		fsm->states[i].has_capture_actions = 0;
	}

	return 1;

cleanup:
	if (ci != NULL) {
		f_free(fsm->alloc, ci);
	}
	return 0;
}

void
fsm_capture_free(struct fsm *fsm)
{
	struct fsm_capture_info *ci = fsm->capture_info;
	if (ci == NULL) {
		return;
	}
	f_free(fsm->alloc, ci->buckets);
	f_free(fsm->alloc, ci);
	fsm->capture_info = NULL;
}

unsigned
fsm_countcaptures(const struct fsm *fsm)
{
	(void)fsm;
	if (fsm->capture_info == NULL) {
		return 0;
	}
	if (fsm->capture_info->buckets_used == 0) {
		return 0;
	}

	/* check actual */
#if EXPENSIVE_CHECKS
	{
		struct fsm_capture_info *ci = fsm->capture_info;
		size_t i;
		for (i = 0; i < ci->bucket_count; i++) {
			struct fsm_capture_action_bucket *b = &ci->buckets[i];
			if (b->state == CAPTURE_NO_STATE) { /* empty */
				continue;
			}
			assert(ci->max_capture_id >= b->action.id);
		}
	}
#endif

	return fsm->capture_info->max_capture_id + 1;
}

int
fsm_capture_has_captures(const struct fsm *fsm)
{
	return fsm->capture_info
	    ? fsm->capture_info->buckets_used > 0
	    : 0;
}

int
fsm_capture_has_capture_actions(const struct fsm *fsm, fsm_state_t state)
{
	assert(state < fsm->statecount);
	return fsm->states[state].has_capture_actions;
}

int
fsm_capture_set_path(struct fsm *fsm, unsigned capture_id,
    fsm_state_t start, fsm_state_t end)
{
	struct fsm_capture_info *ci;
	struct capture_set_path_env env;
	size_t seen_words;
	int res = 0;

	assert(fsm != NULL);
	assert(start < fsm->statecount);
	assert(end < fsm->statecount);

	ci = fsm->capture_info;
	assert(ci != NULL);

	/* captures should no longer be stored as paths -- instead, set
	 * the info on the states _here_, and convert it as necessary. */

#if LOG_CAPTURE > 0
	fprintf(stderr, "fsm_capture_set_path: capture %u: <%u, %u>\n",
	    capture_id, start, end);
#endif

	if (capture_id > FSM_CAPTURE_MAX) {
		return 0;	/* ID out of range */
	}

	if (!init_capture_action_htab(fsm, ci)) {
		return 0;
	}

	/* This will create a trail and do a depth-first search from the
	 * start state, marking every unique path to the end state. */
	env.fsm = fsm;
	env.capture_id = capture_id;
	env.start = start;
	env.end = end;

	env.trail_ceil = 0;
	env.trail = NULL;
	env.seen = NULL;

	env.trail = f_malloc(fsm->alloc,
	    DEF_TRAIL_CEIL * sizeof(env.trail[0]));
	if (env.trail == NULL) {
		goto cleanup;
	}
	env.trail_ceil = DEF_TRAIL_CEIL;

	seen_words = fsm->statecount/64 + 1;
	env.seen = f_malloc(fsm->alloc,
	    seen_words * sizeof(env.seen[0]));

	if (!mark_capture_path(&env)) {
		goto cleanup;
	}

	if (capture_id >= ci->max_capture_id) {
		ci->max_capture_id = capture_id;
	}

	res = 1;
	/* fall through */

cleanup:
	f_free(fsm->alloc, env.trail);
	f_free(fsm->alloc, env.seen);
	return res;
}

static int
init_capture_action_htab(struct fsm *fsm, struct fsm_capture_info *ci)
{
	size_t count, i;
	assert(fsm != NULL);
	assert(ci != NULL);

	if (ci->bucket_count > 0) {
		assert(ci->buckets != NULL);
		return 1;	/* done */
	}

	assert(ci->buckets == NULL);
	assert(ci->buckets_used == 0);

	count = DEF_CAPTURE_ACTION_BUCKET_COUNT;
	ci->buckets = f_malloc(fsm->alloc,
	    count * sizeof(ci->buckets[0]));
	if (ci->buckets == NULL) {
		return 0;
	}

	/* Init buckets to CAPTURE_NO_STATE -> empty. */
	for (i = 0; i < count; i++) {
		ci->buckets[i].state = CAPTURE_NO_STATE;
	}

	ci->bucket_count = count;
	return 1;
}

static int
mark_capture_path(struct capture_set_path_env *env)
{
	const size_t seen_words = env->fsm->statecount/64 + 1;

#if LOG_CAPTURE > 0
	fprintf(stderr, "mark_capture_path: path [id %u, %u - %u]\n",
	    env->capture_id, env->start, env->end);
#endif

	if (env->start == env->end) {
		struct fsm_capture_action action;
		action.type = CAPTURE_ACTION_COMMIT_ZERO_STEP;
		action.id = env->capture_id;
		action.to = CAPTURE_NO_STATE;
		if (!add_capture_action(env->fsm, env->fsm->capture_info,
			env->start, &action)) {
			return 0;
		}
		return 1;
	}

	memset(env->seen, 0x00,
	    seen_words * sizeof(env->seen[0]));

	/* initialize to starting node */
	env->trail_i = 1;
	env->trail[0].state = env->start;
	env->trail[0].step = TRAIL_STEP_START;
	env->trail[0].has_self_edge = 0;

	while (env->trail_i > 0) {
		const enum trail_step step = env->trail[env->trail_i - 1].step;
#if LOG_CAPTURE > 0
		fprintf(stderr, "mark_capture_path: trail %u/%u, cur %u, step %d\n",
		    env->trail_i, env->trail_ceil,
		    env->trail[env->trail_i - 1].state,
		    step);
#endif

		switch (step) {
		case TRAIL_STEP_START:
			if (!step_trail_start(env)) {
				return 0;
			}
			break;
		case TRAIL_STEP_ITER_EDGES:
			if (!step_trail_iter_edges(env)) {
				return 0;
			}
			break;
		case TRAIL_STEP_ITER_EPSILONS:
			if (!step_trail_iter_epsilons(env)) {
				return 0;
			}
			break;
		case TRAIL_STEP_DONE:
			if (!step_trail_done(env)) {
				return 0;
			}
			break;
		default:
			assert(!"match fail");
		}
	}

	return 1;
}

static int
cmp_action(const struct fsm_capture_action *a,
    const struct fsm_capture_action *b) {
	/* could use memcmp here, provided padding is always zeroed. */
	return a->id < b->id ? -1
	    : a->id > b->id ? 1
	    : a->type < b->type ? -1
	    : a->type > b->type ? 1
	    : a->to < b->to ? -1
	    : a->to > b->to ? 1
	    : 0;
}

int
fsm_capture_add_action(struct fsm *fsm,
    fsm_state_t state, enum capture_action_type type,
    unsigned id, fsm_state_t to)
{
	struct fsm_capture_action action;
	assert(fsm->capture_info != NULL);

	action.type = type;
	action.id = id;
	action.to = to;
	return add_capture_action(fsm, fsm->capture_info,
	    state, &action);
}

static int
add_capture_action(struct fsm *fsm, struct fsm_capture_info *ci,
    fsm_state_t state, const struct fsm_capture_action *action)
{
	uint64_t h;
	size_t b_i, mask;

	assert(state < fsm->statecount);
	assert(action->to == CAPTURE_NO_STATE || action->to < fsm->statecount);

#if LOG_CAPTURE > 0
	fprintf(stderr, "add_capture_action: state %u, type %s, ID %u, TO %d\n",
	    state, fsm_capture_action_type_name[action->type],
	    action->id, action->to);
#endif

	if (ci->bucket_count == 0) {
		if (!init_capture_action_htab(fsm, ci)) {
			return 0;
		}
	} else if (ci->buckets_used >= ci->bucket_count/2) { /* grow */
		if (!grow_capture_action_buckets(fsm->alloc, ci)) {
			return 0;
		}
	}

	h = hash_id(state);
	mask = ci->bucket_count - 1;

	for (b_i = 0; b_i < ci->bucket_count; b_i++) {
		struct fsm_capture_action_bucket *b = &ci->buckets[(h + b_i) & mask];
		if (b->state == CAPTURE_NO_STATE) { /* empty */
			b->state = state;
			memcpy(&b->action, action, sizeof(*action));
			ci->buckets_used++;
			fsm->states[state].has_capture_actions = 1;
			if (action->id > ci->max_capture_id) {
				ci->max_capture_id = action->id;
			}
			return 1;
		} else if (b->state == state &&
		    0 == cmp_action(action, &b->action)) {
			/* already present, ignore duplicate */
			assert(fsm->states[state].has_capture_actions);
			assert(ci->max_capture_id >= action->id);
			return 1;
		} else {
			continue; /* skip past collision */
		}
	}

	assert(!"unreachable");
	return 0;
}

static int
grow_capture_action_buckets(const struct fsm_alloc *alloc,
    struct fsm_capture_info *ci)
{
	const size_t ncount = 2 * ci->bucket_count;
	struct fsm_capture_action_bucket *nbuckets;
	size_t nused = 0;
	size_t i;

	assert(ncount != 0);
	nbuckets = f_malloc(alloc, ncount * sizeof(nbuckets[0]));
	if (nbuckets == NULL) {
		return 0;
	}

	for (i = 0; i < ncount; i++) {
		nbuckets[i].state = CAPTURE_NO_STATE;
	}

	for (i = 0; i < ci->bucket_count; i++) {
		const struct fsm_capture_action_bucket *src_b = &ci->buckets[i];
		uint64_t h;
		const size_t mask = ncount - 1;
		size_t b_i;

		if (src_b->state == CAPTURE_NO_STATE) {
			continue;
		}

		h = hash_id(src_b->state);
		for (b_i = 0; b_i < ncount; b_i++) {
			struct fsm_capture_action_bucket *dst_b;
			dst_b = &nbuckets[(h + b_i) & mask];
			if (dst_b->state == CAPTURE_NO_STATE) {
				memcpy(dst_b, src_b, sizeof(*src_b));
				nused++;
				break;
			} else {
				continue;
			}
		}
	}

	assert(nused == ci->buckets_used);
	f_free(alloc, ci->buckets);
	ci->buckets = nbuckets;
	ci->bucket_count = ncount;
	return 1;
}

static int
grow_trail(struct capture_set_path_env *env)
{
	struct trail_cell *ntrail;
	unsigned nceil;
	assert(env != NULL);

	nceil = 2 * env->trail_ceil;
	assert(nceil > env->trail_ceil);

	ntrail = f_realloc(env->fsm->alloc, env->trail,
	    nceil * sizeof(env->trail[0]));
	if (ntrail == NULL) {
		return 0;
	}

	env->trail = ntrail;
	env->trail_ceil = nceil;
	return 1;
}

static int
step_trail_start(struct capture_set_path_env *env)
{
	struct trail_cell *tc = &env->trail[env->trail_i - 1];
	const fsm_state_t cur = tc->state;
	size_t i;
	struct edge_set *edge_set = NULL;

	/* check if node is endpoint, if so mark trail,
	 * then pop trail and continue */
	if (cur == env->end) {
		struct fsm_capture_action action;
#if LOG_CAPTURE > 0
		fprintf(stderr, " -- GOT END at %u\n", cur);
#endif
		action.id = env->capture_id;

		for (i = 0; i < env->trail_i; i++) {
			fsm_state_t state = env->trail[i].state;
#if LOG_CAPTURE > 0
			fprintf(stderr, " -- %lu: %d\n",
			    i, state);
#endif

			/* Special case: if this is marked as having
			 * a self-edge on the path, then also add an
			 * extend for that. */
			if (env->trail[i].has_self_edge) {
				struct fsm_capture_action self_action;
				self_action.type = CAPTURE_ACTION_EXTEND;
				self_action.id = env->capture_id;
				self_action.to = state;

				if (!add_capture_action(env->fsm,
					env->fsm->capture_info,
					state, &self_action)) {
					return 0;
				}
			}


			if (i == 0) {
				action.type = CAPTURE_ACTION_START;
			} else {
				action.type = (i < env->trail_i - 1
				    ? CAPTURE_ACTION_EXTEND
				    : CAPTURE_ACTION_COMMIT);
			}

			if (i < env->trail_i - 1) {
				action.to = env->trail[i + 1].state;
			} else {
				action.to = CAPTURE_NO_STATE;
			}

			if (!add_capture_action(env->fsm,
				env->fsm->capture_info,
				state, &action)) {
				return 0;
			}
		}

		tc->step = TRAIL_STEP_DONE;
		return 1;
	}

#if LOG_CAPTURE > 0
	fprintf(stderr, " -- resetting edge iterator\n");
#endif
	edge_set = env->fsm->states[cur].edges;

	MARK_SEEN(env, cur);
#if LOG_CAPTURE > 0
	fprintf(stderr, " -- marking %u as seen\n", cur);
#endif

	edge_set_reset(edge_set, &tc->iter);
	tc->step = TRAIL_STEP_ITER_EDGES;
	return 1;
}

static int
step_trail_iter_edges(struct capture_set_path_env *env)
{
	struct trail_cell *tc = &env->trail[env->trail_i - 1];
	struct trail_cell *next_tc = NULL;

	struct fsm_edge e;

	if (!edge_set_next(&tc->iter, &e)) {
#if LOG_CAPTURE > 0
		fprintf(stderr, " -- ITER_EDGE_NEXT: DONE %u\n", tc->state);
#endif
		tc->step = TRAIL_STEP_ITER_EPSILONS;
		return 1;
	}

#if LOG_CAPTURE > 0
	fprintf(stderr, " -- ITER_EDGE_NEXT: %u -- NEXT %u\n",
	    tc->state, e.state);
#endif

	if (tc->state == e.state) {
#if LOG_CAPTURE > 0
		fprintf(stderr, "    -- special case, self-edge\n");
#endif
		/* Mark this state as having a self-edge, then continue
		 * the iterator. An EXTEND action will be added for the
		 * self-edge later, if necessary. */
		tc->has_self_edge = 1;
		return 1;
	} else if (CHECK_SEEN(env, e.state)) {
#if LOG_CAPTURE > 0
		fprintf(stderr, "    -- seen, skipping\n");
#endif
		return 1;	/* continue */
	}

	if (env->trail_i == env->trail_ceil) {
		if (!grow_trail(env)) {
			return 0;
		}
	}

#if LOG_CAPTURE > 0
	fprintf(stderr, " -- marking %u as seen\n", e.state);
#endif
	MARK_SEEN(env, e.state);

#if LOG_CAPTURE > 0
	fprintf(stderr, "    -- not seen (%u), exploring\n", e.state);
#endif
	env->trail_i++;
	next_tc = &env->trail[env->trail_i - 1];
	next_tc->state = e.state;
	next_tc->step = TRAIL_STEP_START;
	next_tc->has_self_edge = 0;
	return 1;
}

static int
step_trail_iter_epsilons(struct capture_set_path_env *env)
{
	struct trail_cell *tc = &env->trail[env->trail_i - 1];

	/* skipping this for now */

#if LOG_CAPTURE > 0
	fprintf(stderr, " -- ITER_EPSILONS: %u\n", tc->state);
#endif

	tc->step = TRAIL_STEP_DONE;
	return 1;
}

static int
step_trail_done(struct capture_set_path_env *env)
{
	struct trail_cell *tc;

	/* 0-step paths already handled outside loop */
	assert(env->trail_i > 0);

	tc = &env->trail[env->trail_i - 1];
#if LOG_CAPTURE > 0
	fprintf(stderr, " -- DONE: %u\n", tc->state);
#endif
	CLEAR_SEEN(env, tc->state);

	env->trail_i--;
	return 1;
}

void
fsm_capture_rebase_capture_id(struct fsm *fsm, unsigned base)
{
	size_t i;
	struct fsm_capture_info *ci = fsm->capture_info;
	assert(ci != NULL);

	for (i = 0; i < ci->bucket_count; i++) {
		struct fsm_capture_action_bucket *b = &ci->buckets[i];
		if (b->state == CAPTURE_NO_STATE) {
			continue;
		}

		b->action.id += base;
		if (b->action.id > ci->max_capture_id) {
			ci->max_capture_id = b->action.id;
		}
	}
}

void
fsm_capture_rebase_capture_action_states(struct fsm *fsm, fsm_state_t base)
{
	size_t i;
	struct fsm_capture_info *ci = fsm->capture_info;
	assert(ci != NULL);

	for (i = 0; i < ci->bucket_count; i++) {
		struct fsm_capture_action_bucket *b = &ci->buckets[i];
		if (b->state == CAPTURE_NO_STATE) {
			continue;
		}

		b->state += base;
		if (b->action.to != CAPTURE_NO_STATE) {
			b->action.to += base;
		}
	}
}

struct fsm_capture *
fsm_capture_alloc(const struct fsm *fsm)
{
	(void)fsm;
	assert(!"todo");
	return NULL;
}

void
fsm_capture_update_captures(const struct fsm *fsm,
    fsm_state_t cur_state, fsm_state_t next_state, size_t offset,
    struct fsm_capture *captures)
{
	const struct fsm_capture_info *ci;
	uint64_t h;
	size_t b_i, mask;

	assert(cur_state < fsm->statecount);
	assert(fsm->states[cur_state].has_capture_actions);

	ci = fsm->capture_info;
	assert(ci != NULL);

	h = hash_id(cur_state);
	mask = ci->bucket_count - 1;

#if LOG_CAPTURE > 0
	fprintf(stderr, "-- updating captures at state %u, to %d, offset %lu\n",
	    cur_state, next_state, offset);
#endif

	for (b_i = 0; b_i < ci->bucket_count; b_i++) {
		const size_t b_id = (h + b_i) & mask;
		struct fsm_capture_action_bucket *b = &ci->buckets[b_id];
		unsigned capture_id;

#if LOG_CAPTURE > 3
		fprintf(stderr, "   -- update_captures: bucket %lu, state %d\n", b_id, b->state);
#endif


		if (b->state == CAPTURE_NO_STATE) {
#if LOG_CAPTURE > 3
			fprintf(stderr, "  -- no more actions for this state\n");
#endif
			break;	/* no more for this state */
		} else if (b->state != cur_state) {
			continue; /* skip collision */
		}

		assert(b->state == cur_state);
		capture_id = b->action.id;

		switch (b->action.type) {
		case CAPTURE_ACTION_START:
#if LOG_CAPTURE > 0
			fprintf(stderr, "START [%u, %u]\n",
			    b->action.id, b->action.to);
#endif
			if (next_state == b->action.to && captures[capture_id].pos[0] == FSM_CAPTURE_NO_POS) {
				captures[capture_id].pos[0] = offset;
#if LOG_CAPTURE > 0
				fprintf(stderr, " -- set capture[%u].[0] to %lu\n", b->action.id, offset);
#endif
			} else {
				/* filtered, ignore */
			}
			break;
		case CAPTURE_ACTION_EXTEND:
#if LOG_CAPTURE > 0
			fprintf(stderr, "EXTEND [%u, %u]\n",
			    b->action.id, b->action.to);
#endif
			if (captures[capture_id].pos[0] != FSM_CAPTURE_NO_POS
			    && (0 == (captures[capture_id].pos[1] & COMMITTED_CAPTURE_FLAG))) {
				if (next_state == b->action.to) {
					captures[capture_id].pos[1] = offset;
#if LOG_CAPTURE > 0
				fprintf(stderr, " -- set capture[%u].[1] to %lu\n", b->action.id, offset);
#endif
				} else {
					/* filtered, ignore */
				}
			}
			break;
		case CAPTURE_ACTION_COMMIT_ZERO_STEP:
#if LOG_CAPTURE > 0
			fprintf(stderr, "COMMIT_ZERO_STEP [%u]\n",
			    b->action.id);
#endif

			if (captures[capture_id].pos[0] == FSM_CAPTURE_NO_POS) {
				captures[capture_id].pos[0] = offset;
				captures[capture_id].pos[1] = offset | COMMITTED_CAPTURE_FLAG;
			} else { /* extend */
				captures[capture_id].pos[1] = offset | COMMITTED_CAPTURE_FLAG;
			}

#if LOG_CAPTURE > 0
			fprintf(stderr, " -- set capture[%u].[0] and [1] to %lu (with COMMIT flag)\n", b->action.id, offset);
#endif
			break;
		case CAPTURE_ACTION_COMMIT:
#if LOG_CAPTURE > 0
			fprintf(stderr, "COMMIT [%u]\n",
			    b->action.id);
#endif
			captures[capture_id].pos[1] = offset | COMMITTED_CAPTURE_FLAG;
#if LOG_CAPTURE > 0
			fprintf(stderr, " -- set capture[%u].[1] to %lu (with COMMIT flag)\n", b->action.id, offset);
#endif
			break;
		default:
			assert(!"matchfail");
		}
	}
}

void
fsm_capture_finalize_captures(const struct fsm *fsm,
    size_t capture_count, struct fsm_capture *captures)
{
	size_t i;

	/* If either pos[] is FSM_CAPTURE_NO_POS or the
	 * COMMITTED_CAPTURE_FLAG isn't set on pos[1], then the capture
	 * wasn't finalized; clear it. Otherwise, clear that bit so the
	 * pos[1] offset is meaningful. */

	/* FIXME: this should also take the end state(s) associated
	 * with a capture into account, when that information is available;
	 * otherwise there will be false positives for zero-width captures
	 * where the paths have a common prefix. */
	(void)fsm;

	for (i = 0; i < capture_count; i++) {
#if LOG_CAPTURE > 1
		fprintf(stderr, "finalize[%lu]: pos[0]: %ld, pos[1]: %ld\n",
		    i, captures[i].pos[0], captures[i].pos[1]);
#endif

		if (captures[i].pos[0] == FSM_CAPTURE_NO_POS
		    || captures[i].pos[1] == FSM_CAPTURE_NO_POS
		    || (0 == (captures[i].pos[1] & COMMITTED_CAPTURE_FLAG))) {
			captures[i].pos[0] = FSM_CAPTURE_NO_POS;
			captures[i].pos[1] = FSM_CAPTURE_NO_POS;
#if LOG_CAPTURE > 1
			fprintf(stderr, "finalize: discard %lu\n", i);
#endif
		} else if (captures[i].pos[1] & COMMITTED_CAPTURE_FLAG) {
			captures[i].pos[1] &=~ COMMITTED_CAPTURE_FLAG;
		}
	}
}

void
fsm_capture_action_iter(const struct fsm *fsm,
    fsm_capture_action_iter_cb *cb, void *opaque)
{
	size_t i;
	struct fsm_capture_info *ci = fsm->capture_info;
	assert(ci != NULL);

	for (i = 0; i < ci->bucket_count; i++) {
		struct fsm_capture_action_bucket *b = &ci->buckets[i];
		if (b->state == CAPTURE_NO_STATE) {
			continue;
		}

		if (!cb(b->state, b->action.type,
			b->action.id, b->action.to, opaque)) {
			break;
		}
	}
}

const char *fsm_capture_action_type_name[] = {
	"START", "EXTEND",
	"COMMIT_ZERO_STEP", "COMMIT"
};

static int
dump_iter_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque)
{
	FILE *f = opaque;
	fprintf(f, " - state %u, %s [capture_id: %u, to: %d]\n",
	    state, fsm_capture_action_type_name[type], capture_id, to);
	return 1;
}

/* Dump capture metadata about an FSM. */
void
fsm_capture_dump(FILE *f, const char *tag, const struct fsm *fsm)
{
	struct fsm_capture_info *ci;

	assert(fsm != NULL);
	ci = fsm->capture_info;
	if (ci == NULL || ci->bucket_count == 0) {
		fprintf(f, "==== %s -- no captures\n", tag);
		return;
	}

	fprintf(f, "==== %s -- capture action hash table (%u buckets)\n",
	    tag, ci->bucket_count);
	fsm_capture_action_iter(fsm, dump_iter_cb, f);
}
