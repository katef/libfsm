/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include "determinise_internal.h"

static void
dump_labels(FILE *f, const uint64_t labels[4])
{
	size_t i;
	for (i = 0; i < 256; i++) {
		if (u64bitset_get(labels, i)) {
			fprintf(f, "%c", (char)(isprint(i) ? i : '.'));
		}
	}
}

enum fsm_determinise_with_config_res
fsm_determinise_with_config(struct fsm *nfa,
	const struct fsm_determinise_config *config)
{
	enum fsm_determinise_with_config_res res = FSM_DETERMINISE_WITH_CONFIG_ERRNO;
	struct mappingstack *stack = NULL;

	struct interned_state_set_pool *issp = NULL;
	struct map map = { NULL, 0, 0, NULL };
	struct mapping *curr = NULL;
	size_t dfacount = 0;
	const size_t state_limit = config == NULL
	    ? 0
	    : config->state_limit;

	struct analyze_closures_env ac_env = { 0 };

	assert(nfa != NULL);
	map.alloc = nfa->alloc;

	/*
	 * This NFA->DFA implementation is for epsilon-free NFA only. This keeps
	 * things a little simpler by avoiding epsilon closures, and also a little
	 * faster where we can start with an epsilon-free NFA in the first place.
	 */
	if (fsm_has(nfa, fsm_hasepsilons)) {
		if (!fsm_remove_epsilons(nfa)) {
			return FSM_DETERMINISE_WITH_CONFIG_ERRNO;
		}
	}

#if LOG_DETERMINISE_CAPTURES || LOG_INPUT
	fprintf(stderr, "# post_remove_epsilons, pre_determinise\n");
	fsm_dump(stderr, nfa);
	fsm_capture_dump(stderr, "#### post_remove_epsilons", nfa);
#endif

	issp = interned_state_set_pool_alloc(nfa->alloc);
	if (issp == NULL) {
		return FSM_DETERMINISE_WITH_CONFIG_ERRNO;
	}

	if (state_limit != 0 && fsm_countstates(nfa) > state_limit) {
		res = FSM_DETERMINISE_WITH_CONFIG_STATE_LIMIT_REACHED;
		goto cleanup;
	}

	{
		fsm_state_t start;
		interned_state_set_id start_set;

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
			res = FSM_DETERMINISE_WITH_CONFIG_OK;
			goto cleanup;
		}

#if LOG_DETERMINISE_CAPTURES
		fprintf(stderr, "#### Adding mapping for start state %u -> 0\n", start);
#endif
		if (!interned_state_set_intern_set(issp, 1, &start, &start_set)) {
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
	stack = stack_init(nfa->alloc);
	if (stack == NULL) {
		goto cleanup;
	}

	ac_env.alloc = nfa->alloc;
	ac_env.fsm = nfa;
	ac_env.issp = issp;

	do {
		size_t o_i;

#if LOG_SYMBOL_CLOSURE
		fprintf(stderr, "\nfsm_determinise: current dfacount %lu...\n",
		    dfacount);
#endif

		assert(curr != NULL);

		if (!analyze_closures__pairwise_grouping(&ac_env, curr->iss)) {
			goto cleanup;
		}

		if (!edge_set_advise_growth(&curr->edges, nfa->alloc, ac_env.output_count)) {
			goto cleanup;
		}

		for (o_i = 0; o_i < ac_env.output_count; o_i++) {
			struct mapping *m;
			struct ac_output *output = &ac_env.outputs[o_i];
			interned_state_set_id iss = output->iss;

#if LOG_DETERMINISE_CLOSURES
			fprintf(stderr, "fsm_determinise: output %zu/%zu: cur (dfa %zu) label [",
			    o_i, ac_env.output_count, curr->dfastate);
			dump_labels(stderr, output->labels);
			fprintf(stderr, "] -> iss:%ld: ", output->iss);
			interned_state_set_dump(stderr, issp, output->iss);
			fprintf(stderr, "\n");
#endif

			/*
			 * The set of NFA states output->iss represents a single DFA state.
			 * We use the mappings as a de-duplication mechanism, keyed by the
			 * set of NFA states.
			 */

			/* If this interned_state_set isn't present, then save it as a new mapping.
			 * This is the interned set of states that the current ac_output (set of
			 * labels) leads to. */
			if (map_find(&map, iss, &m)) {
				/* we already have this closure interned, so reuse it */
				assert(m->dfastate < dfacount);
			} else {
				/* not found -- add a new one and push it to the stack for processing */

				if (state_limit != 0 && dfacount > state_limit) {
					res = FSM_DETERMINISE_WITH_CONFIG_STATE_LIMIT_REACHED;
					goto cleanup;
				}
				if (!map_add(&map, dfacount, iss, &m)) {
					goto cleanup;
				}
				dfacount++;
				if (!stack_push(stack, m)) {
					goto cleanup;
				}
			}

#if LOG_SYMBOL_CLOSURE
			fprintf(stderr, "fsm_determinise: adding labels [");
			dump_labels(stderr, output->labels);
			fprintf(stderr, "] -> dfastate %zu on output state %zu\n", m->dfastate, curr->dfastate);
#endif

			if (!edge_set_add_bulk(&curr->edges, nfa->alloc, output->labels, m->dfastate)) {
				goto cleanup;
			}
		}

		ac_env.output_count = 0;

		/* All elements in sclosures[] are interned, so they will be freed later. */
	} while ((curr = stack_pop(stack)));

	{
		struct map_iter it;
		struct mapping *m;
		struct fsm *dfa;

		dfa = fsm_new(nfa->alloc);
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
				interned_state_set_id iss_id = m->iss;
				fsm_state_t state;
				struct state_set *ss = interned_state_set_get_state_set(ac_env.issp, iss_id);
				fprintf(stderr, "%zu:", m->dfastate);

				for (state_set_reset(ss, &si); state_set_next(&si, &state); ) {
					fprintf(stderr, " %u", state);
				}
				fprintf(stderr, "\n");
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
			interned_state_set_id iss_id = m->iss;
			assert(m->dfastate < dfa->statecount);
			assert(dfa->states[m->dfastate].edges == NULL);

			dfa->states[m->dfastate].edges = m->edges;

			/*
			 * The current DFA state is an end state if any of its associated NFA
			 * states are end states.
			 */
			ss = interned_state_set_get_state_set(ac_env.issp, iss_id);
			if (!state_set_has(nfa, ss, fsm_isend)) {
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
		}

		if (!remap_capture_actions(&map, issp, dfa, nfa)) {
			goto cleanup;
		}

		fsm_move(nfa, dfa);
	}

#if EXPENSIVE_CHECKS
	assert(fsm_all(nfa, fsm_isdfa));
#endif

	res = FSM_DETERMINISE_WITH_CONFIG_OK;

cleanup:
	map_free(&map);
	stack_free(stack);
	interned_state_set_pool_free(issp);

	if (ac_env.outputs != NULL) {
		f_free(ac_env.alloc, ac_env.outputs);
	}
	if (ac_env.dst != NULL) {
		f_free(ac_env.alloc, ac_env.dst);
	}
	if (ac_env.cvect.ids != NULL) {
		f_free(ac_env.alloc, ac_env.cvect.ids);
	}
	if (ac_env.groups != NULL) {
		f_free(ac_env.alloc, ac_env.groups);
	}

	f_free(ac_env.alloc, ac_env.pbuf[0]);
	f_free(ac_env.alloc, ac_env.pbuf[1]);
	f_free(ac_env.alloc, ac_env.htab.buckets);
	f_free(ac_env.alloc, ac_env.to_sets.buf);
	f_free(ac_env.alloc, ac_env.to_set_htab.buckets);
	f_free(ac_env.alloc, ac_env.dst_tmp.buf);
	for (size_t i = 0; i < ac_env.results.used; i++) {
		f_free(ac_env.alloc, ac_env.results.rs[i]);
	}
	f_free(ac_env.alloc, ac_env.results.rs);
	f_free(ac_env.alloc, ac_env.results.buffer.entries);

	if (LOG_ANALYSIS_STATS) {
		const size_t count_total = ac_env.count_single + ac_env.count_pair;
		fprintf(stderr, "%s: counts: single %zu/%zu (%g%%), pair %zu/%zu (%g%%)\n",
		    __func__,
		    ac_env.count_single, count_total, (100.0 * ac_env.count_single)/count_total,
		    ac_env.count_pair, count_total, (100.0 * ac_env.count_pair)/count_total);

		if (TRACK_TIMES) {
			const size_t usec_total = ac_env.usec_single + ac_env.usec_pair;
			fprintf(stderr, "%s: usec: single %zu/%zu (%g%%), pair %zu/%zu (%g%%)\n",
			    __func__,
			    ac_env.usec_single, usec_total, (100.0 * ac_env.usec_single)/usec_total,
			    ac_env.usec_pair, usec_total, (100.0 * ac_env.usec_pair)/usec_total);
		}
	}

	return res;
}

int
fsm_determinise(struct fsm *nfa)
{
	enum fsm_determinise_with_config_res res = fsm_determinise_with_config(nfa, NULL);
	switch (res) {
	case FSM_DETERMINISE_WITH_CONFIG_OK:
		return 1;
	case FSM_DETERMINISE_WITH_CONFIG_STATE_LIMIT_REACHED:
		/* unreachable */
		return 0;
	case FSM_DETERMINISE_WITH_CONFIG_ERRNO:
		return 0;
	}
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
	struct det_copy_capture_actions_env env = {
#ifndef NDEBUG
		'D',
#endif
		NULL, NULL, 1
	};
	env.dst = dst;
	env.reverse_mappings = reverse_mappings;

	fsm_capture_action_iter(src, det_copy_capture_actions_cb, &env);
	return env.ok;
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static uint64_t
hash_iss(interned_state_set_id iss)
{
	/* Just hashing the ID directly is fine here -- since they're
	 * interned, they're identified by pointer equality. */
	return hash_id((uintptr_t)iss);
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
	fsm_state_t dfastate, interned_state_set_id iss, struct mapping **new_mapping)
{
	size_t i;
	uint64_t h, mask;

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
		uint64_t h;
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
map_find(const struct map *map, interned_state_set_id iss,
	struct mapping **mapping)
{
	size_t i;
	uint64_t h, mask;

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
#if AC_LOG
		fprintf(stderr, "stack_push: growing stack %zu -> %zu\n",
		    stack->ceil, nceil);
#endif

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
remap_capture_actions(struct map *map, struct interned_state_set_pool *issp,
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
	reverse_mappings = f_calloc(dst_dfa->alloc, src_nfa->statecount, sizeof(reverse_mappings[0]));
	if (reverse_mappings == NULL) {
		return 0;
	}

	/* build reverse mappings table: for every NFA state X, if X is part
	 * of the new DFA state Y, then add Y to a list for X */
	for (m = map_first(map, &it); m != NULL; m = map_next(&it)) {
		struct state_set *ss;
		interned_state_set_id iss_id = m->iss;
		assert(m->dfastate < dst_dfa->statecount);
		ss = interned_state_set_get_state_set(issp, iss_id);

		for (state_set_reset(ss, &si); state_set_next(&si, &state); ) {
			if (!add_reverse_mapping(dst_dfa->alloc,
				reverse_mappings,
				m->dfastate, state)) {
				goto cleanup;
			}
		}
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
			f_free(dst_dfa->alloc, reverse_mappings[i].list);
		}
	}
	f_free(dst_dfa->alloc, reverse_mappings);

	return res;
}

static int
group_labels_overlap(const struct ac_group *a, const struct ac_group *b)
{
	size_t i;

	if (a->words_used & b->words_used) {
		for (i = 0; i < 4; i++) {
			const uint64_t wa = a->labels[i];
			if (wa && (wa & b->labels[i])) {
				return 1;
			}
		}
	}

	return 0;
}

static void
intersect_with(uint64_t *a, const uint64_t *b)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		const uint64_t aw = a[i];
		if (aw == 0) {
			continue;
		}
		a[i] = aw & b[i];
	}
}

static void
union_with(uint64_t *a, const uint64_t *b)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		const uint64_t bw = b[i];
		if (bw == 0) {
			continue;
		}
		a[i] |= bw;
	}
}

static void
clear_group_labels(struct ac_group *g, const uint64_t *b)
{
	size_t i;
	uint8_t bit;
	uint64_t *a = g->labels;

	for (i = 0, bit = 0x01; i < 4; i++, bit <<= 1) {
		if (g->words_used & bit) {
			const uint64_t bw = b[i];
			a[i] &=~ bw;
			if (a[i] == 0) {
				g->words_used &=~ bit;
			}
		}
	}
}

static int
analyze_closures__init_groups(struct analyze_closures_env *env)
{
	if (env->group_ceil == 0) {
		const size_t nceil = DEF_GROUP_CEIL;
		struct ac_group *ngs = f_malloc(env->alloc,
		    nceil * sizeof(env->groups[0]));
		if (ngs == NULL) {
			return 0;
		}
		env->groups = ngs;
		env->group_ceil = nceil;
	}

	assert(env->groups != NULL);
	env->group_count = 0;
	memset(&env->groups[0], 0x00, sizeof(env->groups[0]));
	env->groups[0].to = AC_NO_STATE;
	return 1;
}

static int
grow_clearing_vector(struct analyze_closures_env *env)
{
	const size_t nceil = (env->cvect.ceil == 0
	    ? DEF_CVECT_CEIL
	    : 2*env->cvect.ceil);
	fsm_state_t *nids = f_realloc(env->alloc,
	    env->cvect.ids, nceil * sizeof(nids[0]));
	if (nids == NULL) {
		return 0;
	}

	env->cvect.ceil = nceil;
	env->cvect.ids = nids;
	return 1;
}

#if EXPENSIVE_CHECKS
static void
assert_labels_are_disjoint(const struct analysis_result *r)
{
	/* verify that the labels sets are disjoint */
	uint64_t seen[256/64] = { 0 };
	for (size_t e_i = 0; e_i < r->count; e_i++) {
		const struct result_entry *re = &r->entries[e_i];
		for (size_t i = 0; i < 4; i++) {
			assert((seen[i] & re->labels[i]) == 0);
			seen[i] |= re->labels[i];
		}
	}
}
#endif

static int
analyze_closures__pairwise_grouping(struct analyze_closures_env *env,
	interned_state_set_id iss_id)
{
#if LOG_GROUPING > 1
	fprintf(stderr, "%s: iss_id %lu\n", __func__, iss_id);
#endif

	analyze_closures__init_groups(env);

	INIT_TIMERS();
	TIME(&pre);
	struct state_set *ss = interned_state_set_get_state_set(env->issp, iss_id);
	const size_t set_count = state_set_count(ss);

#if LOG_DETERMINISE_STATE_SETS || LOG_GROUPING > 1
	{
		static size_t ss_id;
		struct state_iter it;
		fsm_state_t s;
		fprintf(stderr, "ss %zu :", ss_id++);
		state_set_reset(ss, &it);
		while (state_set_next(&it, &s)) {
			fprintf(stderr, " %d", s);
		}
		fprintf(stderr, "\n");
	}
#endif

	/* grow buffers as necessary */
	if (set_count > env->pbuf_ceil) {
		size_t nceil = env->pbuf_ceil == 0
		    ? DEF_PBUF_CEIL
		    : 2*env->pbuf_ceil;
		while (nceil < set_count) {
			nceil *= 2;
		}
		fsm_state_t *a = f_realloc(env->alloc,
		    env->pbuf[0],
		    nceil * sizeof(env->pbuf[0]));
		if (a == NULL) { return 0; }
		fsm_state_t *b = f_realloc(env->alloc,
		    env->pbuf[1],
		    nceil * sizeof(env->pbuf[1]));
		if (b == NULL) { return 0; }
		env->pbuf_ceil = nceil;
		env->pbuf[0] = a;
		env->pbuf[1] = b;
	}

	{
		/* init round 0
		 * should be possible to do this using state_set_array */
		size_t i = 0;
		struct state_iter it;
		fsm_state_t s;
		state_set_reset(ss, &it);
		while (state_set_next(&it, &s)) {
			assert(i < env->pbuf_ceil);
			env->pbuf[0][i] = s;
			i++;
		}
	}

	if (set_count == 1) {
		const fsm_state_t id = env->pbuf[0][0];
		assert(0 == (id & RESULT_BIT));
		return analyze_single_state(env, id);
	}

	/* Reduce via pairwise merging, flip/flopping between the
	 * two buffers, until there is only one state/pair id, then
	 * intern the resulting state sets and push any new ones onto
	 * the stack for later processing.
	 *
	 * Cache intermediate results of merging each pair. */
	size_t round = 0;	/* used to flip buffers */
	size_t cur_used = set_count;
	size_t next_used = 0;

	size_t cur_i = 0;
	fsm_state_t *cbuf = env->pbuf[(round + 0) & 1];
	fsm_state_t *nbuf = env->pbuf[(round + 1) & 1];

	while (cur_used > 1) {
#if LOG_GROUPING > 1
		if (cur_i == 0) {
			fprintf(stderr, "%s: round %zu:", __func__, round);
			for (size_t i = 0; i < cur_used; i++) {
				const fsm_state_t id = cbuf[i];
				fprintf(stderr, " %d%s", ID_WITH_SUFFIX(id));
			}
			fprintf(stderr, "\n");
		}
#endif

		if (cur_i == cur_used) { /* flip */
			cur_used = next_used;
			next_used = 0;
			cur_i = 0;
			round++;
			cbuf = env->pbuf[(round + 0) & 1];
			nbuf = env->pbuf[(round + 1) & 1];
		} else if ((cur_used & 0x01) && cur_i == cur_used - 1) {
			/* if the total count is odd and there's only one
			 * left, bounce it to the next round, because
			 * (count/2)+1 will be even */
			assert(next_used + 1 < env->pbuf_ceil);
			nbuf[next_used] = cbuf[cur_i];
			next_used++;
			cur_i++;
		} else {
			assert(cur_i + 2 <= cur_used);

			/* Take two IDs, which are either state IDs or
			 * result IDs, and get a result_id for the group with
			 * their combined result. This will be cached, because
			 * the same ID pairs tend to repeat frequently. */
			fsm_state_t a = cbuf[cur_i++];
			fsm_state_t b = cbuf[cur_i++];
			fsm_state_t result_id = a; /* out-of-bounds value */
			if (!combine_pair(env, a, b, &result_id)) {
				return 0;
			}
#if LOG_GROUPING > 1
			fprintf(stderr, "%s: combine_pair a %d, b %d => result_id %d\n",
			    __func__, a, b, result_id);
#endif
			/* the result_id cannot match either of these */
			result_id |= RESULT_BIT;
			assert(result_id != a);
			assert(result_id != b);
			nbuf[next_used++] = result_id;
		}
	}

	assert(cur_used == 1);
	assert(cbuf[0] & RESULT_BIT);
	const fsm_state_t result_id = cbuf[0] &~ RESULT_BIT;
	assert(result_id < env->results.used);

#if LOG_GROUPING > 1
	fprintf(stderr, "%s: result %d in %zu rounds\n", __func__, result_id, round);
#endif

#if EXPENSIVE_CHECKS
	assert_labels_are_disjoint(env->results.rs[result_id]);
#endif

	if (!build_output_from_cached_analysis(env, result_id)) {
		return 0;
	}
	TIME(&post);
	DIFF_MSEC("det_pairwise_grouping", pre, post, NULL);

	return 1;
}

static int
cache_single_state_analysis(struct analyze_closures_env *env, fsm_state_t state_id,
    fsm_state_t *cached_result_id)
{
#if LOG_GROUPING > 1
	fprintf(stderr, "%s: state_id %d\n", __func__, state_id);
#endif

	INIT_TIMERS();
	TIME(&pre);

	/* build up outputs
	 *
	 * info.to will be ascending
	 *
	 * look at analyze_closures__collect */
	const struct fsm_state *s = &env->fsm->states[state_id];
	struct edge_group_iter iter;
	struct edge_group_iter_info info;

	analyze_closures__init_groups(env);
	assert(env->group_count == 0);

	/* collect groups */
	edge_set_group_iter_reset(s->edges, EDGE_GROUP_ITER_ALL, &iter);
	while (edge_set_group_iter_next(&iter, &info)) {
#if LOG_GROUPING > 2
		fprintf(stderr, "%s: to %d, symbols 0x%016lx%016lx%016lx%016lx\n",
		    __func__, info.to,
		    info.symbols[0], info.symbols[1], info.symbols[2], info.symbols[3]);
#endif

		if (env->group_count + 1 == env->group_ceil) {
			if (!analyze_closures__grow_groups(env)) {
				return 0;
			}
		}
		assert(env->group_count < env->group_ceil);
		struct ac_group *g = &env->groups[env->group_count];

		if (g->to == AC_NO_STATE) { /* init new group */
			memset(g, 0x00, sizeof(*g));
			g->to = info.to;
			union_with(g->labels, info.symbols);
		} else if (g->to == info.to) { /* update current group */
			union_with(g->labels, info.symbols);
		} else {	/* switch to next group */
			assert(info.to > g->to);
			env->group_count++;

			if (env->group_count + 1 == env->group_ceil) {
				if (!analyze_closures__grow_groups(env)) {
					return 0;
				}
			}
			assert(env->group_count < env->group_ceil);

			struct ac_group *ng = &env->groups[env->group_count];
			memset(ng, 0x00, sizeof(*ng));
			ng->to = info.to;
			union_with(ng->labels, info.symbols);
		}
	}
	if (env->groups[env->group_count].to != AC_NO_STATE) {
		env->group_count++;	/* commit current group */
	}

	/* Initialize words_used for each group. */
	for (size_t g_i = 0; g_i < env->group_count; g_i++) {
		size_t w_i;
		uint8_t bit;
		struct ac_group *g = &env->groups[g_i];
		g->words_used = 0;
		for (w_i = 0, bit = 0x01; w_i < 4; w_i++, bit <<= 1) {
			if (g->labels[w_i] != 0) {
				g->words_used |= bit;
			}
		}
	}

	if (env->group_count == 0) {
#if LOG_GROUPING > 1
		fprintf(stderr, "Caching empty group analysis\n");
#endif

		if (!commit_buffered_result(env, cached_result_id)) {
			return 0;
		}
#if LOG_GROUPING > 1
		fprintf(stderr, "%s: cached analysis as result %d\n", __func__, *cached_result_id);
#endif

		if (!analysis_cache_save_single_state(env, state_id, *cached_result_id)) {
			return 0;
		}

		TIME(&post);
		DIFF_MSEC("cache_single_state_analysis_0", pre, post, &env->usec_single);
		env->count_single++;
		return 1;
	}

	/* now we have groups */
#if LOG_GROUPING > 1
	for (size_t i = 0; i < env->group_count; i++) {
		const struct ac_group *g = &env->groups[i];
		fprintf(stderr, "%s: group[%zu]: to %d, labels 0x%016lx%016lx%016lx%016lx\n",
		    __func__, i, g->to, g->labels[0], g->labels[1], g->labels[2], g->labels[3]);
	}
#endif

	/* This partitions the table into an array of non-overlapping
	 * (label set -> state set) pairs.
	 *
	 * For the first group in the table, find labels in that group that
	 * are shared by later groups, create a set of the states those
	 * groups lead to, then clear those labels. If the first group is
	 * empty, advance base_i to the next group, and repeat until all
	 * the groups in the table have been cleared. Because each pass
	 * clears lables, this must eventually terminate.
	 *
	 * The pass that clears the labels on groups that were just
	 * added to the result set benefits from the groups being sorted
	 * by ascending .to fields, but other approaches may be faster.
	 * I tried sorting the table by ascending count of bits used in
	 * the label sets, so earlier groups with only a few labels
	 * would be cleared faster and reduce the overall working set
	 * sooner, but neither approach seemed to be clearly faster for
	 * my test data sets. There is also a lot of room here for
	 * speeding up the group label set unioning, intersecting, and
	 * clearing. Casual experiments suggested small improvements but
	 * not drastic improvements, so I'm leaving that performance
	 * work for later. */
	size_t base_i, o_i; /* base_i, other_i */

	base_i = 0;
	env->output_count = 0;

	if (env->dst_ceil == 0) {
		if (!analyze_closures__grow_dst(env)) {
			return 0;
		}
	}

	while (base_i < env->group_count) {
		/* labels assigned in current sweep. The g_labels wrapper
		 * is used for g_labels.words_used. */
		struct ac_group g_labels = { 0 };
		uint64_t *labels = g_labels.labels;
		size_t dst_count = 0;

#if LOG_AC
		fprintf(stderr, "base_i %zu/%zu\n",
		    base_i, env->group_count);
#endif

		const struct ac_group *bg = &env->groups[base_i];
		memcpy(&g_labels, bg, sizeof(*bg));

		/* At least one bit should be set; otherwise
		 * we should have incremented base_i. */
		assert(labels[0] || labels[1]
		    || labels[2] || labels[3]);

#if LOG_AC
		fprintf(stderr, "ac_analyze: dst[%zu] <- %d (base)\n", dst_count, bg->to);
#endif

		if (dst_count + 1 == env->dst_ceil) {
			if (!analyze_closures__grow_dst(env)) {
				return 0;
			}
		}
		env->dst[dst_count] = bg->to;
		dst_count++;

		if (env->cvect.ceil == 0) {
			if (!grow_clearing_vector(env)) { return 0; }
		}
		env->cvect.ids[0] = base_i;
		env->cvect.used = 1;

		for (o_i = base_i + 1; o_i < env->group_count; o_i++) {
			const struct ac_group *og = &env->groups[o_i];
			if (og->words_used == 0) {
				continue; /* skip empty groups */
			}

			/* TODO: Combining checking for overlap and intersecting
			 * into one pass would be ugly, but a little faster, and
			 * this loop has a big impact on performance. */
			if (group_labels_overlap(&g_labels, og)) {
				intersect_with(labels, og->labels);
#if LOG_AC
				fprintf(stderr, "ac_analyze: dst[%zu] <- %d (other w/ overlapping labels)\n", dst_count, og->to);
#endif

				if (dst_count + 1 == env->dst_ceil) {
					if (!analyze_closures__grow_dst(env)) {
						return 0;
					}
				}

				env->dst[dst_count] = og->to;
				dst_count++;

				if (env->cvect.used == env->cvect.ceil) {
					if (!grow_clearing_vector(env)) { return 0; }
				}
				env->cvect.ids[env->cvect.used] = o_i;
				env->cvect.used++;
			}
		}

#if LOG_AC
		fprintf(stderr, "ac_analyze: dst_count: %zu\n", dst_count);
#endif

		assert(labels[0] || labels[1]
		    || labels[2] || labels[3]);

		for (size_t c_i = 0; c_i < env->cvect.used; c_i++) {
			const fsm_state_t g_id = env->cvect.ids[c_i];
			assert(g_id < env->group_count);
			struct ac_group *g = &env->groups[g_id];
			clear_group_labels(g, labels);
		}

		if (LOG_AC) {
			size_t i;
			fprintf(stderr, "new_edge_group:");
			for (i = 0; i < dst_count; i++) {
				fprintf(stderr, " %d", env->dst[i]);
			}
			fprintf(stderr, " -- ");
			dump_labels(stderr, labels);
			fprintf(stderr, "\n");
		}

		{		/* add dst set and labels to buffer */
#if LOG_GROUPING > 1
			fprintf(stderr, "-- dst [");
			for (size_t i = 0; i < dst_count; i++) {
				fprintf(stderr, "%s%d", i > 0 ? " " : "", env->dst[i]);
			}
			fprintf(stderr, "] <- labels 0x%016lx%016lx%016lx%016lx\n",
			    labels[0], labels[1], labels[2], labels[3]);
#endif
			/* reserve to-set and copy env->dst[] there */
			/* push a group onto env->results.buffer.entries[] */
			if (!add_group_dst_info_to_buffer(env,
				dst_count, env->dst, labels)) {
				return 0;
			}
		}

		while (base_i < env->group_count && env->groups[base_i].words_used == 0) {
#if LOG_AC
			fprintf(stderr, "ac_analyze: base %zu all clear, advancing\n", base_i);
#endif
			base_i++;
		}
	}

	if (!commit_buffered_result(env, cached_result_id)) {
		return 0;
	}
#if LOG_GROUPING > 1
	fprintf(stderr, "%s: cached analysis as result %d\n", __func__, *cached_result_id);
#endif

	if (!analysis_cache_save_single_state(env, state_id, *cached_result_id)) {
		return 0;
	}

	TIME(&post);
	DIFF_MSEC(__func__, pre, post, &env->usec_single);
	env->count_single++;
	return 1;
}

static int
analyze_single_state(struct analyze_closures_env *env, fsm_state_t id)
{
	assert(id < env->fsm->statecount);
	fsm_state_t cached_result_id;
	if (!analysis_cache_check_single_state(env, id, &cached_result_id)) {
		if (!cache_single_state_analysis(env, id, &cached_result_id)) {
			return 0;
		}
	}

	return build_output_from_cached_analysis(env, cached_result_id);
}

static int
build_output_from_cached_analysis(struct analyze_closures_env *env, fsm_state_t cached_result_id)
{
	/* collect (ISS, labels) outputs for single group */
	const struct analysis_result *r = env->results.rs[cached_result_id];
	for (size_t i = 0; i < r->count; i++) {
		const struct result_entry *e = &r->entries[i];
		const uint64_t *labels = e->labels;

		const size_t dst_count = e->to_set_count;
		assert(e->to_set_offset + dst_count <= env->to_sets.used);
		const fsm_state_t *dst = &env->to_sets.buf[e->to_set_offset];

		interned_state_set_id iss;
		if (!interned_state_set_intern_set(env->issp, dst_count, dst, &iss)) {
			return 0;
		}

		if (!analyze_closures__save_output(env, labels, iss)) {
			return 0;
		}
	}

	return 1;
}

#define LOG_TO_SET_HTAB 0

static uint64_t
to_set_hash(size_t count, const fsm_state_t *ids)
{
	return hash_ids(count, ids);
}

static int
to_set_htab_check(struct analyze_closures_env *env,
    size_t count, const fsm_state_t *dst, uint64_t *hash, uint32_t *out_offset)
{
	const uint64_t h = to_set_hash(count, dst);
	*hash = h;

	struct to_set_htab *htab = &env->to_set_htab;
	const size_t bcount = htab->bucket_count;
	if (bcount == 0) {
		return 0;
	}

#if LOG_TO_SET_HTAB || HASH_LOG_PROBES || defined(HASH_PROBE_LIMIT)
	size_t probes = 0;
#endif
	int res = 0;

	const uint64_t mask = bcount - 1;
	for (size_t b_i = 0; b_i < bcount; b_i++) {
#if LOG_TO_SET_HTAB || HASH_LOG_PROBES || defined(HASH_PROBE_LIMIT)
		probes++;
#endif
		const struct to_set_bucket *b = &htab->buckets[(h + b_i) & mask];
		if (b->count == 0) {
			goto done; /* empty bucket -> not found */
		} else if (b->count == count) {
			assert(env->to_sets.buf != NULL);
			assert(b->offset + count <= env->to_sets.used);
			const fsm_state_t *ids = &env->to_sets.buf[b->offset];
			if (0 == memcmp(ids, dst, count * sizeof(dst[0]))) {
				*out_offset = b->offset;
				res = 1;  /* cache hit */
				goto done;
			} else {
				continue; /* collision */
			}
		} else {
			continue; /* collision */
		}
	}

done:
#if LOG_TO_SET_HTAB || HASH_LOG_PROBES
	fprintf(stderr, "%s: result %d, %zu probes, htab: used %zu/%zu ceil\n",
	    __func__, res, probes, htab->buckets_used, htab->bucket_count);
#endif
#ifdef HASH_PROBE_LIMIT
	if (probes >= HASH_PROBE_LIMIT) {
		fprintf(stderr, "-- %zd probes, limit exceeded\n", probes);
	}
	assert(probes < HASH_PROBE_LIMIT);
#endif

	return res;
}

static int
to_set_htab_save(struct analyze_closures_env *env,
    size_t count, uint64_t hash, uint32_t offset)
{
	struct to_set_htab *htab = &env->to_set_htab;
	if (htab->buckets_used >= htab->bucket_count/2) {
		const uint64_t ocount = htab->bucket_count;
		const size_t ncount = (ocount == 0
		    ? DEF_TO_SET_HTAB_BUCKET_COUNT
		    : 2*ocount);
#if LOG_TO_SET_HTAB
		fprintf(stderr, "%s: growing %zu -> %zu\n",
		    __func__, ocount, ncount);
#endif
		struct to_set_bucket *nbuckets = f_calloc(env->alloc,
		    ncount, sizeof(nbuckets[0]));
		if (nbuckets == NULL) {
			return 0;
		}

		const struct to_set_bucket *obuckets = htab->buckets;
		const uint64_t nmask = ncount - 1;
		if (ocount > 0) {
			for (size_t ob_i = 0; ob_i < ocount; ob_i++) {
				const struct to_set_bucket *ob = &obuckets[ob_i];
				if (ob->count == 0) {
					continue; /* empty */
				}
				assert(ob->offset + ob->count <= env->to_sets.used);
				const fsm_state_t *ids = &env->to_sets.buf[ob->offset];
				const uint64_t h = to_set_hash(ob->count, ids);

				for (size_t nb_i = 0; nb_i < ncount; nb_i++) {
					struct to_set_bucket *nb = &nbuckets[(h + nb_i) & nmask];
					if (nb->count == 0) {
						nb->count = ob->count;
						nb->offset = ob->offset;
						break;
					} else {
						continue; /* collision */
					}
				}
			}
		}
		f_free(env->alloc, htab->buckets);
		htab->buckets = nbuckets;
		htab->bucket_count = ncount;
	}

	const size_t bcount = htab->bucket_count;
	assert(htab->buckets_used <= bcount/2);
	assert(bcount > 1);
	const uint64_t mask = bcount - 1;
	for (size_t b_i = 0; b_i < htab->bucket_count; b_i++) {
		struct to_set_bucket *b = &htab->buckets[(hash + b_i) & mask];
		if (b->count == 0) {
			b->count = count;
			b->offset = offset;
			htab->buckets_used++;
#if HASH_LOG_PROBES
			fprintf(stderr, "%s: [", __func__);
			const fsm_state_t *ids = &env->to_sets.buf[offset];
			for (size_t i = 0; i < count; i++) {
				fprintf(stderr, "%s%d",
				    i > 0 ? " " : "", ids[i]);
			}

			fprintf(stderr, "] -> hash %lx -> b %zu (%zu/%zu used), %zd probes\n",
			    hash, (hash + b_i) & mask,
			    htab->buckets_used, htab->bucket_count, b_i);
#endif
#if HASH_PROBE_LIMIT
			assert(b_i < HASH_PROBE_LIMIT);
#endif

			return 1;
		} else if (b->count == count) {
			assert(b->offset != offset); /* no duplicates */
			continue; /* collision */
		}
	}

	assert(!"unreachable");
	return 0;
}

static int
save_to_set(struct analyze_closures_env *env,
    size_t count, const fsm_state_t *dst, uint32_t *out_offset)
{
#define LOG_TO_SET 0
	/* dst should not be a pointer into the to_sets buffer,
	 * because it can be relocated. */
	if (env->to_sets.buf != NULL) {
		assert(dst < &env->to_sets.buf[0]
		    || dst >= &env->to_sets.buf[env->to_sets.ceil]);
	}

	/* There is substantial duplication of these sets, so
	 * check the cache hash table first. */
	uint64_t hash;
	if (to_set_htab_check(env, count, dst, &hash, out_offset)) {
		return 1;
	}

	if (env->to_sets.used + count >= env->to_sets.ceil) {
		size_t nceil = (env->to_sets.ceil == 0
		    ? DEF_TO_SET_CEIL
		    : 2*env->to_sets.ceil);
		while (nceil < env->to_sets.used + count) {
			nceil *= 2;
		}

#if LOG_GROUPING || LOG_TO_SET
		fprintf(stderr, "%s: growing %zu -> %zu\n",
		    __func__, env->to_sets.ceil, nceil);
#endif
		fsm_state_t *nbuf = f_realloc(env->alloc, env->to_sets.buf,
		    nceil * sizeof(nbuf[0]));
		if (nbuf == NULL) {
			return 0;
		}
		env->to_sets.ceil = nceil;
		env->to_sets.buf = nbuf;
	}
	assert(env->to_sets.buf != NULL);

#if LOG_TO_SET
	static size_t to_set_id;
	fprintf(stderr, "%s: %zu:", __func__, to_set_id++);
	for (size_t i = 0; i < count; i++) {
		fprintf(stderr, " %d", dst[i]);
	}
	fprintf(stderr, "\n");
#endif

	uint32_t offset = env->to_sets.used;
	assert(offset + count <= env->to_sets.ceil);
	memcpy(&env->to_sets.buf[offset], dst, count * sizeof(dst[0]));
	env->to_sets.used += count;
	assert(env->to_sets.used <= env->to_sets.ceil);

	if (!to_set_htab_save(env, count, hash, offset)) {
		return 0;
	}

	*out_offset = offset;
	return 1;
}

static int
add_group_dst_info_to_buffer(struct analyze_closures_env *env,
	size_t dst_count, const fsm_state_t *dst,
	const uint64_t labels[256/64])
{
	struct result_buffer *buf = &env->results.buffer;

	if (buf->used == buf->ceil) {
		const size_t nceil = (buf->ceil == 0
		    ? DEF_GROUP_BUFFER_CEIL
		    : 2*buf->ceil);
		struct result_entry *nentries = f_realloc(env->alloc,
		    buf->entries, nceil * sizeof(buf->entries[0]));
		if (nentries == NULL) { return 0; }
		buf->entries = nentries;
		buf->ceil = nceil;
	}

	struct result_entry *ge = &buf->entries[buf->used];
	if (!save_to_set(env, dst_count, dst,
		&ge->to_set_offset)) {
		return 0;
	}
	ge->to_set_count = dst_count;
	ge->words_used = 0;
	memcpy(ge->labels, labels, sizeof(ge->labels));

	for (size_t i = 0; i < 4; i++) {
		if (labels[i] != 0) {
			ge->words_used |= (uint8_t)1 << i;
		}
	}

	buf->used++;
	return 1;
}

static int
commit_buffered_result(struct analyze_closures_env *env, uint32_t *cache_result_id)
{
#define LOG_COMMIT_BUFFERED_GROUP 0

	/* Based on logging hashes from benchmarks, there is usually
	 * little to no duplication here, so they are stored as-is. */

	if (env->results.used == env->results.ceil) {
		const size_t nceil = (env->results.ceil == 0
		    ? DEF_GROUP_GS
		    : 2*env->results.ceil);
		struct analysis_result **nrs = f_realloc(env->alloc,
		    env->results.rs, nceil * sizeof(env->results.rs[0]));
#if LOG_GROUPING || LOG_COMMIT_BUFFERED_GROUP
		fprintf(stderr, "%s: growing %zu -> %zu\n",
		    __func__, env->results.ceil, nceil);
#endif
		if (nrs == NULL) {
			return 0;
		}
		env->results.ceil = nceil;
		env->results.rs = nrs;
	}

	struct analysis_result *nr;
	const size_t alloc_sz = sizeof(*nr)
	    + env->results.buffer.used * sizeof(nr->entries[0]);
	nr = f_malloc(env->alloc, alloc_sz);
#if LOG_GROUPING || LOG_COMMIT_BUFFERED_GROUP
	fprintf(stderr, "%s: alloc %zu\n",
	    __func__, sizeof(*nr) + env->results.buffer.used * sizeof(nr->entries[0]));
#endif
	if (nr == NULL) {
		return 0;
	}
	nr->count = env->results.buffer.used;

	if (nr->count > 0) {
		assert(env->results.buffer.entries != NULL);
		memcpy(&nr->entries[0], env->results.buffer.entries,
		    nr->count * sizeof(env->results.buffer.entries[0]));
#if LOG_GROUPING || LOG_COMMIT_BUFFERED_GROUP
		fprintf(stderr, "%s: alloc %zu, count %u\n",
		    __func__, alloc_sz, nr->count));
#endif
	}

#if EXPENSIVE_CHECKS
	assert_labels_are_disjoint(nr);
#endif

	const uint32_t cache_id = env->results.used;
	env->results.rs[cache_id] = nr;
	env->results.used++;

	/* clear buffer */
	env->results.buffer.used = 0;

	*cache_result_id = cache_id;
	return 1;
}

#define LOG_CACHE_HTAB 0
#if LOG_CACHE_HTAB
#define CACHE_HTAB_INIT_STATS()			\
	size_t collisions = 0;			\
	static size_t max_collisions
#define CACHE_HTAB_INC_COLLISIONS() collisions++
#define COLLISION_LIMIT 100
#define CACHE_HTAB_PRINT_COLLISIONS(TAG, PA, PB, H)			\
	if (collisions > max_collisions) {				\
		max_collisions = collisions;				\
		fprintf(stderr, "%s: %s: new max_collisions: %zu\n",	\
		    __func__, TAG, max_collisions);			\
	} else if (LOG_CACHE_HTAB > 1					\
	    || collisions > COLLISION_LIMIT) {				\
		fprintf(stderr,						\
		    "%s: %s: collisions %zu, { %d, %d } -> %016lx\n",	\
		    __func__, TAG, collisions,				\
		    PA &~ RESULT_BIT,					\
		    PB &~ RESULT_BIT,					\
		    H);							\
		assert(collisions < COLLISION_LIMIT);			\
	}
#define CACHE_HTAB_DUMP(ENV) dump_pair_cache_htab(env)
#else
#define CACHE_HTAB_INIT_STATS()
#define CACHE_HTAB_INC_COLLISIONS()
#define CACHE_HTAB_PRINT_COLLISIONS(TAG, PA, PB, H)
#define CACHE_HTAB_DUMP(ENV)
#endif

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static uint64_t
hash_pair(fsm_state_t a, fsm_state_t b)
{
	assert(a != b);
	assert(a & RESULT_BIT);
	assert(b & RESULT_BIT);
	const uint64_t ma = (uint64_t)(a & ~RESULT_BIT); /* m: masked */
	const uint64_t mb = (uint64_t)(b & ~RESULT_BIT);
	assert(ma != mb);

	/* Left-shift the smaller ID, so the pair order is consistent for hashing. */
	const uint64_t ab = (ma < mb)
	    ? ((ma << 32) | mb)
	    : ((mb << 32) | ma);
	const uint64_t res = hash_id(ab);
	/* fprintf(stderr, "%s: a %d, b %d -> %016lx\n", __func__, a, b, res); */
	return res;
}

static int eq_bucket_pair(const struct result_pair_bucket *b, fsm_state_t pa, fsm_state_t pb)
{
	if ((pa & ~RESULT_BIT) < (pb & ~RESULT_BIT)) {
		return b->ids[0] == pa && b->ids[1] == pb;
	} else {
		return b->ids[0] == pb && b->ids[1] == pa;
	}
}

static int
analysis_cache_check_pair(struct analyze_closures_env *env, fsm_state_t pa, fsm_state_t pb,
	fsm_state_t *result_id)
{
	assert(pa & RESULT_BIT);
	assert(pb & RESULT_BIT);

	const struct result_pair_htab *htab = &env->htab;
	if (htab->bucket_count == 0) {
#if LOG_GROUPING > 1
		fprintf(stderr, "%s: empty cache -> cache miss for pair { %d_R, %d_R }\n",
		    __func__, pa &~ RESULT_BIT, pb &~ RESULT_BIT);
#endif
		return 0;
	}

	const uint64_t h = hash_pair(pa, pb);
	const uint64_t mask = htab->bucket_count - 1;

	CACHE_HTAB_INIT_STATS();

	for (size_t i = 0; i < htab->bucket_count; i++) {
		struct result_pair_bucket *b = &htab->buckets[(h + i) & mask];
		if (b->t == RPBT_UNUSED) {
			break;	/* not found */
		} else if (b->t == RPBT_PAIR && eq_bucket_pair(b, pa, pb)) {
			*result_id = b->result_id; /* hit */
#if LOG_GROUPING > 1
			fprintf(stderr, "%s: cache hit for pair { %d_R, %d_R } => %d\n",
			    __func__, pa, pb, b->result_id);
#endif

			CACHE_HTAB_PRINT_COLLISIONS("hit", pa, pb, h);
			return 1;
		} else {
			CACHE_HTAB_INC_COLLISIONS();
			continue; /* collision */
		}
	}

#if LOG_GROUPING > 1
	fprintf(stderr, "%s: cache miss for pair { %d_R, %d_R }\n",
	    __func__, pa, pb);
#endif

	CACHE_HTAB_PRINT_COLLISIONS("miss", pa, pb, h);

	return 0;
}

static int
analysis_cache_check_single_state(struct analyze_closures_env *env, fsm_state_t id,
	fsm_state_t *result_id)
{
	assert(0 == (id & RESULT_BIT));
	const struct result_pair_htab *htab = &env->htab;
	if (htab->bucket_count == 0) {
#if LOG_GROUPING > 1
		fprintf(stderr, "%s: empty cache -> cache miss for state %d\n",
		    __func__, id);
#endif
		return 0;
	}

	CACHE_HTAB_INIT_STATS();

	const uint64_t h = hash_id(id);
	const uint64_t mask = htab->bucket_count - 1;
	for (size_t i = 0; i < htab->bucket_count; i++) {
		struct result_pair_bucket *b = &htab->buckets[(h + i) & mask];
		if (b->t == RPBT_UNUSED) {
			break;	/* not found */
		} else if (b->t == RPBT_SINGLE_STATE && b->ids[0] == id) {
			assert(b->ids[1] == id);
			*result_id = b->result_id; /* hit */
#if LOG_GROUPING > 1
			fprintf(stderr, "%s: cache hit for state %d -> result_id %d\n",
			    __func__, id, b->result_id);
#endif
			CACHE_HTAB_PRINT_COLLISIONS("hit", id, id, h);
			return 1;
		} else {
			CACHE_HTAB_INC_COLLISIONS();
			continue; /* collision */
		}
	}

#if LOG_GROUPING > 1
	fprintf(stderr, "%s: cache miss for state %d\n",
	    __func__, id);
#endif

	CACHE_HTAB_PRINT_COLLISIONS("miss", id, id, h);
	return 0;
}

#if LOG_GROUPING
static void
dump_group(FILE *f, struct analyze_closures_env *env, const struct analysis_result *r)
{
	for (size_t i = 0; i < r->count; i++) {
		const struct result_entry *e = &r->entries[i];
		const uint64_t *labels = e->labels;
		fprintf(f, "-- 0x%016lx%016lx%016lx%016lx -> [",
		    labels[0], labels[1], labels[2], labels[3]);
		for (size_t to_i = 0; to_i < e->to_set_count; to_i++) {
			fprintf(f, "%s%d", to_i > 0 ? " " : "",
			    env->to_sets.buf[e->to_set_offset + to_i]);
		}
		fprintf(f, "]\n");
	}
}
#endif

#if LOG_CACHE_HTAB
static void
dump_pair_cache_htab(const struct analyze_closures_env *env)
{
	unsigned bits = 0;
	size_t total = 0;
	const size_t bucket_count = env->htab.bucket_count;
	fprintf(stderr, "### pair_htab, %zu buckets\n", bucket_count);
	for (size_t i = 0; i < bucket_count; i++) {
		if (env->htab.buckets[i].t != RPBT_UNUSED) {
			bits++;
		}

		if ((i & 7) == 7) {
			fprintf(stderr, "%c",
			    bits == 0 ? '.' : ('0' + bits));
			total += bits;
			bits = 0;
		}

		if ((i & 511) == 511) { /* 64 cells/row */
			fprintf(stderr, "\n");
		}
	}

	if (bucket_count < 512) {
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "used: %zu/%zu\n", total, bucket_count);
}
#endif

static int
pair_cache_save(struct analyze_closures_env *env,
	enum result_pair_bucket_type type,
	fsm_state_t pa, fsm_state_t pb, fsm_state_t result_id)
{
	if (type == RPBT_SINGLE_STATE) {
		assert(pa == pb);
		assert(0 == (pa & RESULT_BIT));
	} else {
		assert(type == RPBT_PAIR);
		assert(pa != pb);
		assert(pa & RESULT_BIT);
		assert(pb & RESULT_BIT);
		assert((pa & ~RESULT_BIT) < (pb & ~RESULT_BIT));
	}

#if LOG_GROUPING > 1
	fprintf(stderr, "%s: type %s { %d%s, %d%s } => %d\n",
	    __func__,
	    type == RPBT_SINGLE_STATE ? "SINGLE" : "PAIR",
	    ID_WITH_SUFFIX(pa), ID_WITH_SUFFIX(pb), result_id);
#endif

	CACHE_HTAB_INIT_STATS();

	struct result_pair_htab *htab = &env->htab;
	if (htab->buckets_used >= htab->bucket_count/2) {
		const size_t ncount = (htab->bucket_count == 0
		    ? DEF_PAIR_CACHE_HTAB_BUCKET_COUNT
		    : 2*htab->bucket_count);
		struct result_pair_bucket *nbuckets = f_calloc(env->alloc,
		    ncount, sizeof(nbuckets[0]));
		if (nbuckets == NULL) {
			return 0;
		}

		if (htab->bucket_count > 0) {
			const uint32_t ocount = htab->bucket_count;
			const uint32_t nmask = ncount - 1;
			CACHE_HTAB_DUMP(env);
			for (size_t ob_i = 0; ob_i < ocount; ob_i++) {
				const struct result_pair_bucket *ob = &htab->buckets[ob_i];
				if (ob->t == RPBT_UNUSED) {
					continue;
				}

				const uint64_t h = (ob->t == RPBT_SINGLE_STATE
				    ? hash_id(ob->ids[0])
				    : hash_pair(ob->ids[0], ob->ids[1]));
				if (ob->t == RPBT_SINGLE_STATE) {
					assert(ob->ids[0] == ob->ids[1]);
				} else {
					assert((ob->ids[0] & ~RESULT_BIT) < (ob->ids[1] & ~RESULT_BIT));
				}

				for (size_t nb_i = 0; nb_i < ncount; nb_i++) {
					struct result_pair_bucket *nb = &nbuckets[(h + nb_i) & nmask];
					if (nb->t == RPBT_UNUSED) {
						memcpy(nb, ob, sizeof(*ob));
						break;
					} else {
						continue; /* collision */
					}
				}
			}
			f_free(env->alloc, htab->buckets);
		}
		htab->buckets = nbuckets;
		htab->bucket_count = ncount;
	}

	const uint64_t h = (pa == pb
	    ? hash_id(pa)
	    : (hash_pair(pa, pb)));

	const uint64_t mask = htab->bucket_count - 1;
	for (size_t i = 0; i < htab->bucket_count; i++) {
		const size_t b_id = (h + i) & mask;
		struct result_pair_bucket *b = &htab->buckets[b_id];
#if LOG_GROUPING > 1
		fprintf(stderr, "%s: bucket[%zu]: t %d, ids[%d%s %d%s], result_id %d\n",
		    __func__, b_id, b->t,
		    ID_WITH_SUFFIX(b->ids[0]), ID_WITH_SUFFIX(b->ids[1]),
		    b->result_id);
#endif

		if (b->t == RPBT_UNUSED) { /* empty */
			/* Ensure pair ordering is consistent */
			b->ids[0] = pa < pb ? pa : pb;
			b->ids[1] = pa < pb ? pb : pa;
			b->t = type;
			b->result_id = result_id;
			htab->buckets_used++;

#if LOG_GROUPING > 1
			const struct analysis_result *r = env->results.rs[result_id];
			if (b->t == RPBT_SINGLE_STATE) {
				fprintf(stderr, "%s: cached result_id %u for single state %d_s, with %u entries, in bucket %zu:\n",
				    __func__, result_id, pa, r->count, b_id);
			} else {
				fprintf(stderr, "%s: cached result_id %u for { %d_R, %d_R }, with %u entries, in bucket %zu:\n",
				    __func__, result_id, pa &~ RESULT_BIT, pb &~ RESULT_BIT, r->count, b_id);
			}
			dump_group(stderr, env, r);
#endif

			CACHE_HTAB_PRINT_COLLISIONS("saved", pa, pb, h);
			return 1;
		} else {
			/* should not save duplicates */
			assert(b->ids[0] != pa || b->ids[1] != pb);
			CACHE_HTAB_INC_COLLISIONS();
			continue; /* collision */
		}
	}

	assert(!"unreachable");
	return 0;
}

static int
analysis_cache_save_single_state(struct analyze_closures_env *env,
	fsm_state_t state_id, fsm_state_t result_id)
{
	assert(0 == (state_id & RESULT_BIT));
	return pair_cache_save(env, RPBT_SINGLE_STATE, state_id, state_id, result_id);
}

static int
analysis_cache_save_pair(struct analyze_closures_env *env,
	fsm_state_t a, fsm_state_t b, fsm_state_t result_id)
{
	assert(a != b);
	assert((a & RESULT_BIT) == 0);
	assert((b & RESULT_BIT) == 0);

	if (a > b) {		/* if necessary, swap to ensure a < b */
		const fsm_state_t tmp = a;
		a = b;
		b = tmp;
	}
	assert(a < b);

	return pair_cache_save(env, RPBT_PAIR, a | RESULT_BIT, b | RESULT_BIT, result_id);
}

static size_t pair_cache_hit, pair_cache_miss;

static int
combine_pair(struct analyze_closures_env *env, fsm_state_t pa, fsm_state_t pb,
	fsm_state_t *result_id)
{
#if LOG_GROUPING
	fprintf(stderr, "%s: pa %d, pb %d\n", __func__, pa, pb);
#endif

	/* If the IDs are for single states, check if they've already been
	 * analyzed. If not, do that and cache it. Either way, switch to the
	 * ID for their cached analysis result. */
	if (0 == (pa & RESULT_BIT)) {
		fsm_state_t cached_result_id;
		if (!analysis_cache_check_single_state(env, pa, &cached_result_id)) {
			if (!cache_single_state_analysis(env, pa, &cached_result_id)) {
				return 0;
			}
		}
		pa = cached_result_id | RESULT_BIT;
	}

	if (0 == (pb & RESULT_BIT)) {
		fsm_state_t cached_result_id;
		if (!analysis_cache_check_single_state(env, pb, &cached_result_id)) {
			if (!cache_single_state_analysis(env, pb, &cached_result_id)) {
				return 0;
			}
		}
		pb = cached_result_id | RESULT_BIT;
	}

	if (analysis_cache_check_pair(env, pa, pb, result_id)) {
#if LOG_GROUPING
		fprintf(stderr, "%s: cache hit for { %d%s %d%s }: %d\n",
		    __func__, ID_WITH_SUFFIX(pa), ID_WITH_SUFFIX(pb), *result_id);
#endif
		pair_cache_hit++;
		return 1;
	} else {
#if LOG_GROUPING
		fprintf(stderr, "%s: cache miss for { %d%s %d%s }\n",
		    __func__, ID_WITH_SUFFIX(pa), ID_WITH_SUFFIX(pb));
#endif
		pair_cache_miss++;
	}

	assert((pa & RESULT_BIT) && (pb & RESULT_BIT));
	pa &=~ RESULT_BIT;
	pb &=~ RESULT_BIT;

	if (!combine_result_pair_and_commit(env, pa, pb, result_id)) {
		return 0;
	}

#if LOG_GROUPING
	fprintf(stderr, "%s: combined and committed as result_id %d\n",
	    __func__, *result_id);
#endif

	if (!analysis_cache_save_pair(env, pa, pb, *result_id)) {
		return 0;
	}

	return 1;
}

#if EXPENSIVE_CHECKS
static void
assert_entry_labels_are_mutually_exclusive(const struct analysis_result *r)
{
	uint64_t seen[4] = { 0 };
	for (size_t e_i = 0; e_i < r->count; e_i++) {
		const struct result_entry *e = &r->entries[e_i];
		const uint64_t *labels = e->labels;
		for (size_t i = 0; i < 4; i++) {
			assert((seen[i] & labels[i]) == 0);
			seen[i] |= labels[i];
		}
	}
}
#endif

static int
cmp_fsm_state_t(const void *pa, const void *pb)
{
	const fsm_state_t a = *(const fsm_state_t *)pa;
	const fsm_state_t b = *(const fsm_state_t *)pb;
	return a < b ? -1 : a > b ? 1 : 0;
}

#define SMALL_INPUT_LIMIT 255
#define SMALL_INPUT_CHECK 0

#ifdef SMALL_INPUT_LIMIT
static void
small_input_sort_and_dedup(fsm_state_t *buf, size_t *pused)
{
	/* Alternate sort_and_dedup_dst_buf implementation for when
	 * the input is small enough to fit in a few cache lines. */

	const size_t orig_used = *pused;
	assert(orig_used > 0 && orig_used <= SMALL_INPUT_LIMIT);
	size_t used = 0;
	fsm_state_t tmp[SMALL_INPUT_LIMIT];

	const fsm_state_t MOVED = (fsm_state_t)-1;

	tmp[used++] = buf[0];
	buf[0] = MOVED;

	/* First, insert unique ascending values into a tmp buffer.
	 * If the input is already mostly sorted, great. */
	for (size_t i = 1; i < orig_used; i++) {
		fsm_state_t cur = buf[i];
		if (cur > tmp[used - 1]) {
			tmp[used++] = cur;
			buf[i] = MOVED;
		}
	}

	/* Then, do insertion sort for entries that haven't already been
	 * moved into tmp. They must go somewhere in the middle, because
	 * entries > the other entries in tmp would have already been
	 * appended in the first pass. */
	for (size_t i = 1; i < orig_used; i++) {
		const fsm_state_t cur = buf[i];
		if (cur == MOVED) { continue; }
		for (size_t j = 0; j < used; j++) {
			const fsm_state_t other = tmp[j];
			if (cur < other) { /* shift the rest down */
				const size_t to_move = used - j;
				memmove(&tmp[j+1], &tmp[j], to_move * sizeof(tmp[j]));
				tmp[j] = cur;
				used++;
				break;
			} else if (cur == other) {
				break;		  /* discard duplicate */
			}
		}
	}

	/* Finally, copy the sorted/dedup'd input back into the buffer. */
	for (size_t i = 0; i < used; i++) {
		buf[i] = tmp[i];
	}
	*pused = used;

	if (SMALL_INPUT_CHECK) {
		for (size_t i = 1; i < used; i++) {
			assert(buf[i - 1] < buf[i]);
		}
	}
}
#endif

static void
sort_and_dedup_dst_buf(fsm_state_t *buf, size_t *used)
{
	const size_t orig_used = *used;

	if (orig_used <= 1) {
		return;		/* no change */
	}

#ifdef SMALL_INPUT_LIMIT
	if (orig_used <= SMALL_INPUT_LIMIT) {
		small_input_sort_and_dedup(buf, used);
		return;
	}
#endif

	/* Figure out what the min and max values are, because
	 * when the difference between them is not too large it
	 * can be significantly faster to avoid qsort here. */
	fsm_state_t min = (fsm_state_t)-1;
	fsm_state_t max = 0;
	fsm_state_t prev;
	int already_sorted_and_unique = 1;

	for (size_t i = 0; i < orig_used; i++) {
		const fsm_state_t cur = buf[i];
		if (cur < min) { min = cur; }
		if (cur > max) { max = cur; }

		if (i > 0) {
			if (cur <= prev) {
				already_sorted_and_unique = 0;
			}
		}
		prev = cur;
	}

	/* If the buffer is already sorted and unique, we're done. */
	if (already_sorted_and_unique) {
		*used = orig_used;
		return;
	}

	/* If there's only one unique value, then we're done. */
	if (min == max) {
		buf[0] = min;
		*used = 1;
		return;
	}

/* 81920 = 10 KB buffer on the stack. This must be divisible by 64.
 * Set to 0 to disable. */
#define QSORT_CUTOFF 81920

	if (QSORT_CUTOFF == 0 || max - min > QSORT_CUTOFF) {
		/* If the bitset would be very large but sparse due to
		 * extreme values, then fall back on using qsort and
		 * then sweeping over the array to squash out
		 * duplicates. */
		qsort(buf, orig_used, sizeof(buf[0]), cmp_fsm_state_t);

		/* squash out duplicates */
		size_t rd = 1;
		size_t wr = 1;
		while (rd < orig_used) {
			if (buf[rd - 1] == buf[rd]) {
				rd++;	/* skip */
			} else {
				buf[wr] = buf[rd];
				rd++;
				wr++;
			}
		}

		*used = wr;
#if EXPENSIVE_CHECKS
		assert(wr <= orig_used);
		for (size_t i = 1; i < *used; i++) {
			assert(buf[i - 1] < buf[i]);
		}
#endif
	} else {
		/* Convert the array into a bitset and back, which sorts
		 * and deduplicates in the process. Add 1 to avoid a zero-
		 * zero-length array error if QSORT_CUTOFF is 0. */
		uint64_t bitset[QSORT_CUTOFF/64 + 1];
		const size_t words = u64bitset_words(max - min + 1);
		memset(bitset, 0x00, words * sizeof(bitset[0]));

		for (size_t i = 0; i < orig_used; i++) {
			u64bitset_set(bitset, buf[i] - min);
		}

		size_t dst = 0;
		for (size_t i = 0; i < words; i++) {
			const uint64_t w = bitset[i];
			if (w != 0) { /* skip empty words */
				uint64_t bit = 0x1;
				for (size_t b_i = 0; b_i < 64; b_i++, bit <<= 1) {
					if (w & bit) {
						buf[dst] = 64*i + b_i + min;
						dst++;
					}
				}
			}
		}
		*used = dst;
	}
}

static int
dst_tmp_buf_append(struct analyze_closures_env *env,
    fsm_state_t id)
{
	if (env->dst_tmp.used == env->dst_tmp.ceil) {
		const size_t nceil = (env->dst_tmp.ceil == 0
		    ? DEF_DST_TMP_CEIL
		    : 2*env->dst_tmp.ceil);
		fsm_state_t *nbuf = f_realloc(env->alloc, env->dst_tmp.buf,
		    nceil * sizeof(nbuf[0]));
		if (nbuf == NULL) {
			return 0;
		}
		env->dst_tmp.buf = nbuf;
		env->dst_tmp.ceil = nceil;
	}
	env->dst_tmp.buf[env->dst_tmp.used] = id;
	env->dst_tmp.used++;
	return 1;
}

static int
combine_result_pair_and_commit(struct analyze_closures_env *env,
    fsm_state_t result_id_a, fsm_state_t result_id_b,
    fsm_state_t *committed_result_id)
{
	INIT_TIMERS();
	TIME(&pre);

	assert(result_id_a < env->results.used);
	assert(result_id_b < env->results.used);
	const struct analysis_result *ra = env->results.rs[result_id_a];
	const struct analysis_result *rb = env->results.rs[result_id_b];

	#if LOG_GROUPING
	fprintf(stderr, "\n");
	fprintf(stderr, "%s: result_id a %d, %u entries\n", __func__, result_id_a, ra->count);
	dump_group(stderr, env, ra);
	fprintf(stderr, "%s: result_id b %d, %u entries\n", __func__, result_id_b, rb->count);
	dump_group(stderr, env, rb);
	#endif

	#if EXPENSIVE_CHECKS
	assert_entry_labels_are_mutually_exclusive(ra);
	assert_entry_labels_are_mutually_exclusive(rb);
	#endif

	/* For whichever group is smaller (has less entries), find every overlap
	 * it has with the other, and then emit all remaning from the other as-is. */
	const struct analysis_result *rl = (ra->count <= rb->count ? ra : rb);
	const struct analysis_result *rr = (ra->count > rb->count ? ra : rb);
	assert(rl != rr);

	env->results.buffer.used = 0;
	env->dst_tmp.used = 0;

	uint64_t checked[256/64] = { 0 };
	#if LOG_GROUPING > 2
	fprintf(stderr, "%s: rl %d, rr %d\n",
	    __func__,
	    rl == ra ? result_id_a : result_id_b,
	    rl == rb ? result_id_b : result_id_a);
#endif

	size_t rl_i = 0;
	while (rl_i < rl->count) {
		uint64_t labels[256/64];

		const struct result_entry *el = &rl->entries[rl_i];
		uint8_t wu = 0;
		for (size_t w_i = 0; w_i < 4; w_i++) {
			labels[w_i] = el->labels[w_i] &~ checked[w_i];
			if (labels[w_i] != 0) { wu |= ((uint8_t)1 << w_i); }
		}
		if (wu == 0) {
			rl_i++;
			continue;
		}

		/* copy all to-states from the left group's entry */
		{
			assert(el->to_set_offset + el->to_set_count <= env->to_sets.used);
			const fsm_state_t *e_to = &env->to_sets.buf[el->to_set_offset];
			for (size_t i = 0; i < el->to_set_count; i++) {
				if (!dst_tmp_buf_append(env, e_to[i])) {
					return 0;
				}
			}
		}

		/* intersect with any overlapping entries on the right group */
		for (size_t rr_i = 0; rr_i < rr->count; rr_i++) {
			const struct result_entry *er = &rr->entries[rr_i];
			assert(er->words_used != 0);
			if (er->words_used & wu) {
				int found_any = 0; /* check that wu remains accurate */
				for (size_t w_i = 0; w_i < 4; w_i++) {
					if (labels[w_i] & er->labels[w_i]) {
						found_any = 1;
						break;
					}
				}
				if (found_any) {
					for (size_t w_i = 0; w_i < 4; w_i++) {
						labels[w_i] &= er->labels[w_i];
						if (labels[w_i] == 0) {
							wu &=~ ((uint8_t)1 << w_i);
						}
					}

					assert(er->to_set_offset + er->to_set_count <= env->to_sets.used);
					const fsm_state_t *e_to = &env->to_sets.buf[er->to_set_offset];
					for (size_t i = 0; i < er->to_set_count; i++) {
						if (!dst_tmp_buf_append(env, e_to[i])) {
							return 0;
						}
					}
				}
			}
		}

		assert(labels[0] || labels[1] || labels[2] || labels[3]);
		sort_and_dedup_dst_buf(env->dst_tmp.buf, &env->dst_tmp.used);

		if (!add_group_dst_info_to_buffer(env,
			env->dst_tmp.used, env->dst_tmp.buf,
			labels)) {
			return 0;
		}
		env->dst_tmp.used = 0;

		for (size_t w_i = 0; w_i < 4; w_i++) {
			checked[w_i] |= labels[w_i];
		}
	}

	/* second pass, emit any unchecked labels in rr */
	for (size_t rr_i = 0; rr_i < rr->count; rr_i++) {
		const struct result_entry *er = &rr->entries[rr_i];
		uint64_t labels[256/64];
		memcpy(labels, er->labels, sizeof(labels));

		/* check for intersection with remaining unchecked labels */
		int any_labels_remaining = 0;
		for (size_t w_i = 0; w_i < 4; w_i++) {
			/* todo: could negate checked above to avoid ~ here */
			if (labels[w_i] & ~checked[w_i]) {
				any_labels_remaining = 1;
				break;
			}
		}

		/* Add those labels and their to-states to the group
		 * buffer, since rr's entries' labels do not overlap we
		 * can add it immediately and do not need to update
		 * checked[]. */
		if (any_labels_remaining) {
			for (size_t w_i = 0; w_i < 4; w_i++) {
				labels[w_i] &=~ checked[w_i];
			}
			assert(er->to_set_offset + er->to_set_count <= env->to_sets.used);

			const fsm_state_t *e_to = &env->to_sets.buf[er->to_set_offset];

			for (size_t i = 0; i < er->to_set_count; i++) {
				if (!dst_tmp_buf_append(env, e_to[i])) {
					return 0;
				}
			}

			if (!add_group_dst_info_to_buffer(env,
				env->dst_tmp.used, env->dst_tmp.buf,
				labels)) {
				return 0;
			}
			env->dst_tmp.used = 0;
		}
	}

	if (!commit_buffered_result(env, committed_result_id)) {
		return 0;
	}

	#if LOG_GROUPING
	const struct analysis_result *rz = env->results.rs[*committed_result_id];
	fprintf(stderr, "%s: result, %u entries, committed as %d\n",
	    __func__, rz->count, *committed_result_id);
	dump_group(stderr, env, rz);
	#endif

	TIME(&post);
	DIFF_MSEC(__func__, pre, post, &env->usec_pair);
	env->count_pair++;

	return 1;
}

static int
analyze_closures__save_output(struct analyze_closures_env *env,
    const uint64_t labels[256/64], interned_state_set_id iss)
{
	if (env->output_count + 1 >= env->output_ceil) {
		if (!analyze_closures__grow_outputs(env)) {
			return 0;
		}
	}

	struct ac_output *dst = &env->outputs[env->output_count];
	memcpy(dst->labels, labels, sizeof(dst->labels));
	dst->iss = iss;

#if LOG_AC
	fprintf(stderr, "%s: output %zu: labels [", __func__, env->output_count);
	dump_labels(stderr, labels);
	fprintf(stderr, "] -> iss:%ld\n", iss);
#endif

	env->output_count++;
	return 1;
}

static int
analyze_closures__grow_groups(struct analyze_closures_env *env)
{
	const size_t nceil = 2*env->group_ceil;
	struct ac_group *ngs = f_realloc(env->alloc,
	    env->groups, nceil * sizeof(env->groups[0]));
	if (ngs == NULL) {
		return 0;
	}

#if LOG_AC
	fprintf(stderr, "ac_grow_groups: growing groups %zu -> %zu\n",
	    env->group_ceil, nceil);
#endif

	env->groups = ngs;
	env->group_ceil = nceil;
	return 1;
}

static int
analyze_closures__grow_dst(struct analyze_closures_env *env)
{
	const size_t nceil = (env->dst_ceil == 0
	    ? DEF_DST_CEIL : 2*env->dst_ceil);
	fsm_state_t *ndst = f_realloc(env->alloc,
	    env->dst, nceil * sizeof(env->dst[0]));
	if (ndst == NULL) {
		return 0;
	}

#if LOG_AC
	fprintf(stderr, "ac_grow_dst: growing dst %zu -> %zu\n",
	    env->dst_ceil, nceil);
#endif

	env->dst = ndst;
	env->dst_ceil = nceil;
	return 1;

}

static int
analyze_closures__grow_outputs(struct analyze_closures_env *env)
{
	const size_t nceil = (env->output_ceil == 0
	    ? DEF_OUTPUT_CEIL : 2*env->output_ceil);
	struct ac_output *nos = f_realloc(env->alloc,
	    env->outputs, nceil * sizeof(env->outputs[0]));
	if (nos == NULL) {
		return 0;
	}

#if LOG_AC
	fprintf(stderr, "ac_grow_outputs: growing outputs %zu -> %zu\n",
	    env->output_ceil, nceil);
#endif

	env->outputs = nos;
	env->output_ceil = nceil;
	return 1;
}
