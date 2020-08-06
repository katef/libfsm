/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_dfa.h"

static enum theft_trial_res
check_minimise_matches_naive_fixpoint_algorithm(const struct dfa_spec *spec);

static enum theft_trial_res
prop_minimise_matches_naive_fixpoint_algorithm(struct theft *t, void *arg1);

static bool
naive_minimised_count(const struct dfa_spec *spec, size_t *count);

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
		return THEFT_TRIAL_ERROR;
	}

	const size_t states_needed_naive = naive_minimised_count(spec);

	if (!fsm_minimise(fsm)) {
		fprintf(stderr, "-- fail: fsm_minimise\n");
		goto cleanup;
	}

	const size_t states_post_minimise = fsm_countstates(fsm);
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

static size_t
naive_minimised_count(const struct dfa_spec *spec)
{
	return spec->state_count; /* lies! */
}

void
register_test_minimise(void)
{
	reg_test3("dfa_minimise", test_dfa_minimise,
	    0, 0, 0);
}
