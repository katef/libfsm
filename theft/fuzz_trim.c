/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_dfa.h"

static enum theft_trial_res
check_trimming_matches_reachability(const struct dfa_spec *spec);

static enum theft_trial_res
prop_trimming_matches_reachability(struct theft *t, void *arg1);

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

	struct fsm *fsm = dfa_spec_build_fsm(spec);
	if (fsm == NULL) {
		return THEFT_TRIAL_ERROR;
	}

	size_t expected_pre_states = 0;
	for (size_t s_i = 0; s_i < spec->state_count; s_i++) {
		const struct dfa_spec_state *s = &spec->states[s_i];
		if (!s->used) { continue; }
		expected_pre_states++;
	}

	const size_t pre_states = fsm_countstates(fsm);
	if (pre_states != expected_pre_states) {
		fprintf(stderr, "-- FAIL: pre_states: exp %zu, got %zu\n",
		    expected_pre_states, pre_states);
		return THEFT_TRIAL_FAIL;
	}

	size_t expected_post_states;
	if (!dfa_spec_check_liveness(spec, NULL, &expected_post_states)) {
		fprintf(stderr, "-- ERROR: dfa_spec_check_liveness\n");
		return THEFT_TRIAL_ERROR;
	}

	long trim_res = fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE, NULL);
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
