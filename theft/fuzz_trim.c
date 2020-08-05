/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_dfa.h"

#include <adt/queue.h>

static enum theft_trial_res
check_trimming_matches_reachability(const struct dfa_spec *spec);

static enum theft_trial_res
prop_trimming_matches_reachability(struct theft *t, void *arg1);

static bool
count_expected_live(const struct dfa_spec *spec,
    fsm_state_t start_state, size_t *exp);

static bool
test_dfa_trimming(theft_seed seed,
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
		.prop1 = prop_trimming_matches_reachability,
		.type_info = { &type_info_dfa, },
		.hooks = {
			.trial_pre = theft_hook_first_fail_halt,
			.env = (void *)&env,
		},
		.seed = seed,
		.trials = 10000,
		.fork = {
			.enable = true,
			.timeout = 1000,
		},
	};

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));
	return res == THEFT_RUN_PASS;
}

static enum theft_trial_res
prop_trimming_matches_reachability(struct theft *t, void *arg1)
{
	(void)t;
	const struct dfa_spec *spec = arg1;
	return check_trimming_matches_reachability(spec);
}

static enum theft_trial_res
check_trimming_matches_reachability(const struct dfa_spec *spec)
{
	/* For an arbitrary DFA, the number of states after
	 * trimming should match the expected count given what
	 * states are on a path between the start state and
	 * any end state -- no more, no less. */
	struct fsm *fsm = fsm_new(NULL);
	if (fsm == NULL) {
		fprintf(stderr, "-- ERROR: fsm_new\n");
		return THEFT_TRIAL_ERROR;
	}

	if (!fsm_addstate_bulk(fsm, spec->state_count)) {
		fprintf(stderr, "-- ERROR: fsm_addstate_bulk\n");
		return THEFT_TRIAL_ERROR;
	}

	size_t expected_pre_states = 0;

	if (spec->start < spec->state_count) {
		fsm_setstart(fsm, spec->start);
	}

	for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
		const fsm_state_t state_id = (fsm_state_t)s_i;
		const struct dfa_spec_state *s = &spec->states[s_i];
		if (!s->used) { continue; }
		expected_pre_states++;

		if (s->end) {
			fsm_setend(fsm, state_id, 1);
		}

		for (size_t e_i = 0; e_i < s->edge_count; e_i++) {
			const fsm_state_t to = s->edges[e_i].state;
			const unsigned char symbol = s->edges[e_i].symbol;
			if (!fsm_addedge_literal(fsm,
				state_id, to, symbol)) {
				fprintf(stderr, "-- ERROR: fsm_addedge_literal\n");
				return THEFT_TRIAL_ERROR;
			}
		}
	}

	if (!fsm_all(fsm, fsm_isdfa)) {
		fprintf(stderr, "-- FAIL: all is_dfa\n");
		return THEFT_TRIAL_FAIL;
	}

	const size_t pre_states = fsm_countstates(fsm);
	if (pre_states != expected_pre_states) {
		fprintf(stderr, "-- FAIL: pre_states: exp %zu, got %zu\n",
		    expected_pre_states, pre_states);
		return THEFT_TRIAL_FAIL;
	}

	size_t expected_post_states;
	if (!count_expected_live(spec, spec->start, &expected_post_states)) {
		fprintf(stderr, "-- ERROR: count_expected_live alloc\n");
		return THEFT_TRIAL_ERROR;
	}

	long trim_res = fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE);
	if (trim_res < 0) {
		fprintf(stderr, "-- FAIL: trim_res %ld\n", trim_res);
		return THEFT_TRIAL_FAIL;
	}

	const size_t post_states = fsm_countstates(fsm);
	if (post_states != expected_post_states) {
		fprintf(stderr, "-- FAIL: post_states: exp %zu, got %zu\n",
		    expected_post_states, post_states);
		return THEFT_TRIAL_FAIL;
	}

	fsm_free(fsm);
	return THEFT_TRIAL_PASS;
}

static bool
enqueue_reachable(const struct dfa_spec *spec, struct queue *q,
    uint64_t *seen, fsm_state_t state)
{
	if (BITSET_CHECK(seen, state)) { return true; }
	if (!queue_push(q, state)) { return false; }
	BITSET_SET(seen, state);

	const struct dfa_spec_state *s = &spec->states[state];
	assert(s->used);

	for (size_t i = 0; i < s->edge_count; i++) {
		const fsm_state_t to = s->edges[i].state;
		if (BITSET_CHECK(seen, to)) { continue; }
		if (!enqueue_reachable(spec, q, seen, to)) {
			return false;
		}
	}

	return true;
}

static bool
calc_reaches_end_by_fixpoint(const struct dfa_spec *spec,
    uint64_t *reaches_end)
{
	/* A state reaches an end if it's an end, or one of its
	 * edges transitively reaches an end. */
	bool changed;
	do {
		changed = false;
		for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
			const struct dfa_spec_state *s = &spec->states[s_i];
			if (!s->used) { continue; }

			if (s->end) {
				if (!BITSET_CHECK(reaches_end, s_i)) {
					changed = true;
					BITSET_SET(reaches_end, s_i);
				}
				continue;
			}

			for (size_t i = 0; i < s->edge_count; i++) {
				const fsm_state_t to = s->edges[i].state;
				if (BITSET_CHECK(reaches_end, to)) {
					if (!BITSET_CHECK(reaches_end, s_i)) {
						changed = true;
						BITSET_SET(reaches_end, s_i);
					}
				}
			}
		}
	} while (changed);
	return true;
}

static bool
count_expected_live(const struct dfa_spec *spec, fsm_state_t start_state,
	size_t *exp)
{
	bool res = false;
	uint64_t *seen = NULL;
	uint64_t *reaches_end = NULL;
	size_t exp_count = 0;
	struct queue *q = NULL;

	if (spec->state_count == 0) {
		*exp = 0;
		return true;
	}

	q = queue_new(NULL, spec->state_count);
	if (q == NULL) {
		fprintf(stderr, "cleanup %d\n", __LINE__);
		goto cleanup;
	}

	seen = calloc(spec->state_count/64 + 1, sizeof(seen[0]));
	if (seen == NULL) { goto cleanup; }

	reaches_end = calloc(spec->state_count/64 + 1, sizeof(reaches_end[0]));
	if (reaches_end == NULL) { goto cleanup; }

	if (!enqueue_reachable(spec, q, seen, start_state)) {
		fprintf(stderr, "-- ERROR: enqueue_reachable\n");
		goto cleanup;
	}

	if (!calc_reaches_end_by_fixpoint(spec, reaches_end)) {
		fprintf(stderr, "-- ERROR: calc_reaches_end_by_fixpoint\n");
		goto cleanup;
	}

	fsm_state_t s;
	while (queue_pop(q, &s)) {
		if (BITSET_CHECK(reaches_end, s)) {
			exp_count++;
		}
	}

	res = true;
	*exp = exp_count;

cleanup:
	if (seen != NULL) { free(seen); }
	if (reaches_end != NULL) { free(reaches_end); }
	if (q != NULL) { queue_free(q); }

	return res;
}

#define RUN_SPEC(STATES, START)					 \
	struct dfa_spec spec = {				 \
		.state_count = sizeof(STATES)/sizeof(STATES[0]), \
		.start = START,					 \
		.states = STATES,				 \
	};							 \
	return (check_trimming_matches_reachability(&spec)	 \
	    == THEFT_TRIAL_PASS)

static bool
dfa_trim_reg0(theft_seed seed)
{
	(void)seed;
	struct dfa_spec_state states[] = {
		[0] = { .used = true,
			.edge_count = 1,
			.edges = { { .symbol = 0, .state = 2 } },
		},
		[1] = { .used = true,
			.edge_count = 1,
			.edges = { { .symbol = 0, .state = 0 } },
		},
		[2] = { .used = true, .end = true,
			.edge_count = 1,
			.edges = { { .symbol = 0, .state = 1 } },
		},
	};
	RUN_SPEC(states, 0);
}

void
register_test_trim(void)
{
	reg_test("dfa_trimming_regression0", dfa_trim_reg0);

	reg_test3("dfa_trimming", test_dfa_trimming,
	    0, 0, 0);
}
