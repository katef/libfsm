/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include "determinise_internal.h"

int
fsm_determinise(struct fsm *nfa)
{
	int res = 0;
	struct mappingstack *stack = NULL;

	struct interned_state_set_pool *issp = NULL;
	struct interned_state_set *empty = NULL;
	struct map map = { NULL, 0, 0, NULL };
	struct mapping *curr = NULL;
	size_t dfacount = 0;

	assert(nfa != NULL);
	map.alloc = nfa->opt->alloc;

	/*
	 * This NFA->DFA implementation is for Glushkov NFA only; it keeps things
	 * a little simpler by avoiding epsilon closures here, and also a little
	 * faster where we can start with a Glushkov NFA in the first place.
	 */
	if (fsm_has(nfa, fsm_hasepsilons)) {
		if (!fsm_glushkovise(nfa)) {
			return 0;
		}
	}

#if LOG_DETERMINISE_CAPTURES
	fprintf(stderr, "# post_glushkovise\n");
	fsm_print_fsm(stderr, nfa);
	fsm_capture_dump(stderr, "#### post_glushkovise", nfa);
#endif

	issp = interned_state_set_pool_alloc(nfa->opt->alloc);
	if (issp == NULL) {
		return 0;
	}

	empty = interned_state_set_empty(issp);
	assert(empty != NULL);	/* cannot fail */

	{
		fsm_state_t start;
		struct interned_state_set *start_set;

		/*
		 * The starting condition is the epsilon closure of a set of states
		 * containing just the start state.
		 *
		 * You can think of this as having reached the FSM's start state by
		 * previously consuming some imaginary prefix symbol (giving the set
		 * containing just the start state), and then this epsilon closure
		 * is equivalent to the usual case of taking the epsilon closure after
		 * each symbol closure in the main loop.
		 *
		 * (We take a copy of this set for sake of memory ownership only,
		 * for freeing the epsilon closures later).
		 */

		if (!fsm_getstart(nfa, &start)) {
			interned_state_set_pool_free(issp);
			return 1;
		}

#if LOG_DETERMINISE_CAPTURES
		fprintf(stderr, "#### Adding mapping for start state %u -> 0\n", start);
#endif
		start_set = interned_state_set_add(issp, empty, start);
		if (start_set == NULL) {
			goto cleanup;
		}

		if (!map_add(&map, dfacount, start_set, &curr)) {
			goto cleanup;
		}
		dfacount++;
	}

	/*
	 * Our "todo" list. It needn't be a stack; we treat it as an unordered
	 * set where we can consume arbitrary items in turn.
	 */
	stack = stack_init(nfa->opt->alloc);
	if (stack == NULL) {
		goto cleanup;
	}

	do {
		struct interned_state_set *sclosures[FSM_SIGMA_COUNT] = { NULL };
		int i;
		assert(curr != NULL);

		/*
		 * The closure of a set is equivalent to the union of closures of
		 * each item. Here we iteratively build up closures[] in-situ
		 * to avoid needing to create a state set to store the union.
		 */
		{
			struct state_iter it;
			fsm_state_t s;
			struct state_set *ss = interned_state_set_retain(curr->iss);

			for (state_set_reset(ss, &it); state_set_next(&it, &s); ) {
 				if (!interned_symbol_closure_without_epsilons(nfa, s, issp, sclosures)) {
					goto cleanup;
				}
			}
			interned_state_set_release(curr->iss);
		}

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			struct mapping *m;

			if (sclosures[i] == NULL) {
				continue;
			}

#if LOG_DETERMINISE_CLOSURES
			fprintf(stderr, "fsm_determinise: cur (dfa %zu) label '%c': %p:",
			    curr->dfastate, (char)i, (void *)sclosures[i]);
			{
				struct state_iter it;
				fsm_state_t s;
				struct state_set *ss = interned_state_set_retain(sclosures[i]);

				for (state_set_reset(ss, &it); state_set_next(&it, &s); ) {
					fprintf(stderr, " %u", s);
				}
				fprintf(stderr, "\n");
				interned_state_set_release(sclosures[i]);
			}
#endif

			/*
			 * The set of NFA states sclosures[i] represents a single DFA state.
			 * We use the mappings as a de-duplication mechanism, keyed by the
			 * set of NFA states.
			 */

			/* If this interned_state_set isn't present, then save it as a new mapping. */
			if (map_find(&map, sclosures[i], &m)) {
				/* we already have this closure interned */
				assert(m->dfastate < dfacount);
			} else {
				/* not found -- add a new one */
				if (!map_add(&map, dfacount, sclosures[i], &m)) {
					goto cleanup;
				}
				dfacount++;
				if (!stack_push(stack, m)) {
					goto cleanup;
				}
			}

			if (!edge_set_add(&curr->edges, nfa->opt->alloc, i, m->dfastate)) {
				goto cleanup;
			}
		}

		/* All elements in sclosures[] are interned, so they will be freed later. */
	} while ((curr = stack_pop(stack)));

	{
		struct map_iter it;
		struct mapping *m;
		struct fsm *dfa;

		dfa = fsm_new(nfa->opt);
		if (dfa == NULL) {
			goto cleanup;
		}

#if DUMP_MAPPING
		{
			fprintf(stderr, "#### fsm_determinise: mapping\n");

			/* build reverse mappings table: for every NFA state X, if X is part
			 * of the new DFA state Y, then add Y to a list for X */
			for (m = map_first(&map, &it); m != NULL; m = map_next(&it)) {
				struct state_iter si;
				fsm_state_t state;
				struct state_set *ss = interned_state_set_retain(m->iss);
				fprintf(stderr, "%zu:", m->dfastate);

				for (state_set_reset(ss, &si); state_set_next(&si, &state); ) {
					fprintf(stderr, " %u", state);
				}
				fprintf(stderr, "\n");
				interned_state_set_release(m->iss);
			}
			fprintf(stderr, "#### fsm_determinise: end of mapping\n");
		}
#endif
		if (!fsm_addstate_bulk(dfa, dfacount)) {
			goto cleanup;
		}

		assert(dfa->statecount == dfacount);

		/*
		 * We know state 0 is the start state, because its mapping
		 * was created first.
		 */
		fsm_setstart(dfa, 0);

		for (m = map_first(&map, &it); m != NULL; m = map_next(&it)) {
			struct state_set *ss;
			assert(m->dfastate < dfa->statecount);
			assert(dfa->states[m->dfastate].edges == NULL);

			dfa->states[m->dfastate].edges = m->edges;

			/*
			 * The current DFA state is an end state if any of its associated NFA
			 * states are end states.
			 */

			ss = interned_state_set_retain(m->iss);
			if (!state_set_has(nfa, ss, fsm_isend)) {
				interned_state_set_release(m->iss);
				continue;
			}

			fsm_setend(dfa, m->dfastate, 1);

			/*
			 * Carry through end IDs, if present. This isn't anything to do
			 * with the DFA conversion; it's meaningful only to the caller.
			 *
			 * The closure may contain non-end states, but at least one state is
			 * known to have been an end state.
			 */
			if (!fsm_endid_carry(nfa, ss, dfa, m->dfastate)) {
				goto cleanup;
			}
			interned_state_set_release(m->iss);
		}

		if (!remap_capture_actions(&map, dfa, nfa)) {
			goto cleanup;
		}

		fsm_move(nfa, dfa);
	}

	res = 1;

cleanup:
	map_free(&map);
	stack_free(stack);
	interned_state_set_pool_free(issp);
	return res;
}

/* Add DFA_state to the list for NFA_state. */
static int
add_reverse_mapping(const struct fsm_alloc *alloc,
    struct reverse_mapping *reverse_mappings,
    fsm_state_t dfastate, fsm_state_t nfa_state)
{
	struct reverse_mapping *rm = &reverse_mappings[nfa_state];
	if (rm->count == rm->ceil) {
		const unsigned nceil = (rm->ceil ? 2*rm->ceil : 2);
		fsm_state_t *nlist = f_realloc(alloc,
		    rm->list, nceil * sizeof(rm->list));
		if (nlist == NULL) {
			return 0;
		}
		rm->list = nlist;
		rm->ceil = nceil;
	}

	rm->list[rm->count] = dfastate;
	rm->count++;
	return 1;
}

static int
det_copy_capture_actions_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque)
{
	struct reverse_mapping *rm_s;
	size_t s_i, t_i;
	struct det_copy_capture_actions_env *env = opaque;
	assert(env->tag == 'D');

#if LOG_DETERMINISE_CAPTURES
	fprintf(stderr, "det_copy_capture_actions_cb: state %u, type %s, ID %u, TO %d\n",
	    state, fsm_capture_action_type_name[type],
	    capture_id, to);
#endif

	rm_s = &env->reverse_mappings[state];

	for (s_i = 0; s_i < rm_s->count; s_i++) {
		const fsm_state_t s = rm_s->list[s_i];

		if (to == CAPTURE_NO_STATE) {
			if (!fsm_capture_add_action(env->dst,
				s, type, capture_id, CAPTURE_NO_STATE)) {
				env->ok = 0;
				return 0;
			}
		} else {
			struct reverse_mapping *rm_t = &env->reverse_mappings[to];
			for (t_i = 0; t_i < rm_t->count; t_i++) {
				const fsm_state_t t = rm_t->list[t_i];

				if (!fsm_capture_add_action(env->dst,
					s, type, capture_id, t)) {
					env->ok = 0;
					return 0;
				}
			}
		}
	}

	return 1;
}

static int
det_copy_capture_actions(struct reverse_mapping *reverse_mappings,
    struct fsm *dst, struct fsm *src)
{
	struct det_copy_capture_actions_env env = { 'D', NULL, NULL, 1 };
	env.dst = dst;
	env.reverse_mappings = reverse_mappings;

	fsm_capture_action_iter(src, det_copy_capture_actions_cb, &env);
	return env.ok;
}

static int
interned_symbol_closure_without_epsilons(const struct fsm *fsm, fsm_state_t s,
	struct interned_state_set_pool *issp,
	struct interned_state_set *sclosures[static FSM_SIGMA_COUNT])
{
	struct edge_iter jt;
	struct fsm_edge e;

	assert(fsm != NULL);
	assert(sclosures != NULL);

	if (fsm->states[s].edges == NULL) {
		return 1;
	}

	/*
	 * TODO: it's common for many symbols to have transitions to the same state
	 * (the worst case being an "any" transition). It'd be nice to find a way
	 * to avoid repeating that work by de-duplicating on the destination.
	 */

	for (edge_set_reset(fsm->states[s].edges, &jt); edge_set_next(&jt, &e); ) {
		struct interned_state_set *iss = sclosures[e.symbol];
		struct interned_state_set *updated;
		if (iss == NULL) {
			iss = interned_state_set_empty(issp);
		}

		updated = interned_state_set_add(issp, iss, e.state);
		if (updated == NULL) {
			return 0;
		}
		sclosures[e.symbol] = updated;
	}

	return 1;
}

static unsigned long
hash_iss(const struct interned_state_set *iss)
{
	/* Just hashing the address directly is fine here -- since they're
	 * interned, they're identified by pointer equality. */
	return PHI32 * (uintptr_t)iss;
}

static struct mapping *
map_first(struct map *map, struct map_iter *iter)
{
	iter->m = map;
	iter->i = 0;
	return map_next(iter);
}

static struct mapping *
map_next(struct map_iter *iter)
{
	const size_t limit = iter->m->count;
	while (iter->i < limit) {
		struct mapping **m = &iter->m->buckets[iter->i];
		iter->i++;
		if (*m == NULL) {
			continue;
		}
		return *m;
	}

	return NULL;
}

static int
map_add(struct map *map,
	fsm_state_t dfastate, struct interned_state_set *iss,
	struct mapping **new_mapping)
{
	size_t i;
	unsigned long h, mask;

	if (map->buckets == NULL) {
		assert(map->count == 0);
		assert(map->used == 0);

		map->buckets = f_calloc(map->alloc,
		    DEF_MAP_BUCKET_CEIL, sizeof(map->buckets[0]));
		if (map->buckets == NULL) {
			return 0;
		}
		map->count = DEF_MAP_BUCKET_CEIL;
	}

	if (map->used == map->count/2) {
		if (!grow_map(map)) {
			return 0;
		}
	}

	h = hash_iss(iss);
	mask = map->count - 1;

	for (i = 0; i < map->count; i++) {
		struct mapping **b = &map->buckets[(h + i) & mask];
		if (*b == NULL) {
			struct mapping *m = f_malloc(map->alloc, sizeof(*m));
			if (m == NULL) {
				return 0;
			}
			m->iss = iss;
			m->dfastate = dfastate;
			m->edges = NULL;
			*b = m;

			if (new_mapping != NULL) {
				*new_mapping = m;
			}
			map->used++;
			return 1;
		} else {
			continue; /* hash collision */
		}
	}

	assert(!"unreachable");
	return 0;
}

static int
grow_map(struct map *map)
{
	size_t nceil = 2 * map->count;
	struct mapping **nbuckets;
	size_t o_i, n_i;
	const size_t nmask = nceil - 1;

	assert(nceil > 0);

	nbuckets = f_calloc(map->alloc,
	    nceil, sizeof(nbuckets[0]));
	if (nbuckets == NULL) {
		return 0;
	}

	for (o_i = 0; o_i < map->count; o_i++) {
		unsigned long h;
		struct mapping *ob = map->buckets[o_i];
		if (ob == NULL) {
			continue; /* empty */
		}
		h = hash_iss(ob->iss);
		for (n_i = 0; n_i < nmask; n_i++) {
			struct mapping **nb = &nbuckets[(h + n_i) & nmask];
			if (*nb == NULL) {
				*nb = ob;
				break;
			}
		}
		assert(n_i < nmask); /* found a spot */
	}

	f_free(map->alloc, map->buckets);
	map->count = nceil;
	map->buckets = nbuckets;
	return 1;
}

static int
map_find(const struct map *map, struct interned_state_set *iss,
	struct mapping **mapping)
{
	size_t i;
	unsigned long h, mask;

	if (map->buckets == NULL) {
		return 0;
	}

	h = hash_iss(iss);
	mask = map->count - 1;

	for (i = 0; i < map->count; i++) {
		struct mapping *b = map->buckets[(h + i) & mask];
		if (b == NULL) {
			return 0; /* not found */
		} else if (b->iss == iss) {
			*mapping = b;
			return 1;
		} else {
			continue; /* hash collision */
		}
	}

	assert(!"unreachable");
	return 0;
}

static void
map_free(struct map *map)
{
	size_t i;
	if (map->buckets == NULL) {
		return;
	}

	for (i = 0; i < map->count; i++) {
		struct mapping *b = map->buckets[i];
		if (b == NULL) {
			continue;
		}
		f_free(map->alloc, b);
	}

	f_free(map->alloc, map->buckets);
}

static struct mappingstack *
stack_init(const struct fsm_alloc *alloc)
{
	struct mapping **s;
	struct mappingstack *res = f_malloc(alloc, sizeof(*res));
	if (res == NULL) {
		return NULL;
	}

	s = f_malloc(alloc, MAPPINGSTACK_DEF_CEIL * sizeof(s[0]));
	if (s == NULL) {
		f_free(alloc, res);
		return NULL;
	}

	res->alloc = alloc;
	res->ceil = MAPPINGSTACK_DEF_CEIL;
	res->depth = 0;
	res->s = s;

	return res;
}

static void
stack_free(struct mappingstack *stack)
{
	if (stack == NULL) {
		return;
	}
	f_free(stack->alloc, stack->s);
	f_free(stack->alloc, stack);
}

static int
stack_push(struct mappingstack *stack, struct mapping *item)
{
	if (stack->depth + 1 == stack->ceil) {
		size_t nceil = 2 * stack->ceil;
		struct mapping **ns = f_realloc(stack->alloc,
		    stack->s, nceil * sizeof(ns[0]));
		if (ns == NULL) {
			return 0;
		}
		stack->ceil = nceil;
		stack->s = ns;
	}

	stack->s[stack->depth] = item;
	stack->depth++;
	return 1;
}

static struct mapping *
stack_pop(struct mappingstack *stack)
{
	assert(stack != NULL);
	if (stack->depth == 0) {
		return 0;
	}

	stack->depth--;
	struct mapping *item = stack->s[stack->depth];
	return item;
}

static int
remap_capture_actions(struct map *map,
    struct fsm *dst_dfa, struct fsm *src_nfa)
{
	struct map_iter it;
	struct state_iter si;
	struct mapping *m;
	struct reverse_mapping *reverse_mappings;
	fsm_state_t state;
	const size_t capture_count = fsm_countcaptures(src_nfa);
	size_t i, j;
	int res = 0;

	if (capture_count == 0) {
		return 1;
	}

	/* This is not 1 to 1 -- if state X is now represented by multiple
	 * states Y in the DFA, and state X has action(s) when transitioning
	 * to state Z, this needs to be added on every Y, for every state
	 * representing Z in the DFA.
	 *
	 * We could probably filter this somehow, at the very least by
	 * checking reachability from every X, but the actual path
	 * handling later will also check reachability. */
	reverse_mappings = f_calloc(dst_dfa->opt->alloc, src_nfa->statecount, sizeof(reverse_mappings[0]));
	if (reverse_mappings == NULL) {
		return 0;
	}

	/* build reverse mappings table: for every NFA state X, if X is part
	 * of the new DFA state Y, then add Y to a list for X */
	for (m = map_first(map, &it); m != NULL; m = map_next(&it)) {
		struct state_set *ss;
		assert(m->dfastate < dst_dfa->statecount);
		ss = interned_state_set_retain(m->iss);

		for (state_set_reset(ss, &si); state_set_next(&si, &state); ) {
			if (!add_reverse_mapping(dst_dfa->opt->alloc,
				reverse_mappings,
				m->dfastate, state)) {
				goto cleanup;
			}
		}
		interned_state_set_release(m->iss);
	}

#if LOG_DETERMINISE_CAPTURES
	fprintf(stderr, "#### reverse mapping for %zu states\n", src_nfa->statecount);
	for (i = 0; i < src_nfa->statecount; i++) {
		struct reverse_mapping *rm = &reverse_mappings[i];
		fprintf(stderr, "%lu:", i);
		for (j = 0; j < rm->count; j++) {
			fprintf(stderr, " %u", rm->list[j]);
		}
		fprintf(stderr, "\n");
	}
#else
	(void)j;
#endif

	if (!det_copy_capture_actions(reverse_mappings, dst_dfa, src_nfa)) {
		goto cleanup;
	}

	res = 1;
cleanup:
	for (i = 0; i < src_nfa->statecount; i++) {
		if (reverse_mappings[i].list != NULL) {
			f_free(dst_dfa->opt->alloc, reverse_mappings[i].list);
		}
	}
	f_free(dst_dfa->opt->alloc, reverse_mappings);

	return res;
}
