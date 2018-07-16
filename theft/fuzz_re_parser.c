/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include "fuzz_re.h"

#include "type_info_re.h"

static enum theft_trial_res
prop_re(struct theft *t, void *arg1)
{
	struct test_re_info *info = arg1;
	struct test_env *env;

	env = theft_hook_get_env(t);

	assert(env->tag == 'E');

	switch (info->dialect) {
	case RE_LITERAL:
		return test_re_parser_literal(env->verbosity,
			info->string, info->size,
			info->pos_count, info->pos_pairs)
			? THEFT_TRIAL_PASS
			: THEFT_TRIAL_FAIL;

	case RE_PCRE:
		return test_re_parser_pcre(env->verbosity,
			info->string, info->size,
			info->pos_count, info->pos_pairs,
			info->neg_count, info->neg_pairs)
			? THEFT_TRIAL_PASS
			: THEFT_TRIAL_FAIL;

	default:
		return THEFT_TRIAL_ERROR;
	}
}

bool
test_re_parser(theft_seed seed, uintptr_t arg)
{
	struct theft_type_info arg_info;
	enum theft_run_res res;

	struct test_env env = {
		.tag = 'E',
		.dialect = (enum re_dialect) arg,
	};

	//seed = 0x3961b78ee5691deaLLU;
	//seed = 0xb78b0cc5c726b0baLLU;

	//seed = 0x2e3ed21639bada8bLLU;
	//seed = 0x4d8bf6b95265bec9LLU;

	//seed = 0x58a0ea8aea9b1432LLU;
	//seed = 0xa624e67b8553d8caLLU;

	//seed = 0xe86963e7ad089564LLU;

	//seed = 0x475da3e4667ae202LLU;
	//seed = 0x5fd3e54988c24fadLLU;

	struct theft_run_config cfg = {
		.name = __func__,
		.prop1 = prop_re,
		.type_info = { &arg_info },
		.trials = 10000,
		.hooks = {
			.trial_pre = trial_pre_fail_once,
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

void
register_test_re(void)
{
	reg_test1("re_parser_literal", test_re_parser, RE_LITERAL);
	reg_test1("re_parser_pcre", test_re_parser, RE_PCRE);
	reg_test("re_pcre_minimize", test_re_pcre_minimize);
}

