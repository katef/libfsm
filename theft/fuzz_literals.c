/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include "type_info_fsm_literal.h"

static enum theft_trial_res
prop_union_literals(struct theft *t, void *arg1);
static struct fsm *
add_literal(struct fsm *fsm, const uint8_t *string, size_t size, fsm_end_id_t id);
static bool
check_literal(struct fsm *fsm, struct fsm_literal_scen *scen, fsm_end_id_t id);

static const struct fsm_options opt = {
	.anonymous_states  = 1,
	.consolidate_edges = 1,
};

static bool
test_union_literals(theft_seed seed)
{
	enum theft_run_res res;
	struct theft_type_info arg_info;

	struct test_env env = {
		.tag = 'E',
		.verbosity = 0,
	};

	struct theft_run_config cfg = {
		.name = __func__,
		.prop1 = prop_union_literals,
		.type_info = { &arg_info },
		.trials = 10000,
		.hooks = {
			.trial_pre = trial_pre_fail_once,
			.trial_post = trial_post_inc_verbosity,
			.env = &env,
		},

		.seed = seed,
		.fork = {
			.enable = true,
		},
	};

	//seed = 0x25cb16aaa66773ceLLU;
	memcpy(&arg_info, &type_info_fsm_literal, sizeof arg_info);
	arg_info.env = &env;

	res = theft_run(&cfg);

	return res == THEFT_RUN_PASS;
}

static enum theft_trial_res
check_union_literals(struct fsm_literal_scen *scen)
{
	struct fsm *fsm;
	fsm_state_t s;

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		fprintf(stdout, "ERROR @ %d: fsm_new NULL\n", __LINE__);
		return THEFT_TRIAL_ERROR;
	}

	if (!fsm_addstate(fsm, &s)) {
		fprintf(stderr, "ERROR @ %d: fsm_addstate NULL\n", __LINE__);
		return THEFT_TRIAL_ERROR;
	}
	fsm_setstart(fsm, s);

	assert((intptr_t) scen->count >= 0);

	for (size_t lit_i = 0; lit_i < scen->count; lit_i++) {
		fsm = add_literal(fsm, scen->pairs[lit_i].string,
			scen->pairs[lit_i].size, (intptr_t) lit_i);
		if (fsm == NULL) {
			if (scen->verbosity > 0) {
				printf("FAIL: add_literal returned NULL\n");
			}
			goto fail;
		}
	}

	if (!fsm_determinise(fsm)) {
		if (scen->verbosity > 0) {
			printf("FAIL: fsm_determinise\n");
		}
		goto fail;
	}

	for (fsm_end_id_t lit_i = 0; lit_i < scen->count; lit_i++) {
		if (!check_literal(fsm, scen, lit_i)) {
			goto fail;
		}
	}

	fsm_free(fsm);

	return THEFT_TRIAL_PASS;

fail:

	fsm_free(fsm);

	return THEFT_TRIAL_FAIL;
}

static enum theft_trial_res
prop_union_literals(struct theft *t, void *arg1)
{
	struct fsm_literal_scen *scen = arg1;
	(void)t;
	return check_union_literals(scen);
}

static struct fsm *
add_literal(struct fsm *fsm, const uint8_t *string, size_t size, fsm_end_id_t id)
{
	struct fsm *new;
	struct fsm *res;
	struct re_err err;

	struct string_pair pair = {
		.string = (uint8_t *) string,
		.size   = size
	};

	new = wrap_re_comp(RE_LITERAL, &pair, &opt, &err);
	if (new == NULL) {
		re_perror(RE_LITERAL, &err, NULL, (const char *) string);
		return NULL;
	}

	if (!fsm_setendid(new, id)) {
		return NULL;
	}

	res = fsm_union(fsm, new, NULL);
	if (res == NULL) {
		return NULL;
	}

	return res;
}

/* Check that literal with number LIT_I is still associated with the
 * correct end ID. */
static bool
check_literal(struct fsm *fsm, struct fsm_literal_scen *scen, fsm_end_id_t id)
{
	fsm_end_id_t count;
	fsm_state_t st;
	int e;

	count = (intptr_t) scen->count;

	assert(id < count);
	assert(scen->pairs[id].size > 0);

	e = wrap_fsm_exec(fsm, &scen->pairs[id], &st);

	if (e != 1) {
		if (scen->verbosity > 0) {
			printf("FAIL: fsm_exec failure\n");
		}

		return false;
	}

	{
		enum fsm_getendids_res eres;
		fsm_end_id_t got;
		size_t written;
		eres = fsm_getendids(fsm, st, 1, &got, &written);
		if (eres == FSM_GETENDIDS_FOUND
			&& written == 1
			&& got == id) {
			return true;
		}

		if (scen->verbosity > 0) {
			printf("FAIL: fsm_getendids failure, exp %u, got %u\n",
			    id, got);
		}
	}

	return false;
}

void
register_test_literals(void)
{
	reg_test("union_literals", test_union_literals);
}

