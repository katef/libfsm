#include "test_libfsm.h"

#include "type_info_re.h"

/* in test_re_parser_pcre.c */
bool test_re_PCRE_minimize(void);

static enum theft_trial_res
prop_re(struct theft *t, void *arg1)
{
	struct test_env *env = (struct test_env *)theft_hook_get_env(t);
	assert(env->tag == 'E');

	struct test_re_info *info = (struct test_re_info *)arg1;
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

bool test_re_regress1(void)
{
	uint8_t high_bit[] = { 0x80, 0x00 };
	struct string_pair exp_pairs[] = {
		{ .size = 1, .string = high_bit },
	};
	return test_re_parser_literal(test_get_verbosity(),
	    high_bit, 1, 1, exp_pairs);
}

bool test_re_regress2(void)
{
	uint8_t eff_eff[] = { 0xFF, 0x00 };
	struct string_pair exp_pairs[] = {
		{ .size = 1, .string = eff_eff },
	};
	return test_re_parser_literal(test_get_verbosity(),
	    eff_eff, 1, 1, exp_pairs);
}

bool test_re_regress3(void)
{
	uint8_t re[] = { '\\', '.', '?' };
	uint8_t zero[] = { '.' };
	struct string_pair exp_pairs[] = {
		{ .size = 1, .string = zero },
	};
	return test_re_parser_pcre(test_get_verbosity(),
	    re, sizeof(re),
	    1, exp_pairs,
	    0, NULL);
}

bool test_re_regress4(void)
{
	uint8_t re[] = "[\\(a]?";
	
	uint8_t str[] = "a";
	struct string_pair exp_pairs[] = {
		{ .size = sizeof(str), .string = str },
	};
	bool res = test_re_parser_pcre(test_get_verbosity(),
	    re, sizeof(re),
	    1, exp_pairs,
	    0, NULL);
	return res;
}

bool test_re_regress5(void)
{
	uint8_t re[] = ".*[\0a]";
	
	uint8_t str[] = "\0\0";
	struct string_pair exp_pairs[] = {
		{ .size = sizeof(str), .string = str },
	};
	bool res = test_re_parser_pcre(0,
	    re, sizeof(re),
	    1, exp_pairs,
	    0, NULL);
	return res;
}

// seed = 0xe86963e7ad089564LLU;
bool test_re_regress6(void)
{
	uint8_t re[] = { '[', 0x01,
			 '\\', '\\', ']',
			 ']' };
	uint8_t str[] = { 0x01, ']'};
	struct string_pair exp_pairs[] = {
		{ .size = sizeof(str), .string = str },
	};
	bool res = test_re_parser_pcre(0,
	    re, sizeof(re),
	    1, exp_pairs,
	    0, NULL);
	return res;
}

bool test_re_aBx(void)
{
	uint8_t re[] = "[aBx]*a";
	uint8_t str[] = "aBxa";
	struct string_pair exp_pairs[] = {
		{ .size = strlen((char *)str), .string = str },
	};
	bool res = test_re_parser_pcre(0,
	    re, strlen((char *)re),
	    1, exp_pairs,
	    0, NULL);
	return res;
}

bool test_re_parser(uintptr_t arg)
{
	enum re_dialect dialect = (enum re_dialect)arg;
	theft_seed seed = theft_seed_of_time();
	struct test_env env = {
		.tag = 'E',
		.dialect = dialect,
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

	struct theft_type_info arg_info;
	memcpy(&arg_info, &type_info_re, sizeof(arg_info));
	arg_info.env = &env;

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
		},
	};
	enum theft_run_res res = theft_run(&cfg);
	return res == THEFT_RUN_PASS;
}

void register_test_re(void)
{
	reg_test("re_regress1", test_re_regress1);
	reg_test("re_regress2", test_re_regress2);
	reg_test("re_regress3", test_re_regress3);
	reg_test("re_regress4", test_re_regress4);
	reg_test("re_regress5", test_re_regress5);
	reg_test("re_regress6", test_re_regress6);

	reg_test("re_aBx", test_re_aBx);

	reg_test1("re_parser_LITERAL", test_re_parser, RE_LITERAL);
	reg_test1("re_parser_PCRE", test_re_parser, RE_PCRE);
	reg_test("re_PCRE_minimize", test_re_PCRE_minimize);
}
