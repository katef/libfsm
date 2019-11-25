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
add_literal(struct fsm *fsm, const uint8_t *string, size_t size, intptr_t id);
static bool
check_literal(struct fsm *fsm, struct fsm_literal_scen *scen, intptr_t id);
static void
carryopaque_cb(const fsm_state_t *set, size_t count,
	struct fsm *fsm, fsm_state_t state);

static const struct fsm_options opt = {
	.anonymous_states  = 1,
	.consolidate_edges = 1,
	.carryopaque = carryopaque_cb,
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
prop_union_literals(struct theft *t, void *arg1)
{
	struct fsm_literal_scen *scen = arg1;
	struct fsm *fsm;
	fsm_state_t s;

	(void) t;

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

	for (size_t lit_i = 0; lit_i < scen->count; lit_i++) {
		if (!check_literal(fsm, scen, (intptr_t) lit_i)) {
			goto fail;
		}
	}

	fsm_free(fsm);

	return THEFT_TRIAL_PASS;

fail:

	fsm_free(fsm);

	return THEFT_TRIAL_FAIL;
}

static struct fsm *
add_literal(struct fsm *fsm, const uint8_t *string, size_t size, intptr_t id)
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

	fsm_setendopaque(new, (void *) id);

	res = fsm_union(fsm, new);
	if (res == NULL) {
		return NULL;
	}

	return fsm;
}

/* Check that literal with number LIT_I is still associated with the
 * correct opaque info (its ID). */
static bool
check_literal(struct fsm *fsm, struct fsm_literal_scen *scen, intptr_t id)
{
	intptr_t count;
	intptr_t opaque;
	fsm_state_t st;
	int e;

	count = (intptr_t) scen->count;

	assert(count >= 0);
	assert(id >= 0);
	assert(id < count);
	assert(scen->pairs[id].size > 0);

	e = wrap_fsm_exec(fsm, &scen->pairs[id], &st);

	if (e != 1) {
		if (scen->verbosity > 0) {
			printf("FAIL: fsm_exec failure\n");
		}

		return false;
	}

	opaque = (intptr_t) fsm_getopaque(fsm, st);
	if (opaque == id) {
		return true;
	}

	if (scen->verbosity > 0) {
		printf("FAIL: fsm_getopaque failure, exp % "
			PRIdPTR ", got %" PRIdPTR" \n",
			id, opaque);
	}

	return false;
}

static void
carryopaque_cb(const fsm_state_t *set, size_t count,
	struct fsm *fsm, fsm_state_t state)
{
	const intptr_t NONE = -1;
	intptr_t first_id;
	size_t first_i;
	intptr_t other;
	size_t i;

	if (!fsm_isend(fsm, state)) {
		return;
	}

	assert(fsm_getopaque(fsm, state) == NULL);

	/* Get the first id */
	first_id = NONE;

	for (first_i = 0; first_i < count; first_i++) {
		/* Get first udata */
		if (fsm_isend(fsm, set[first_i])) {
			first_id = (intptr_t) fsm_getopaque(fsm, set[first_i]);
			fsm_setopaque(fsm, state, (void *) first_id);
			break;
		}
	}

	other = NONE;
	for (i = 0; i < count; i++) {
		if (i == first_i) {
			continue;
		}

		if (!fsm_isend(fsm, set[i])) {
			continue;
		}

		assert(fsm_getopaque(fsm, set[i]) != NULL);
		other = (intptr_t) fsm_getopaque(fsm, set[i]);
	}

	if (first_id != NONE && other == NONE) {
		fsm_setopaque(fsm, state, (void *) first_id);
	} else if (first_id == NONE && other != NONE) {
		fsm_setopaque(fsm, state, (void *) other);
	} else if (first_id != other) {
		printf("CONFLICT\n");
	}
}

void
register_test_literals(void)
{
	reg_test("union_literals", test_union_literals);
}

