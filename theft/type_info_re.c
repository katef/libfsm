/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_re.h"

#include <string.h>
#include <limits.h>

#include <adt/xalloc.h>

static enum theft_alloc_res
re_alloc(struct theft *t, void *penv, void **output)
{
	struct test_re_info *res;
	struct test_env *env;
	bool ok;

	env = penv;

	assert(env->tag == 'E');

	res = xcalloc(1, sizeof *res);

	res->tag = 'r';

	ok = true;
	switch (env->dialect) {
	case RE_LITERAL:
		ok = type_info_re_literal_build_info(t, res);
		break;

	case RE_PCRE:
		ok = type_info_re_pcre_build_info(t, res);
		break;

	default:
		fprintf(stderr, "NYI\n");
		ok = false;
		break;
	}

	if (!ok) {
		free(res);
		return THEFT_ALLOC_ERROR;
	}

	res->dialect = env->dialect;

	*output = res;

	return THEFT_ALLOC_OK;
}

static void
re_free(void *instance, void *env)
{
	struct test_re_info *info;

	(void) env;

	info = instance;

	assert(info->tag == 'r');

	switch (info->dialect) {
	case RE_LITERAL:
		/* we're just matching the string literal against itself */
		break;

	case RE_PCRE:
		free(info->string);
		type_info_re_pcre_free(info->u.pcre.head);
		break;

	default:
		assert(false);
	}

	for (size_t i = 0; i < info->pos_count; i++) {
		free(info->pos_pairs[i].string);
	}
	free(info->pos_pairs);

	for (size_t i = 0; i < info->neg_count; i++) {
		free(info->neg_pairs[i].string);
	}

	free(info->neg_pairs);
	free(info);
}

static void
re_print(FILE *f, const void *instance, void *env)
{
	const struct test_re_info *info = instance;

	(void) env;

	assert(info->tag == 'r');

	/* RE string is built in the test */
	fprintf(f, "re_info{%p}, %zd: \n",
		(void *) info, info->pos_count);

	hexdump(f, info->string, info->size);
	fprintf(f, "\n");

	if (info->pos_count > 0) {
		fprintf(f, "## Positive test string(s): %zd\n", info->pos_count);
	}
	for (size_t li = 0; li < info->pos_count; li++) {
		struct string_pair *pair = &info->pos_pairs[li];
		const uint8_t *str = pair->string;
		size_t len = pair->size;

		fprintf(f, "-- %zd [%zd]: ", li, len);
		hexdump(f, str, len);
		fprintf(f, "\n");
	}

	if (info->neg_count > 0) {
		fprintf(f, "## Negative test string(s): %zd\n", info->neg_count);
	}
	for (size_t li = 0; li < info->neg_count; li++) {
		struct string_pair *pair;
		uint8_t *str;
		size_t len;

		pair = &info->neg_pairs[li];
		str  = pair->string;

		if (str == NULL) {
			fprintf(f, "-- %zd: [rejected]\n", li);
			continue;
		}

		len = pair->size;

		fprintf(f, "-- %zd [%zd]: ", li, len);
		hexdump(f, str, len);
		fprintf(f, "\n");
	}
}

const struct theft_type_info type_info_re = {
	.alloc = re_alloc,
	.free  = re_free,
	.print = re_print,
	.autoshrink_config = {
		.enable = true,
	}
};

