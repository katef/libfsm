/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_dfa.h"

#define LOG_NAIVE_MIN 0
#define LOG_SPEC 0

static enum theft_trial_res
check_minimise_matches_naive_fixpoint_algorithm(const struct dfa_spec *spec);

static enum theft_trial_res
prop_minimise_matches_naive_fixpoint_algorithm(struct theft *t, void *arg1);

static bool
naive_minimised_count(const struct dfa_spec *spec, size_t *count);

static bool
naive_minimised_count_inner(const struct dfa_spec *spec, size_t *count);

static bool
test_dfa_minimise(theft_seed seed,
    uintptr_t state_ceil_bits,
    uintptr_t symbol_ceil_bits,
    uintptr_t end_bits)
{
	enum theft_run_res res;

	struct dfa_spec_env env = {
		.tag = 'D',
		.state_ceil_bits = state_ceil_bits,
		.symbol_ceil_bits = symbol_ceil_bits,
		.end_bits = end_bits,
	};

	const char *gen_seed = getenv("GEN_SEED");

	if (gen_seed != NULL) {
		seed = strtoul(gen_seed, NULL, 0);
		theft_generate(stderr, seed,
		    &type_info_dfa, (void *)&env);
		return true;
	}

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_minimise_matches_naive_fixpoint_algorithm,
		.type_info = { &type_info_dfa, },
		.hooks = {
			/* .trial_pre = theft_hook_first_fail_halt, */
			.env = (void *)&env,
		},
		.seed = seed,
		.trials = 10000,
		.fork = {
			.enable = true,
			.timeout = 10000,
		},
	};

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));
	return res == THEFT_RUN_PASS;
}

static enum theft_trial_res
prop_minimise_matches_naive_fixpoint_algorithm(struct theft *t, void *arg1)
{
	(void)t;
	const struct dfa_spec *spec = arg1;
	return check_minimise_matches_naive_fixpoint_algorithm(spec);
}

static enum theft_trial_res
check_minimise_matches_naive_fixpoint_algorithm(const struct dfa_spec *spec)
{
	enum theft_trial_res res = THEFT_TRIAL_FAIL;
	(void)spec;

	/* For an arbitrary DFA, the number of states remaining after
	 * minimisation matches the number of unique states necessary
	 * according to a naive fixpoint-based minimisation algorithm. */

	if (spec->state_count == 0) {
		return THEFT_TRIAL_SKIP;
	}

	struct fsm *fsm = dfa_spec_build_fsm(spec);
	if (fsm == NULL) {
		fprintf(stderr, "-- ERROR: dfa_spec_build_fsm\n");
		return THEFT_TRIAL_ERROR;
	}

	size_t states_needed_naive;
	if (!naive_minimised_count(spec, &states_needed_naive)) {
		fprintf(stderr, "-- ERROR: naive_minimised_count\n");
		res = THEFT_TRIAL_ERROR;
		goto cleanup;
	}

	if (!fsm_minimise(fsm)) {
		fprintf(stderr, "-- fail: fsm_minimise\n");
		goto cleanup;
	}

	const size_t states_post_minimise = fsm_countstates(fsm);

	if (states_post_minimise == 0) {
		res = THEFT_TRIAL_SKIP;
		goto cleanup;
	}

	if (states_post_minimise != states_needed_naive) {
		fprintf(stderr, "-- fail: state count after fsm_minimise: exp %zu, got %zu\n",
		    states_needed_naive, states_post_minimise);
		goto cleanup;
	}

	res = THEFT_TRIAL_PASS;

cleanup:
	if (fsm != NULL) { fsm_free(fsm); }
	return res;
}

static bool
collect_labels(const struct dfa_spec *spec,
    unsigned char **labels, size_t *label_count)
{
	uint64_t used[4] = { 0 };
	size_t count = 0;
	for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
		const struct dfa_spec_state *s = &spec->states[s_i];
		if (!s->used) { continue; }
		for (size_t e_i = 0; e_i < s->edge_count; e_i++) {
			const unsigned char symbol = s->edges[e_i].symbol;
			if (!BITSET_CHECK(used, symbol)) {
				BITSET_SET(used, symbol);
				count++;
			}
		}
	}

	unsigned char *res = malloc(count * sizeof(res[0]));
	if (res == NULL) {
		return false;
	}

	size_t written = 0;
	for (size_t i = 0; i < 256; i++) {
		if (BITSET_CHECK(used, i)) {
			res[written] = (unsigned char)i;
			written++;
		}
	}
	assert(written == count);

	*labels = res;
	*label_count = count;
	return true;
}

static fsm_state_t
naive_delta(const struct dfa_spec *spec,
    fsm_state_t from, unsigned char label)
{
	if (from == spec->state_count) {
		goto dead_state;
	}
	assert(from < spec->state_count);

	const struct dfa_spec_state *s = &spec->states[from];
	assert(s->used);
	for (size_t e_i = 0; e_i < s->edge_count; e_i++) {
		const unsigned char symbol = s->edges[e_i].symbol;
		if (symbol == label) {
			return s->edges[e_i].state;
		}
	}

dead_state:
	return spec->state_count;	/* dead state */
}

static struct dfa_spec *
alloc_trimmed_spec(const struct dfa_spec *spec)
{
	struct dfa_spec *res = NULL;
	uint64_t *live = NULL;
	struct dfa_spec_state *nstates = NULL;
	fsm_state_t *mapping = NULL;

	const size_t live_words = spec->state_count/64 + 1;
	live = calloc(live_words, sizeof(live[0]));
	if (live == NULL) { goto cleanup; }

	res = malloc(sizeof(*res));
	if (res == NULL) { goto cleanup; }

	size_t live_count;
	if (!dfa_spec_check_liveness(spec, live, &live_count)) {
		goto cleanup;
	}
#define NO_STATE ((fsm_state_t)-1)

	nstates = calloc(live_count,
	    sizeof(nstates[0]));
	if (nstates == NULL) { goto cleanup; }

	mapping = calloc(spec->state_count, sizeof(mapping[0]));
	if (mapping == NULL) { goto cleanup; }

	/* FIXME: how should this handle a dead start? */
	assert(mapping[spec->start] != NO_STATE);

	/* First pass: assign new IDs */
	size_t dst = 0;
	for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
		if (BITSET_CHECK(live, s_i)) {
			mapping[s_i] = dst;
			dst++;
		} else {
			mapping[s_i] = NO_STATE;
		}
	}
	assert(dst == live_count);

	/* Second pass: rewrite and compact states */
	dst = 0;
	for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
		if (mapping[s_i] == NO_STATE) { continue; }
		const struct dfa_spec_state *os = &spec->states[s_i];
		struct dfa_spec_state *ns = &nstates[dst];
		ns->used = true;
		ns->end = os->end;
		for (size_t e_i = 0; e_i < os->edge_count; e_i++) {
			const struct dfa_spec_edge *oe = &os->edges[e_i];
			/* discard edges to trimmed states */
			if (mapping[oe->state] == NO_STATE) { continue; }
			ns->edges[ns->edge_count].symbol = oe->symbol;
			ns->edges[ns->edge_count].state = mapping[oe->state];
			ns->edge_count++;
		}
		dst++;
	}
	assert(dst == live_count);

	res->state_count = live_count;
	res->states = nstates;
	res->start = mapping[spec->start];
	free(live);
	free(mapping);
	return res;

#undef NO_STATE
cleanup:
	if (res != NULL) { free(res); }
	if (live != NULL) { free(live); }
	if (nstates != NULL) { free(nstates); }
	if (mapping != NULL) { free(mapping); }
	return NULL;
}

static bool
naive_minimised_count(const struct dfa_spec *spec, size_t *count)
{
	bool res = true;

	struct dfa_spec *nspec = alloc_trimmed_spec(spec);
	if (nspec == NULL) {
		return false;
	}

	if (!naive_minimised_count_inner(nspec, count)) {
		res = false;
	}

	free(nspec->states);
	free(nspec);

	return res;
}

#if LOG_SPEC
static void
dump_spec(const struct dfa_spec *spec)
{
	fprintf(stderr, "-- dump_spec\n");
	for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
		const struct dfa_spec_state *s = &spec->states[s_i];
		assert(s->used);
		fprintf(stderr, "  -- s_i %zu\n", s_i);

		for (size_t e_i = 0; e_i < s->edge_count; e_i++) {
			fprintf(stderr, "    -- e_i %zu, [label 0x%x, to %d]\n",
			    e_i, s->edges[e_i].symbol, s->edges[e_i].state);
		}
	}
}
#endif

static bool
naive_minimised_count_inner(const struct dfa_spec *spec, size_t *count)
{
	bool res = false;

	/* Fixpoint-based algorithm for calculating equivalent states
	 * in a DFA, from pages 68-69 ("A minimization algorithm") of
	 * _Introduction to Automata Theory, Languages, and Computation_
	 * by Hopcroft & Ullman.
	 *
	 * Allocate an NxN bit table, used to track which states have
	 * been proven distinguishable.
	 *
	 * For two states A and B, only use the half of the table where
	 * A <= B, to avoid redundantly comparing B with A. In other
	 * words, only the bits below the diagonal are actually used.
	 *
	 * An extra state is added to internally represent a complete
	 * graph -- for any state that does not have a particular label
	 * used elsewhere, treat it as if it has that label leading to
	 * an extra dead state, which is non-accepting and only has
	 * edges leading to itself.
	 *
	 * To start, mark all pairs of states in the table where one
	 * state is an end state and the other is not. This indicates
	 * that they are distinguishable by their result. Then, using
	 * the Myhill-Nerode theorem, mark all pairs of states where
	 * following the same label leads each to a pair of states that
	 * have already been marked (and are thus transitively
	 * distinguishable). Repeat until no further progress can be
	 * made. Any pairs of states still unmarked can be combined into
	 * a single state, because no possible input leads them to
	 * observably distinguishable results.
	 *
	 * This method scales poorly as the number of states increases
	 * (since it's comparing everything against everything else over
	 * and over, for every label present), but it's straightforward
	 * to implement and to check its results by hand, so it works
	 * well as a test oracle. */
	uint64_t *table = NULL;
	const fsm_state_t dead_state = spec->state_count;
	const size_t table_states = (spec->state_count + 1 /* dead state */);
	const size_t row_words = table_states/64 + 1 /* round up */;
	fsm_state_t *tmp_map = NULL;
	size_t label_count;
	unsigned char *labels = NULL;
	fsm_state_t *mapping = NULL;

#if LOG_SPEC
	dump_spec(spec);
#endif

	table = calloc(row_words * table_states, sizeof(table[0]));
	if (table == NULL) { goto cleanup; }

	tmp_map = malloc(spec->state_count * sizeof(tmp_map[0]));
	if (tmp_map == NULL) { goto cleanup; }

	mapping = malloc(spec->state_count * sizeof(mapping[0]));
	if (mapping == NULL) { goto cleanup; }

	if (!collect_labels(spec, &labels, &label_count)) {
		goto cleanup;
	}

	/* macros for NxN bit table */
#define POS(X,Y) ((X*table_states) + Y)
#define BITS 64
#define BIT_OP(X,Y,OP) (table[POS(X,Y)/BITS] OP (1ULL << (POS(X,Y) & (BITS - 1))))
#define MARK(X,Y)  (BIT_OP(X, Y, |=))
#define CLEAR(X,Y) (BIT_OP(X, Y, &=~))
#define CHECK(X,Y) (BIT_OP(X, Y, &))
	/* This implementation assumes that BITS is a power of 2,
	 * to avoid the extra overhead from modulus. True when
	 * sizeof(unsigned) is 4 or 8. */
	assert((BITS & (BITS - 1)) == 0);

	/* Mark all pairs of states where one is final and one is not.
	 * This includes the dead state. */
	for (size_t i = 0; i < table_states; i++) {
		for (size_t j = 0; j < i; j++) {
			const bool end_i = i == dead_state
			    ? false : spec->states[i].end;
			const bool end_j = j == dead_state
			    ? false : spec->states[j].end;
			if (end_i != end_j) {
				if (LOG_NAIVE_MIN > 0) {
					fprintf(stderr,
					    "naive: setup mark %ld,%ld\n",
					    i, j);
				}
				MARK(i, j);
			}
		}
	}

	bool changed;
	do {			/* run until fixpoint */
		changed = false;
		/* For every combination of states that are still
		 * considered potentially indistinguishable, check
		 * whether the current label leads both to a pair of
		 * distinguishable states. If so, then they are
		 * transitively distinguishable, so mark them and
		 * and set changed to note that another iteration may be
		 * necessary. */
		if (LOG_NAIVE_MIN > 1) {
			fprintf(stderr, "==== table\n");
			for (size_t i = 0; i < table_states; i++) {
				fprintf(stderr, "%-6zu%s ", i, "|");
				for (size_t j = 0; j < i; j++) {
					fprintf(stderr, "%-8d",
					    CHECK(i, j) != 0);
				}
				fprintf(stderr, "\n");
			}

			fprintf(stderr, "%6s", " ");
			for (size_t i = 0; i < table_states; i++) {
				fprintf(stderr, "--------");
			}
			fprintf(stderr, "\n%8s", " ");
			for (size_t i = 0; i < table_states; i++) {
				fprintf(stderr, "%-8zu", i);
			}
			fprintf(stderr, "\n");

			fprintf(stderr, "====\n");
		}

		for (size_t i = 0; i < table_states; i++) {
			for (size_t j = 0; j < i; j++) {
				if (LOG_NAIVE_MIN > 0) {
					fprintf(stderr, "naive: fix [%ld,%ld]: %d\n",
					    i, j, 0 != CHECK(i,j));
				}

				if (CHECK(i, j)) { continue; }
				for (size_t label_i = 0; label_i < label_count; label_i++) {
					const unsigned char label = labels[label_i];
					const fsm_state_t to_i = naive_delta(spec, i, label);
					const fsm_state_t to_j = naive_delta(spec, j, label);
					bool tbval;
					if (to_i >= to_j) {
						tbval = 0 != CHECK(to_i, to_j);
					} else {
						tbval = 0 != CHECK(to_j, to_i);
					}

					if (LOG_NAIVE_MIN > 0) {
						fprintf(stderr,
						    "naive: label 0x%x, i %ld -> %d, j %ld -> %d => tbs %d\n",
						    label, i, to_i, j, to_j, tbval);
					}

					if (tbval) {
						if (LOG_NAIVE_MIN > 0) {
							fprintf(stderr,
							    "naive: MARKING %ld,%ld\n",
							    i, j);
						}

						MARK(i, j);
						changed = true;
						break;
					}
				}
			}
		}
	} while (changed);

#define NO_ID (fsm_state_t)-1

	/* Initialize to (NO_ID) */
	for (size_t i = 0; i < spec->state_count; i++) {
		tmp_map[i] = NO_ID;
	}

	/* For any state that cannot be proven distinguishable from an
	 * earlier state, map it back to the earlier one. */
	for (size_t i = 1; i < spec->state_count; i++) {
		for (size_t j = 0; j < i; j++) {
			if (!CHECK(i, j)) {
				tmp_map[i] = j;
				if (LOG_NAIVE_MIN > 0) {
					fprintf(stderr, "naive: merging %ld with %ld\n",
					    i, j);
				}
			}
		}
	}

	/* Set IDs for the states, mapping some back to earlier states
	 * (combining them).
	 *
	 * Note: this does not count the dead state. */
	fsm_state_t new_id = 0;
	for (size_t i = 0; i < spec->state_count; i++) {
		if (tmp_map[i] != NO_ID) {
			fsm_state_t prev = mapping[tmp_map[i]];

			mapping[i] = prev;
		} else {
			mapping[i] = new_id;
			new_id++;
		}
	}

	if (LOG_NAIVE_MIN > 1) {
		for (size_t i = 0; i < spec->state_count; i++) {
			fprintf(stderr, "mapping[%zu]: %d\n", i, mapping[i]);
		}
	}

	*count = new_id;
	res = true;

#undef POS
#undef BITS
#undef MARK
#undef CHECK
#undef CLEAR
#undef BIT_OP
#undef NO_ID

cleanup:
	if (labels != NULL) { free(labels); }
	if (table != NULL) { free(table); }
	if (tmp_map != NULL) { free(tmp_map); }
	if (mapping != NULL) { free(mapping); }
	return res;
}

#define RUN_SPEC(STATES, START)					 \
	{							 \
		struct dfa_spec spec = {			 \
			.state_count = sizeof(STATES)/sizeof(STATES[0]), \
			.start = START,					\
			.states = STATES,				\
		};							\
		const enum theft_trial_res tres =			\
		    check_minimise_matches_naive_fixpoint_algorithm(&spec); \
		if (tres == THEFT_TRIAL_ERROR) {			\
			fprintf(stderr, "(ERROR)\n");			\
			return false;					\
		} else if (tres == THEFT_TRIAL_SKIP) {			\
			fprintf(stderr, "(SKIP)\n");			\
			return true;					\
		} else {						\
			return tres == THEFT_TRIAL_PASS;		\
		}							\
	}

static bool
dfa_min_ex(theft_seed seed)
{
	(void)seed;
	/* example from Hopcroft & Ullman */
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 1, },
				{ .symbol = 1, .state = 5, },
			},
		},
		[1] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 6, },
				{ .symbol = 1, .state = 2, },
			},
		},
		[2] = { .used = true, .end = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 0, },
				{ .symbol = 1, .state = 2, },
			},
		},
		[3] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 2, },
				{ .symbol = 1, .state = 6, },
			},
		},
		[4] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 7, },
				{ .symbol = 1, .state = 5, },
			},
		},
		[5] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 2, },
				{ .symbol = 1, .state = 6, },
			},
		},
		[6] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 6, },
				{ .symbol = 1, .state = 4, },
			},
		},
		[7] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0, .state = 6, },
				{ .symbol = 1, .state = 2, },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg0(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 0,
		},
		[1] = { .used = true, .end = true,
			.edge_count = 0,
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg1(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true, .end = true,
			.edge_count = 0,
		},
		[1] = { .used = true,
			.edge_count = 0,
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg2(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 2 },
			},
		},
		[1] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
		[2] = { .used = true, .end = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 1 },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg3(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true, .end = true,
			.edge_count = 0,
		},
		[1] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg4(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true, .end = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
		[1] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg5(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 1 },
			},
		},
		[1] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x1, .state = 2 },
			},
		},
		[2] = { .used = true, .end = true,
			.edge_count = 0,
		},
	};
	RUN_SPEC(states, 0);
}

/* The minimal example of a DFA where our implementation of Brzozowski's
 * minimisation _added_ states. Found by the property tests. */
static bool
dfa_min_reg_brz(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0x0, .state = 0 },
				{ .symbol = 0x1, .state = 1 },
			},
		},
		[1] = { .used = true, .end = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg_m0(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x1, .state = 1 },
			},
		},
		[1] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 2 },
			},
		},
		[2] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 3 },
			},
		},
		[3] = { .used = true, .end = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg_end_distance_compaction(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0x0, .state = 1 },
				{ .symbol = 0x1, .state = 2 },
			},
		},
		[1] = { .used = true, .edge_count = 0, },
		[2] = { .used = true, .end = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0x0, .state = 3 },
				{ .symbol = 0x1, .state = 4 },
			},
		},
		[3] = { .used = true, .edge_count = 0, },
		[4] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 5 },
			},
		},
		[5] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 2 },
			},
		},
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_reg_realloc_counts(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 2, .edges = {
				{ .symbol = 0x0, .state = 4 },
				{ .symbol = 0x1, .state = 1 },
			},
		},
		[1] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 0 },
			},
		},
		[2] = { .used = true, },
		[3] = { .used = true, },
		[4] = { .used = true,
			.edge_count = 1, .edges = {
				{ .symbol = 0x0, .state = 7 },
			},
		},
		[5] = { .used = true, },
		[6] = { .used = true, },
		[7] = { .used = true, .end = true, },
	};
	RUN_SPEC(states, 0);
}

static bool
dfa_min_ring(theft_seed seed)
{
#define LIMIT 2000
	static struct dfa_spec_state ring_states[LIMIT];
	size_t i;
	(void)seed;
	for (i = 0; i < LIMIT; i++) {
		struct dfa_spec_state *s = &ring_states[i];
		s->used = 1;
		s->edge_count = 1;
		s->edges[0].symbol = 0x0;
		s->edges[0].state = i + 1;
	}

	/* wrap around to start */
	ring_states[LIMIT - 1].edges[0].state = 0;

	/* last state is edge state */
	ring_states[LIMIT - 1].end = 1;

	/* so is halfway */
	ring_states[LIMIT/2 - 1].end = 1;

	RUN_SPEC(ring_states, 0);
#undef LIMIT
}

void
register_test_minimise(void)
{
	reg_test("dfa_minimise_example", dfa_min_ex);
	reg_test("dfa_minimise_regression0", dfa_min_reg0);
	reg_test("dfa_minimise_regression1", dfa_min_reg1);
	reg_test("dfa_minimise_regression2", dfa_min_reg2);
	reg_test("dfa_minimise_regression3", dfa_min_reg3);
	reg_test("dfa_minimise_regression4", dfa_min_reg4);
	reg_test("dfa_minimise_regression5", dfa_min_reg5);

	reg_test("dfa_minimise_regression_brz", dfa_min_reg_brz);

	reg_test("dfa_minimise_regression_m0", dfa_min_reg_m0);
	reg_test("dfa_minimise_regression_end_distance_compaction",
	    dfa_min_reg_end_distance_compaction);
	reg_test("dfa_minimise_regression_realloc_counts",
	    dfa_min_reg_realloc_counts);

	reg_test("dfa_minimise_ring", dfa_min_ring);


	/* state, symbol, end */
	reg_test3("dfa_minimise_0_0_0", test_dfa_minimise,
	    0, 0, 0);
	reg_test3("dfa_minimise_8_2_2", test_dfa_minimise,
	    8, 2, 2);
	reg_test3("dfa_minimise_8_2_4", test_dfa_minimise,
	    8, 2, 4);
	reg_test3("dfa_minimise_8_3_2", test_dfa_minimise,
	    8, 3, 2);
	reg_test3("dfa_minimise_8_3_4", test_dfa_minimise,
	    8, 3, 4);
	reg_test3("dfa_minimise_9_5_2", test_dfa_minimise,
	    9, 5, 2);
	reg_test3("dfa_minimise_9_5_4", test_dfa_minimise,
	    9, 5, 4);
}
