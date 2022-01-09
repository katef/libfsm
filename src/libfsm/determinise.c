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

int
fsm_determinise(struct fsm *nfa)
{
	int res = 0;
	struct mappingstack *stack = NULL;

	struct interned_state_set_pool *issp = NULL;
	struct map map = { NULL, 0, 0, NULL };
	struct mapping *curr = NULL;
	size_t dfacount = 0;

	struct analyze_closures_env ac_env = { 0 };

	assert(nfa != NULL);
	map.alloc = nfa->opt->alloc;

	/*
	 * This NFA->DFA implementation is for Glushkov NFA only; it keeps things
	 * a little simpler by avoiding epsilon closures here, and also a little
	 * faster where we can start with a Glushkov NFA in the first place.
	 */
	if (fsm_has(nfa, fsm_hasepsilons)) {
		if (!fsm_remove_epsilons(nfa)) {
			return 0;
		}
	}

#if LOG_DETERMINISE_CAPTURES
	fprintf(stderr, "# post_remove_epsilons\n");
	fsm_print_fsm(stderr, nfa);
	fsm_capture_dump(stderr, "#### post_remove_epsilons", nfa);
#endif

	issp = interned_state_set_pool_alloc(nfa->opt->alloc);
	if (issp == NULL) {
		return 0;
	}

	{
		fsm_state_t start;
		interned_state_set_id start_set;
		interned_state_set_id empty = interned_state_set_empty(issp);

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
			res = 1;
			goto cleanup;
		}

#if LOG_DETERMINISE_CAPTURES
		fprintf(stderr, "#### Adding mapping for start state %u -> 0\n", start);
#endif
		if (!interned_state_set_add(issp, &empty, start, &start_set)) {
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

	ac_env.alloc = nfa->opt->alloc;
	ac_env.fsm = nfa;
	ac_env.issp = issp;

	do {
		size_t o_i;

#if LOG_SYMBOL_CLOSURE
		fprintf(stderr, "\nfsm_determinise: current dfacount %lu...\n",
		    dfacount);
#endif

		assert(curr != NULL);

		if (!analyze_closures_for_iss(&ac_env, curr->iss)) {
			goto cleanup;
		}

		if (!edge_set_advise_growth(&curr->edges, nfa->opt->alloc, ac_env.output_count)) {
			goto cleanup;
		}

		for (o_i = 0; o_i < ac_env.output_count; o_i++) {
			struct mapping *m;
			struct ac_output *output = &ac_env.outputs[o_i];
			interned_state_set_id iss = output->iss;

#if LOG_DETERMINISE_CLOSURES
			fprintf(stderr, "fsm_determinise: cur (dfa %zu) label [", curr->dfastate);
			dump_labels(stderr, output->labels);
			fprintf(stderr, "] -> iss:%p: ", output->iss);
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

			/* If this interned_state_set isn't present, then save it as a new mapping.
			 * This is the interned set of states that the current ac_output (set of
			 * labels) leads to. */
			if (map_find(&map, iss, &m)) {
				/* we already have this closure interned, so reuse it */
				assert(m->dfastate < dfacount);
			} else {
				/* not found -- add a new one and push it to the stack for processing */
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
			interned_state_set_id iss_id = m->iss;
			assert(m->dfastate < dfa->statecount);
			assert(dfa->states[m->dfastate].edges == NULL);

			dfa->states[m->dfastate].edges = m->edges;

			/*
			 * The current DFA state is an end state if any of its associated NFA
			 * states are end states.
			 */

			ss = interned_state_set_retain(issp, iss_id);
			if (!state_set_has(nfa, ss, fsm_isend)) {
				interned_state_set_release(issp, &iss_id);
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
			interned_state_set_release(issp, &iss_id);
		}

		if (!remap_capture_actions(&map, issp, dfa, nfa)) {
			goto cleanup;
		}

		fsm_move(nfa, dfa);
	}

	res = 1;

cleanup:
	map_free(&map);
	stack_free(stack);
	interned_state_set_pool_free(issp);

	if (ac_env.iters != NULL) {
		f_free(ac_env.alloc, ac_env.iters);
	}
	if (ac_env.groups != NULL) {
		f_free(ac_env.alloc, ac_env.groups);
	}
	if (ac_env.outputs != NULL) {
		f_free(ac_env.alloc, ac_env.outputs);
	}
	if (ac_env.dst != NULL) {
		f_free(ac_env.alloc, ac_env.dst);
	}

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

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static unsigned long
hash_iss(interned_state_set_id iss)
{
	/* Just hashing the ID directly is fine here -- since they're
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
	fsm_state_t dfastate, interned_state_set_id iss, struct mapping **new_mapping)
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
map_find(const struct map *map, interned_state_set_id iss,
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
	reverse_mappings = f_calloc(dst_dfa->opt->alloc, src_nfa->statecount, sizeof(reverse_mappings[0]));
	if (reverse_mappings == NULL) {
		return 0;
	}

	/* build reverse mappings table: for every NFA state X, if X is part
	 * of the new DFA state Y, then add Y to a list for X */
	for (m = map_first(map, &it); m != NULL; m = map_next(&it)) {
		struct state_set *ss;
		interned_state_set_id iss_id = m->iss;
		assert(m->dfastate < dst_dfa->statecount);
		ss = interned_state_set_retain(issp, iss_id);

		for (state_set_reset(ss, &si); state_set_next(&si, &state); ) {
			if (!add_reverse_mapping(dst_dfa->opt->alloc,
				reverse_mappings,
				m->dfastate, state)) {
				goto cleanup;
			}
		}
		interned_state_set_release(issp, &iss_id);
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

#define TRACK_TIMES 0

#if TRACK_TIMES
#include <sys/time.h>
#define INIT_TIMERS() struct timeval pre, post
#define TIME(T) if (gettimeofday(T, NULL) == -1) { assert(!"gettimeofday"); }
#define DIFF_MSEC(LABEL, PRE, POST, ACCUM)				\
	do {								\
		const size_t diff = 1000000*(POST.tv_sec - PRE.tv_sec)	\
		    + (POST.tv_usec - PRE.tv_usec);			\
		if (diff > 100) {					\
			fprintf(stderr, "%s: %zu%s\n", LABEL, diff,	\
			    diff >= 100 ? " #### OVER 100" : "");	\
		}							\
		if (ACCUM != NULL) {					\
			*ACCUM += diff;					\
		}							\
	} while(0)
#else
#define INIT_TIMERS()
#define TIME(T)
#define DIFF_MSEC(A, B, C, D)
#endif

static int
analyze_closures_for_iss(struct analyze_closures_env *env,
    interned_state_set_id cur_iss)
{
	int res = 0;

	/* Save the ID in a local variable, because release
	 * below needs to overwrite the reference. */
	interned_state_set_id iss_id = cur_iss;

	struct state_set *ss = interned_state_set_retain(env->issp, iss_id);
	const size_t set_count = state_set_count(ss);

	INIT_TIMERS();

	assert(env != NULL);
	assert(set_count > 0);

	TIME(&pre);
	if (!analyze_closures__init_iterators(env, ss, set_count)) {
		goto cleanup;
	}
	TIME(&post);
	DIFF_MSEC("init_iterators", pre, post, NULL);

	env->output_count = 0;

	TIME(&pre);
	if (!analyze_closures__init_groups(env)) {
		goto cleanup;
	}
	TIME(&post);
	DIFF_MSEC("init_groups", pre, post, NULL);

	TIME(&pre);

	switch (analyze_closures__collect(env)) {
	case AC_COLLECT_DONE:
		TIME(&post);
		DIFF_MSEC("collect", pre, post, NULL);

		TIME(&pre);
		if (!analyze_closures__analyze(env)) {
			goto cleanup;
		}
		TIME(&post);
		DIFF_MSEC("analyze", pre, post, NULL);
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
	interned_state_set_release(env->issp, &iss_id);
	if (env->pq != NULL) {
		ipriq_free(env->pq);
		env->pq = NULL;
	}

	return res;

}

static enum ipriq_cmp_res
cmp_iterator_cb(size_t a, size_t b, void *opaque)
{
	struct analyze_closures_env *env = opaque;
	assert(env != NULL);

	assert(a < env->iter_count);
	assert(b < env->iter_count);

	const fsm_state_t to_a = env->iters[a].info.to;
	const fsm_state_t to_b = env->iters[b].info.to;

#if LOG_AC
	fprintf(stderr, "cmp_iterator_ac: a %zu -> to_a %d, b %zu -> to_b %d\n",
	    a, to_a, b, to_b);
#endif

	return to_a < to_b ? IPRIQ_CMP_LT : to_a > to_b ? IPRIQ_CMP_GT : IPRIQ_CMP_EQ;
}

static int
analyze_closures__init_iterators(struct analyze_closures_env *env,
	const struct state_set *ss, size_t set_count)
{
	struct state_iter it;
	fsm_state_t s;
	size_t i_i;

	assert(env->pq == NULL);
	env->pq = ipriq_new(env->alloc,
	    cmp_iterator_cb, (void *)env);
	if (env->pq == NULL) {
		return 0;
	}

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
		 * are unique within the entire state set, so that
		 * doesn't actually help us much. */
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

	assert(env->pq != NULL);
	assert(ipriq_empty(env->pq));

	for (i_i = 0; i_i < set_count; i_i++) {
		struct ac_iter *egi = &env->iters[i_i];
		if (edge_set_group_iter_next(&egi->iter, &egi->info)) {
#if LOG_AC
			fprintf(stderr, "ac_collect: iter[%zu]: to: %d\n",
			    i_i, egi->info.to);
			dump_egi_info(i_i, &egi->info);
#endif
			if (!ipriq_add(env->pq, i_i)) {
				return 0;
			}
		} else {
#if LOG_AC
			fprintf(stderr, "ac_collect: iter[%zu]: DONE\n", i_i);
#endif
			egi->info.to = AC_NO_STATE; /* done */
		}
	}

	return 1;
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

static enum ac_collect_res
analyze_closures__collect(struct analyze_closures_env *env)
{
	/* All iterators have been stepped once, and any that didn't
	 * finish immediately were added to the queue. Keep stepping
	 * whichever is earliest (by .to state) until they're all done.
	 * Merge (label set -> state) info along the way. */

	if (ipriq_empty(env->pq)) {
#if LOG_AC
		fprintf(stderr, "ac_collect: empty\n");
#endif
		return AC_COLLECT_EMPTY;
	}

	size_t steps = 0;
	while (!ipriq_empty(env->pq)) {
		size_t next_i;
		steps++;

		if (env->group_count + 1 == env->group_ceil) {
			if (!analyze_closures__grow_groups(env)) {
				return AC_COLLECT_ERROR;
			}
		}

		if (!ipriq_pop(env->pq, &next_i)) {
			assert(!"unreachable: non-empty, but pop failed");
			return AC_COLLECT_ERROR;
		}

#if LOG_AC
		fprintf(stderr, "ac_collect: popped %zu\n", next_i);
#endif

		assert(next_i < env->iter_count);
		struct ac_iter *iter = &env->iters[next_i];
		assert(iter->info.to != AC_NO_STATE);

		struct ac_group *g = &env->groups[env->group_count];

		if (g->to == AC_NO_STATE) { /* init new group */
			memset(g, 0x00, sizeof(*g));
			g->to = iter->info.to;
			union_with(g->labels, iter->info.symbols);
		} else if (g->to == iter->info.to) { /* update current group */
			union_with(g->labels, iter->info.symbols);
		} else {	/* switch to next group */
			assert(iter->info.to > g->to);
			env->group_count++;
			struct ac_group *ng = &env->groups[env->group_count];
			memset(ng, 0x00, sizeof(*ng));
			ng->to = iter->info.to;
			union_with(ng->labels, iter->info.symbols);
		}

		if (edge_set_group_iter_next(&iter->iter, &iter->info)) {
#if LOG_AC
			fprintf(stderr, "ac_collect: iter %zu -- to %d\n",
			    next_i, iter->info.to);
#endif
			if (!ipriq_add(env->pq, next_i)) {
				return AC_COLLECT_ERROR;
			}
		} else {
#if LOG_AC
			fprintf(stderr, "ac_collect: iter %zu -- DONE\n", next_i);
#endif
		}
	}

	env->group_count++;	/* commit current group */

	return AC_COLLECT_DONE;
}

static int
analyze_closures__analyze(struct analyze_closures_env *env)
{
#if LOG_AC
	/* Dump group table. */
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
	size_t base_i, g_i, o_i; /* base_i, group_i, other_i */

	base_i = 0;
	env->output_count = 0;

	if (env->dst_ceil == 0) {
		if (!analyze_closures__grow_dst(env)) {
			return 0;
		}
	}

	/* Initialize words_used for each group. */
	for (g_i = 0; g_i < env->group_count; g_i++) {
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
			}
		}

#if LOG_AC
		fprintf(stderr, "ac_analyze: dst_count: %zu\n", dst_count);
#endif

		assert(labels[0] || labels[1]
		    || labels[2] || labels[3]);

		/* Since the groups are stored in order we don't need to
		 * clear the bits from all of them -- both are sorting
		 * by ascending .to ID, so sweep over both and clear the
		 * labels on groups with IDs in common. */
		{
			size_t d_i = 0;	     /* dst index */
			size_t g_i = base_i; /* group index */
			while (d_i < dst_count) {
				struct ac_group *g = &env->groups[g_i];
				const fsm_state_t g_to = g->to;
				const fsm_state_t dst = env->dst[d_i];

#if LOG_AC
				fprintf(stderr, "ac_analyze: clearing loop: d_i %zu/%zu, g_i %zu/%zu\n",
				    d_i, dst_count, g_i, env->group_count);
#endif
				/* advance one or both indices, clearing
				 * labels when appropriate */
				if (g_to < dst) {
					g_i++;
					assert(g_i < env->group_count);
				} else if (g_to > dst) {
					d_i++;
				} else {
					assert(g_to == dst);
					clear_group_labels(g, labels);
					g_i++;
					d_i++;
				}
			}
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

		{		/* build the state set and add to the output */
			interned_state_set_id iss = interned_state_set_empty(env->issp);
			size_t d_i;
			for (d_i = 0; d_i < dst_count; d_i++) {
				interned_state_set_id updated;
#if LOG_AC
				fprintf(stderr, "ac_analyze: adding state %d to interned_state_set\n", env->dst[d_i]);
#endif

				if (!interned_state_set_add(env->issp,
					&iss, env->dst[d_i], &updated)) {
					return 0;
				}
				iss = updated;
			}

			if (!analyze_closures__save_output(env, labels, iss)) {
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

#if LOG_AC
	fprintf(stderr, "ac_analyze: done\n");
#endif

	return 1;
}

static int
analyze_closures__save_output(struct analyze_closures_env *env,
    const uint64_t labels[256/4], interned_state_set_id iss)
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
