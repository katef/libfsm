/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include "fuzz_re.h"

#include "type_info_re.h"

/*
 * Not yet implemented:
 * - ^ and $ anchors
 * - other PCRE extensions
 */

static bool check_pos_matches(uint8_t verbosity, struct fsm *fsm,
	const uint8_t *re_string, size_t re_size,
	size_t count, const struct string_pair *pairs);
static bool check_neg_matches(uint8_t verbosity, struct fsm *fsm,
	const uint8_t *re_string, size_t re_size,
	size_t count, const struct string_pair *pairs);
static enum theft_trial_res
prop_re_minimise(struct theft *t, void *arg1);
static enum theft_trial_res check_error(uint8_t verbosity, struct re_err *err);

bool
test_re_parser_pcre(uint8_t verbosity,
	const uint8_t *re_string, size_t re_size,
	size_t pos_count, const struct string_pair *pos_pairs,
	size_t neg_count, const struct string_pair *neg_pairs)
{
	struct timeval pre  = { 0, 0 };
	struct timeval post = { 0, 0 };
	size_t tdelta = 0;
	size_t ttotal = 0;
	struct re_err err;
	struct fsm *fsm;

	const struct fsm_options opt = {
		.anonymous_states  = 1,
		.consolidate_edges = 1,
		.always_hex        = 1
	};

	struct string_pair pair = {
		.string = (uint8_t *) re_string,
		.size   = re_size
	};

#define TIME(TV) do { if (-1 == gettimeofday(TV, NULL)) { assert(false); } } while(0)
#define ELAPSED(PRE, POST)						  \
	(1000 * (POST.tv_sec - PRE.tv_sec))						  \
		+ ((POST.tv_usec - PRE.tv_usec) / 1000)

	if (re_string == NULL) {
		return false;
	}

	if (verbosity > 0) {
		fprintf(stdout, "RE[%zd]: \n", re_size);
		hexdump(stdout, re_string, re_size);
		fprintf(stdout, "\n");
	}

	TIME(&pre);

	fsm = wrap_re_comp(RE_PCRE, &pair, &opt, &err);

	TIME(&post);

	tdelta = ELAPSED(pre, post);
	ttotal += tdelta;
	if (verbosity > 0) {
		fprintf(stdout, "re_comp: %zd msec elapsed\n", tdelta);
	}

	if (fsm == NULL) {
		return check_error(verbosity, &err);
	}

	/* Invalid RE: did we flag an error? */
	if (fsm == NULL) {
		if (err.e == RE_ESUCCESS) {
			LOG_FAIL(verbosity, "Expected error, got RE_ESUCCESS\n");
			fprintf(stderr, "Expected error, got RE_ESUCCESS\n");
			return false;
		} else {
			return true;
		}
	}

	TIME(&pre);

	if (!fsm_determinise(fsm)) {
		LOG_FAIL(verbosity, "DETERMINISE\n");
		fprintf(stderr, "DETERMINISE\n");
		goto fail;
	}

	TIME(&post);

	tdelta = ELAPSED(pre, post);
	ttotal += tdelta;
	if (verbosity > 0) {
		fprintf(stdout, "fsm_determinise: %zd msec elapsed\n", tdelta);
	}
	if (tdelta > 1000) {
		LOG_FAIL(verbosity, "fsm_determinise took %zd msec!\n", tdelta);
		//goto fail;
	}

	if (verbosity > 1) {
		fsm_print_dot(LOG_OUT, fsm);
	}

	if (!check_pos_matches(verbosity, fsm,
		re_string, re_size,
		pos_count, pos_pairs)) {
		goto fail;
	}
	if (!check_neg_matches(verbosity, fsm,
		re_string, re_size,
		neg_count, neg_pairs)) {
		goto fail;
	}

	fsm_free(fsm);

	return true;

fail:

	if (fsm != NULL) {
		fsm_free(fsm);
	}

	return false;
}

bool
test_re_pcre_minimize(theft_seed seed)
{
	struct theft_type_info arg_info;
	enum theft_run_res res;

	struct test_env env = {
		.tag = 'E',
		.dialect = RE_PCRE,
	};

	struct theft_run_config cfg = {
		.name = __func__,
		.prop1 = prop_re_minimise,
		.type_info = { &arg_info },
		.trials = 10000,
		.hooks = {
			.trial_pre  = trial_pre_fail_once,
			.trial_post = trial_post_inc_verbosity,
			.env = &env,
		},
		.seed = seed,
		.fork = {
			.enable = false,
		}
	};

	memcpy(&arg_info, &type_info_re, sizeof arg_info);
	arg_info.env = &env;

	res = theft_run(&cfg);

	return res == THEFT_RUN_PASS;
}

static enum theft_trial_res
prop_re_minimise(struct theft *t, void *arg1)
{
	struct test_re_info *info = arg1;
	struct test_env *env;
	uint8_t verbosity;
	unsigned int state_count_pre, state_count_post;
	struct re_err err;
	struct fsm *fsm;

	const struct fsm_options opt = {
		.anonymous_states  = 1,
		.consolidate_edges = 1,
		.always_hex        = 1
	};

	struct string_pair pair = {
		.string = (uint8_t *) info->string,
		.size   = info->size
	};

	assert(info->string);

	env = theft_hook_get_env(t);
	assert(env->tag == 'E');
	verbosity = env->verbosity;

	fsm = wrap_re_comp(RE_PCRE, &pair, &opt, &err);
	if (fsm == NULL) {
		return check_error(verbosity, &err);
	}

	state_count_pre = fsm_count(fsm, fsm_isany);

	if (!fsm_minimise(fsm)) {
		LOG_FAIL(verbosity, "fsm_minimise failure\n");
		return THEFT_TRIAL_FAIL;
	}

	state_count_post = fsm_count(fsm, fsm_isany);

	if (state_count_post > state_count_pre) {
		LOG_FAIL(verbosity, "fsm_minimize increased states! %u -> %u\n",
			state_count_pre, state_count_post);
		goto fail;
	}

	if (!check_pos_matches(verbosity, fsm,
		info->string, info->size,
		info->pos_count, info->pos_pairs)) {
		goto fail;
	}

	if (!check_neg_matches(verbosity, fsm,
		info->string, info->size,
		info->neg_count, info->neg_pairs)) {
		goto fail;
	}

	fsm_free(fsm);

	return THEFT_TRIAL_PASS;

fail:

	fsm_free(fsm);

	return THEFT_TRIAL_FAIL;
}

static enum theft_trial_res
check_error(uint8_t verbosity, struct re_err *err)
{
	if (err->e == RE_ESUCCESS) {
		LOG_FAIL(verbosity, "Expected error, got RE_ESUCCESS\n");
		return THEFT_TRIAL_FAIL;
	} else {
		LOG_FAIL(verbosity, "Got error for RE that failed to compile, passing\n");
		return THEFT_TRIAL_PASS;
	}
}

static bool
check_pos_matches(uint8_t verbosity, struct fsm *fsm,
	const uint8_t *re_string, size_t re_size,
	size_t pos_count, const struct string_pair *pos_pairs)
{
	struct timeval pre  = { 0, 0 };
	struct timeval post = { 0, 0 };
	size_t tdelta = 0;
	size_t ttotal = 0;

	/* Valid RE: check if expected strings are matched */
	if (verbosity > 0) {
		fprintf(stdout, "-- Checking %zd positive matches...\n", pos_count);
	}

	for (size_t i = 0; i < pos_count; i++) {
		fsm_state_t st;
		int e;

		if (pos_pairs[i].string == NULL) {
			if (verbosity > 0) {
				fprintf(stdout, "	%zd: discarded, skipping\n", i);
			}

			continue;
		}

		TIME(&pre);

		e = wrap_fsm_exec(fsm, &pos_pairs[i], &st);

		TIME(&post);

		tdelta = ELAPSED(pre, post);
		ttotal += tdelta;
		if (verbosity > 0) {
			fprintf(stdout, "match: %zd msec elapsed\n", tdelta);
		}
		if (tdelta > 10) {
			LOG_FAIL(verbosity, "match took %zd msec!\n", tdelta);
			fprintf(stderr, "positive DFA match took %zd msec!!!\n", tdelta);
			return false;
		}

		if (verbosity > 1) {
			fsm_print_dot(LOG_OUT, fsm);
		}
		if (e != 1) {
			if (verbosity > 0 || 1) {
				fprintf(LOG_OUT, "\nEXEC FAIL:\nregex \n");
				hexdump(LOG_OUT, re_string, re_size);
				fprintf(LOG_OUT, "\nstring [%zd]: \n", pos_pairs[i].size);
				hexdump(LOG_OUT, pos_pairs[i].string, pos_pairs[i].size);
				fprintf(LOG_OUT, "\n");
			}
			return false;
		} else if (!fsm_isend(fsm, st)) {
			if (verbosity > 0 || 1) {
				fprintf(LOG_OUT, "\nNO END STATE:\nregex \n");
				hexdump(LOG_OUT, re_string, re_size);
				fprintf(LOG_OUT, "\nstring [%zd]: \n", pos_pairs[i].size);
				hexdump(LOG_OUT, pos_pairs[i].string, pos_pairs[i].size);
				fprintf(LOG_OUT, "\n");
			}
			return false;
		}
	}

	return true;
}

static bool
check_neg_matches(uint8_t verbosity, struct fsm *fsm,
	const uint8_t *re_string, size_t re_size,
	size_t neg_count, const struct string_pair *neg_pairs)
{
	struct timeval pre  = { 0, 0 };
	struct timeval post = { 0, 0 };
	size_t tdelta = 0;
	size_t ttotal = 0;

	/* Negative tests: these should probably not match
	 * (though there can be false negatives) */
	if (verbosity > 0) {
		fprintf(stdout, "-- Checking %zd negative matches...\n", neg_count);
	}

	for (size_t i = 0; i < neg_count; i++) {
		fsm_state_t st;
		int r;

		if (neg_pairs[i].string == NULL) {
			if (verbosity > 0) {
				fprintf(stdout, "	%zd: discarded, skipping\n", i);
			}
			continue;
		}

		TIME(&pre);

		r = wrap_fsm_exec(fsm, &neg_pairs[i], &st);

		TIME(&post);

		tdelta = ELAPSED(pre, post);
		ttotal += tdelta;
		if (verbosity > 0) {
			fprintf(stdout, "match: %zd elapsed\n", tdelta);
		}
		if (tdelta > 10) {
			fprintf(stderr, "match took %zd msec!\n", tdelta);
			fprintf(stderr, "negative DFA match took %zd msec!!!\n", tdelta);
			return false;
		}

		if (!r && fsm_isend(fsm, st)) {
			if (verbosity > 0) {
				fprintf(LOG_OUT, "Possible false match:\nregex \n");
				hexdump(LOG_OUT, re_string, re_size);
				fprintf(LOG_OUT, "\nstring [%zd]: \n", neg_pairs[i].size);
				hexdump(LOG_OUT, neg_pairs[i].string, neg_pairs[i].size);
				fprintf(LOG_OUT, "\n");
			}
		}
	}

	return true;
}

