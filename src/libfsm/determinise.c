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
		if (labels[i/64] & ((uint64_t)1 << (i&63))) {
			fprintf(f, "%c", (char)(isprint(i) ? i : '.'));
		}
	}
}

#define D(N) static size_t N;
#include "stats.h"
#undef D

static void
reset_stats(void)
{
#define D(N) N = 0
#include "stats.h"
#undef D
}

static void
dump_stats(void)
{
#define D(N) fprintf(stderr, "^^^ %s: %zu\n", #N, N)
#include "stats.h"
#undef D
}

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

	struct edge_group_mapping egm[MAX_EGM] = { 0 };

#if PROCESS_AS_GROUP
	struct analyze_closures_env ac_env = { 0 };
#endif

	assert(nfa != NULL);
	map.alloc = nfa->opt->alloc;

	reset_stats();

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

#if PROCESS_AS_GROUP
	ac_env.alloc = nfa->opt->alloc;
	ac_env.fsm = nfa;
	ac_env.issp = issp;
	ac_env.egm = egm;
#endif

	do {
		num_stack_steps++;
#if LOG_SYMBOL_CLOSURE
		fprintf(stderr, "\nfsm_determinise: current dfacount %lu...\n",
		    dfacount);
#endif


#if PROCESS_AS_GROUP
		assert(curr != NULL);

		if (!analyze_closures_for_iss(&ac_env, curr->iss)) {
			goto cleanup;
		}

		int o_i;
		for (o_i = 0; o_i < ac_env.output_count; o_i++) {
			struct mapping *m;
			struct ac_output *output = &ac_env.outputs[o_i];
			struct interned_state_set *iss = output->iss;

#if LOG_DETERMINISE_CLOSURES
			fprintf(stderr, "fsm_determinise: cur (dfa %zu) label [", curr->dfastate);
			dump_labels(stderr, output->labels);
			fprintf(stderr, " -> iss:%p: ", output->iss);
			{
				struct state_iter it;
				fsm_state_t s;
				struct state_set *ss = interned_state_set_retain(output->iss);

				for (state_set_reset(ss, &it); state_set_next(&it, &s); ) {
					fprintf(stderr, " %u", s);
				}
				fprintf(stderr, "\n");
				interned_state_set_release(output->iss);
			}
#endif

			/*
			 * The set of NFA states output->iss represents a single DFA state.
			 * We use the mappings as a de-duplication mechanism, keyed by the
			 * set of NFA states.
			 */

			/* If this interned_state_set isn't present, then save it as a new mapping. */
			/* this is the set of states the current closure ----i---> dst set */
			if (LOG_SYMBOL_CLOSURE) { fprintf(stderr, "MAP_FIND: checking for iss %p\n", (void *)iss); }
			if (map_find(&map, iss, &m)) {
				/* we already have this closure interned, so reuse it */
				assert(m->dfastate < dfacount);
				if (LOG_SYMBOL_CLOSURE) { fprintf(stderr, "MAP_FIND: loaded m->dfastate %zu\n", m->dfastate); }
			} else {
				/* not found -- add a new one and push it to the stack for processing */
				if (LOG_SYMBOL_CLOSURE) { fprintf(stderr, "MAP_FIND: not found -> dfacount %zu\n", dfacount); }
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

			if (!edge_set_add_bulk(&curr->edges, nfa->opt->alloc, output->labels, m->dfastate)) {
				goto cleanup;
			}
		}

#else
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
			size_t set_count = 0;
			struct state_set *ss = interned_state_set_retain(curr->iss);

			/* count how many states are in this closure */
			for (state_set_reset(ss, &it); state_set_next(&it, &s); ) {
				set_count++;
			}

			/* for every state in this closure, get the unique sets of labels;
			 * we need to check that this set is unique across all the states here. right? FIXME */

			for (state_set_reset(ss, &it); state_set_next(&it, &s); ) {
#if LOG_SYMBOL_CLOSURE
				fprintf(stderr, "   -- FIXME: other stuff during iscwe: s %d, curr->dfastate %zu, set_count %zu\n", s, curr->dfastate, set_count);
#endif

				/* For each edge group, save the set of states reachable via those labels in sclosures[label] */
 				if (!interned_symbol_closure_without_epsilons(nfa, s, issp, sclosures,
					curr->dfastate)) {
					goto cleanup;
				}
			}
			interned_state_set_release(curr->iss);
		}

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			struct mapping *m;
			fsm_state_t egm_load_state;
			fsm_state_t egm_to_state;

			if (sclosures[i] == NULL) {
				continue;
			}

			num_closures_processed++;

#if LOG_DETERMINISE_CLOSURES
			fprintf(stderr, "fsm_determinise: cur (dfa %zu) label '%c': %p:",
			    curr->dfastate, (char)i, (void *)sclosures[i]);
			{
				struct state_iter it;
				fsm_state_t s;
				struct state_set *ss = interned_state_set_retain(sclosures[i]);

				/* TODO: this does not log associated group mapping */

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
			/* this is the set of states the current closure ----i---> dst set */
			if (map_find(&map, sclosures[i], &m)) {
				/* we already have this closure interned */
				assert(m->dfastate < dfacount);
				egm_to_state = m->dfastate;

				if (LOG_SYMBOL_CLOSURE) { fprintf(stderr, "MAP_FIND: loaded m->dfastate %d\n", egm_to_state); }
			} else {
				/* not found -- add a new one */
				egm_to_state = dfacount;
				if (LOG_SYMBOL_CLOSURE) { fprintf(stderr, "MAP_FIND: not found -> dfacount %d\n", egm_to_state); }
				if (!map_add(&map, dfacount, sclosures[i], &m)) {
					goto cleanup;
				}
				dfacount++;
				if (!stack_push(stack, m)) {
					goto cleanup;
				}
			}

			{
				assert(m != NULL);
				if (!edge_set_add(&curr->edges, nfa->opt->alloc, i, /* to */ m->dfastate)) {
					goto cleanup;
				}
			}
		}
#endif

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
#if LOG_AC
		dump_stats();
#endif
	}

	res = 1;

#if 0
	/* expensive invariant check */
	assert(fsm_all(nfa, fsm_isdfa));
#endif

cleanup:
	map_free(&map);
	stack_free(stack);
	interned_state_set_pool_free(issp);

#if PROCESS_AS_GROUP
	if (ac_env.iters != NULL) {
		f_free(ac_env.alloc, ac_env.iters);
	}
#endif

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
next_edge_label(const uint64_t *symbols,
    unsigned char *current, unsigned char *label)
{
	size_t i;
	if (current == NULL) {
		i = 0;
	} else if (*current == UCHAR_MAX) {
		return 0;
	} else {
		i = *current + 1;
	}

	while (i < 256) {
		if ((i & 63) == 0 && symbols[i/64] == 0) {
			i += 64;
		} else {
			if (symbols[i/64] & ((size_t)1 << (i & 63))) {
				*label = (unsigned char)i;
				return 1;
			}
			i++;
		}
	}

	return 0;
}

static int
first_edge_label(const struct edge_group_iter_info *g,
    unsigned char *label)
{
	return next_edge_label(g->symbols, NULL, label);
}

static size_t
popcount_labels(const uint64_t labels[static 4])
{
	size_t i, res = 0;
	for (i = 0; i < 4; i++) {
		res += __builtin_popcountll(labels[i]);
	}
	return res;
}

static size_t
popcount_edge_labels(const struct edge_group_iter_info *g)
{
	return popcount_labels(g->symbols);
}

static int
interned_symbol_closure_without_epsilons(const struct fsm *fsm, fsm_state_t s,
	struct interned_state_set_pool *issp,
	struct interned_state_set *sclosures[static FSM_SIGMA_COUNT], size_t dfa_state)
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
		} else {

			if (LOG_SYMBOL_CLOSURE) { fprintf(stderr, "ISS not_empty -- line %d\n", __LINE__); }
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
	fsm_state_t dfastate, struct interned_state_set *iss, struct mapping **new_mapping)
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

#if LOG_AC
static void
dump_egi_info(size_t i, const struct edge_group_iter_info *info) {
	if (info->to == AC_NO_STATE) {
		fprintf(stderr, "%zu: DONE\n", i);
		return;
	}
	fprintf(stderr, "%zu: unique %d, to %d, symbols: [0x%lx, 0x%lx, 0x%lx, 0x%lx] -- ",
	    i, info->unique, info->to,
	    info->symbols[0], info->symbols[1],
	    info->symbols[2], info->symbols[3]);
	dump_labels(stderr, info->symbols);
	fprintf(stderr, "\n");
}
#endif

static int
labels_overlap(const uint64_t *a, const uint64_t *b)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		if (a[i] & b[i]) {
			return 1;
		}
	}
	return 0;
}

static void
intersect_with(uint64_t *a, const uint64_t *b)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		a[i] &= b[i];
	}
}

static void
union_with(uint64_t *a, const uint64_t *b)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		a[i] |= b[i];
	}
}

static void
clear_labels(uint64_t *a, const uint64_t *b)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		a[i] &=~ b[i];
	}
}

static int
all_cleared(const uint64_t *a)
{
	size_t i;
	for (i = 0; i < 4; i++) {
		if (a[i] != 0) {
			return 0;
		}
	}
	return 1;
}

/* TODO:
 * - the looping in Collect isn't quite right, and it may actually
 *   be *less* complicated to set up a freelist and collect as long
 *   as the to state is the same, the e_i stuff is gnarly
 * - needs a test input with e.g. abcdef->3, cde->4
 * - needs a test input with a bunch of single-label edges
 * - then let theft gnaw on this */
static int
analyze_closures_for_iss(struct analyze_closures_env *env,
    struct interned_state_set *curr_iss)
{
	int res = 0;

	struct state_set *ss = interned_state_set_retain(curr_iss);
	const size_t set_count = state_set_count(ss);

	assert(env != NULL);
	assert(set_count > 0);

	if (!analyze_closures__init_iterators(env, ss, set_count)) {
		goto cleanup;
	}

	env->output_count = 0;

	switch (analyze_closures__collect(env)) {
	case AC_COLLECT_DONE:
		if (!analyze_closures__analyze(env)) {
			goto cleanup;
		}
		break;
	case AC_COLLECT_EMPTY:
		/* no analysis to do */
		break;
	default:
	case AC_COLLECT_ERROR:
		goto cleanup;
	}

	res = 1;

cleanup:
	interned_state_set_release(curr_iss);
	return res;

}

static int
analyze_closures__init_iterators(struct analyze_closures_env *env,
	const struct state_set *ss, size_t set_count)
{
	struct state_iter it;
	fsm_state_t s;
	size_t i_i;

#if LOG_AC
	fprintf(stderr, "ac_init: ceil %zu, count %zu\n",
	    env->iter_ceil, set_count);
#endif

	/* Grow backing array for iterators on demand */
	if (env->iter_ceil < set_count) {
		if (!analyze_closures__grow_iters(env, set_count)) {
			return 0;
		}
	}

	/* Init all the edge group iterators so we can step them in
	 * parallel and merge. Each will yield edge groups in order,
	 * sorted by .to, so we can merge them that way. */
	i_i = 0;
	state_set_reset(ss, &it);
#if LOG_AC
	fprintf(stderr, "ac_init: initializing iterators:");
#endif

	while (state_set_next(&it, &s)) {
		/* The edge set group iterator can partition them into
		 * unique (within the edge set) and non-unique label
		 * sets, but what we really care about is labels that
		 * are unique within the state set, so that doesn't
		 * actually help us much. */
#if LOG_AC
		fprintf(stderr, " %d", s);
#endif
		edge_set_group_iter_reset(env->fsm->states[s].edges,
		    EDGE_GROUP_ITER_ALL, &env->iters[i_i].iter);
		i_i++;
	}

#if LOG_AC
	fprintf(stderr, "\n");
#endif
	env->iter_count = set_count;

	return 1;
}

static enum ac_collect_res
analyze_closures__collect(struct analyze_closures_env *env)
{
	/* Step all the iterators once, and then keep stepping whichever
	 * is earliest (by .to state) until they're all done. Collect
	 * (label set -> state) info along the way.
	 *
	 * This could use a freelist or something
	 * later, rather than running to a fixpoint. */
	fsm_state_t e_to = AC_NO_STATE;
	size_t e_i = AC_NO_STATE; /* i with earliest to */
	size_t i_i;

	/* First pass, step everything once and save their info. */
	for (i_i = 0; i_i < env->iter_count; i_i++) {
		struct ac_iter *egi = &env->iters[i_i];

#if LOG_AC
		egi->info.unique = 0; /* deterministically init for logging  */
#endif

		if (edge_set_group_iter_next(&egi->iter, &egi->info)) {
#if LOG_AC
			fprintf(stderr, "ac_collect: iter[%zu]: to: %d\n",
			    i_i, egi->info.to);
			dump_egi_info(i_i, &egi->info);
#endif
			if (e_to == AC_NO_STATE || egi->info.to < e_to) {
				e_to = egi->info.to;
				e_i = i_i;
			}
		} else {
#if LOG_AC
			fprintf(stderr, "ac_collect: iter[%zu]: DONE\n", i_i);
#endif
			egi->info.to = AC_NO_STATE; /* done */
			assert(e_i != i_i);
		}
	}

#if LOG_AC
	fprintf(stderr, "ac_collect: post-init e_i %zu, e_to %d\n", e_i, e_to);
#endif

	if (e_to == AC_NO_STATE) { /* empty */
		return AC_COLLECT_EMPTY;
	}

	/* If we're reusing a pre-allocated group array, reinitialize
	 * the first group; otherwise it's done after allocation. */
	if (env->group_ceil > 0) {
		env->group_count = 0;
		memset(&env->groups[0], 0x00, sizeof(env->groups[0]));
		env->groups[0].to = e_to;
	}

	int progress;
	size_t steps = 0;
	do {
		progress = 0;
		steps++;

		if (env->group_count + 1 >= env->group_ceil) { /* grow on demand */
			if (!analyze_closures__grow_groups(env)) {
				return AC_COLLECT_ERROR;
			}
			if (env->group_count == 0) { /* init first group */
				env->groups[0].to = e_to;
			}
		}

		/* find iterator with earliest .to, to step later */
		e_to = AC_NO_STATE;
		for (i_i = 0; i_i < env->iter_count; i_i++) {
			struct ac_iter *egi = &env->iters[i_i];
#if LOG_AC
			fprintf(stderr, "ac_collect: egi[%zu/%zu]: to %d\n", i_i, env->iter_count, egi->info.to);
#endif

			if (egi->info.to == AC_NO_STATE) {
				if (e_i == i_i) {
					e_i = AC_NO_STATE;
				}
				continue; /* done */
			}

			/* pick first/earliest iterator */
			if (e_to == AC_NO_STATE
			    || egi->info.to < e_to
			    || (egi->info.to == e_to
				&& i_i < e_i)) {
				e_i = i_i;
				e_to = egi->info.to;
				progress = 1;
			}
		}

#if LOG_AC
		fprintf(stderr, "ac_collect: earliest e_to: %d at e_i: %zu, group_count %zu\n",
		    e_to, e_i, env->group_count);
#endif

		/* collect from all iterators with current .to */
		/* FIXME this is currently run multiple times but should be harmless */
		if (e_i != AC_NO_STATE) {
			struct ac_group *g = &env->groups[env->group_count];
			for (i_i = 0; i_i < env->iter_count; i_i++) {
				struct ac_iter *egi = &env->iters[i_i];
				if (egi->info.to == g->to) {
#if LOG_AC
					fprintf(stderr, "ac_collect: unioning labels from egi->to: %d\n", egi->info.to);
#endif
					union_with(g->labels, egi->info.symbols);
				}
			}
		}

#if LOG_AC
		fprintf(stderr, "ac_collect: fixpoint: step %zu: e_i %zd, e_to %d\n",
		    steps, e_i, e_to);
#endif
		if (e_i != AC_NO_STATE) {
			struct ac_iter *egi = &env->iters[e_i];
			if (edge_set_group_iter_next(&egi->iter, &egi->info)) {
				struct ac_group *g = &env->groups[env->group_count];

#if LOG_AC
				fprintf(stderr, "ac_collect: collecting for group with g->to: %d, e_i got %d\n",
				    g->to, egi->info.to);
#endif

				e_to = egi->info.to;
				if (e_to != g->to) {
					assert(e_to > g->to); /* ascending */
					env->group_count++;
					assert(env->group_count < env->group_ceil);
					struct ac_group *ng = &env->groups[env->group_count];
					memset(ng, 0x00, sizeof(*ng));
					ng->to = e_to;
				}
			} else {
				egi->info.to = AC_NO_STATE; /* done */
			}
			progress = 1;
#if LOG_AC
			dump_egi_info(e_i, &egi->info);
#endif
		}
	} while (progress);

	env->group_count++;	/* always at least one */
	return AC_COLLECT_DONE;
}

static int
analyze_closures__analyze(struct analyze_closures_env *env)
{
#if LOG_AC
	/* Dump. */
	{
		size_t g_i;
		fprintf(stderr, "# group label/to closure table\n");
		for (g_i = 0; g_i < env->group_count; g_i++) {
			const struct ac_group *g = &env->groups[g_i];
			fprintf(stderr,
			    "g[%zu]: to %d: [0x%lx, 0x%lx, 0x%lx, 0x%lx] -- ",
			    g_i, g->to,
			    g->labels[0], g->labels[1],
			    g->labels[2], g->labels[3]);
			dump_labels(stderr, g->labels);
			fprintf(stderr, "\n");
		}
	}
#endif

	/* for every group, find labels that are unique to that
	 * group, then create that edge group and clear those.
	 * repeat and collect shared groups, always shrinking
	 * the current group. advance when the base has no labels
	 * remaining. this should eventually terminate. */
	size_t base_i, g_i, o_i; /* base_i, group_i, other_i */

	size_t dst_count;
#define MAX_DST 100 /* fixme: make dynamic */
	fsm_state_t dst[MAX_DST];

	base_i = 0;
	env->output_count = 0;

	while (base_i < env->group_count) {
		/* labels assigned in current sweep */
		uint64_t labels[256/64];
		dst_count = 0;

#if LOG_AC
		fprintf(stderr, "base_i %zu/%zu\n",
		    base_i, env->group_count);
#endif

		const struct ac_group *bg = &env->groups[base_i];
		memcpy(labels, bg->labels, sizeof(bg->labels));
		/* at least one bit should be set, otherwise
		 * we should have incremented base_i */
		assert(labels[0] || labels[1]
		    || labels[2] || labels[3]);

#if LOG_AC
		fprintf(stderr, "ac_analyze: dst[%zu] <- %d (base)\n", dst_count, bg->to);
#endif
		dst[dst_count] = bg->to;
		dst_count++;

		for (o_i = base_i + 1; o_i < env->group_count; o_i++) {
			const struct ac_group *og = &env->groups[o_i];
			if (labels_overlap(labels, og->labels)) {
				intersect_with(labels, og->labels);
#if LOG_AC
				fprintf(stderr, "ac_analyze: dst[%zu] <- %d (other w/ overlapping labels)\n", dst_count, og->to);
#endif
				dst[dst_count] = og->to;
				dst_count++;
				assert(dst_count < MAX_DST);
			}
		}

#if LOG_AC
		fprintf(stderr, "ac_analyze: dst_count: %zu\n", dst_count);
#endif
		if (dst_count == 1) {
			/* special case: if there's only one dst
			 * state saved, it must have come from
			 * the base group, so don't waste time
			 * scanning over everything else. */
			struct ac_group *g = &env->groups[base_i];
			clear_labels(g->labels, labels);
#if LOG_AC
			fprintf(stderr, "ac_analyze: cleared base labels, now: ");
			dump_labels(stderr, g->labels);
			fprintf(stderr, "\n");
#endif
		} else {
			for (g_i = base_i; g_i < env->group_count; g_i++) {
				struct ac_group *g = &env->groups[g_i];
				clear_labels(g->labels, labels);
#if LOG_AC
				fprintf(stderr, "ac_analyze: cleared g[%zu] labels, now: ", g_i);
				dump_labels(stderr, g->labels);
				fprintf(stderr, "\n");
#endif
			}
		}

		if (LOG_AC) {
			size_t i;
			fprintf(stderr, "new_edge_group:");
			for (i = 0; i < dst_count; i++) {
				fprintf(stderr, " %d", dst[i]);
			}
			fprintf(stderr, " -- ");
			dump_labels(stderr, labels);
			fprintf(stderr, "\n");
		}

		{
			struct interned_state_set *iss = interned_state_set_empty(env->issp);
			unsigned char first;
			size_t d_i, label_count;
			for (d_i = 0; d_i < dst_count; d_i++) {
				struct interned_state_set *updated;
#if LOG_AC
				fprintf(stderr, "ac_analyze: adding state %d to interned_state_set\n", dst[d_i]);
#endif
				updated = interned_state_set_add(env->issp,
				    iss, dst[d_i]);
				if (updated == NULL) {
					return 0;
				}
				iss = updated;
			}

			if (!analyze_closures__save_output(env, labels, iss)) {
				return 0;
			}
		}

		while (base_i < env->group_count &&
		    all_cleared(env->groups[base_i].labels)) {
#if LOG_AC
			fprintf(stderr, "ac_analyze: base %zu all clear, advancing\n", base_i);
#endif
			base_i++;
		}
	}

#if LOG_AC
	fprintf(stderr, "ac_analyze: done\n");
#endif
	return 1;
}

static int
analyze_closures__save_output(struct analyze_closures_env *env,
    const uint64_t labels[256/4], struct interned_state_set *iss)
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
	fprintf(stderr, "ac_save_output: labels [");
	dump_labels(stderr, labels);
	fprintf(stderr, "] -> iss:%p\n", (void *)iss);
#endif

	env->output_count++;
	return 1;
}

static int
analyze_closures__grow_iters(struct analyze_closures_env *env,
    size_t set_count)
{
	size_t nceil = (env->iter_ceil == 0
	    ? DEF_ITER_CEIL : env->iter_ceil);
	while (nceil < set_count) {
		assert(nceil > 0);
		nceil *= 2;
	}

#if LOG_AC
	fprintf(stderr, "ac_init: growing iters to %zu\n", nceil);
#endif

	struct ac_iter *niters = f_realloc(env->alloc,
	    env->iters, nceil * sizeof(env->iters[0]));
	if (niters == NULL) {
		return 0;
	}
	env->iters = niters;
	env->iter_ceil = nceil;
	return 1;
}

static int
analyze_closures__grow_groups(struct analyze_closures_env *env)
{
	const size_t nceil = (env->group_ceil == 0
	    ? DEF_GROUP_CEIL : 2*env->group_ceil);
	struct ac_group *ngs = f_realloc(env->alloc,
	    env->groups, nceil * sizeof(env->groups[0]));
	if (ngs == NULL) {
		return 0;
	}

#if LOG_AC
	fprintf(stderr, "ac_grow_groups: growing groups %zu -> %zu\n",
	    env->group_ceil, nceil);
#endif

	/* FIXME don't do this here */
	if (env->group_count == 0) {
		/* Zero the first one; the others will
		 * be zeroed as env->group_count advances. */
		memset(&ngs[0], 0x00, sizeof(ngs[0]));
	}

	env->groups = ngs;
	env->group_ceil = nceil;
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
