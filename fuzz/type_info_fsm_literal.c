/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_fsm_literal.h"

#include <ctype.h>

#include <adt/xalloc.h>

static enum theft_alloc_res
fsm_literal_alloc(struct theft *t, void *penv, void **output)
{
	struct test_env *env;
	struct fsm_literal_scen *res;
	const uint8_t literal_bits = 6;
	size_t count;

#define LITERAL_MAX 256
	uint64_t buf64[LITERAL_MAX + 1];
#undef LITERAL_MAX
	char *buf = (char *) buf64;
	size_t lit_i;

	if (penv == NULL) {
		penv = theft_hook_get_env(t);
	}

	env = penv;
	assert(env->tag == 'E');

	assert((1LLU << literal_bits) == FSM_MAX_LITERALS);

	count = theft_random_bits(t, literal_bits);
	assert(count <= FSM_MAX_LITERALS);

	res = xmalloc(sizeof *res + count * sizeof *res->pairs);

	for (lit_i = 0; lit_i < count; lit_i++) {
		size_t lit_size;
		bool duplicates;
		uint8_t *lit;

		lit_size = theft_random_bits(t, 8);
		if (lit_size == 0) {
			lit_size = 1;
		}

		memset(buf64, 0x00, sizeof buf64);

		theft_random_bits_bulk(t, 8*lit_size, buf64);

		/* Make sure string is 0-terminated. The string can
		 * contain inline zeroes, though. */
		buf[lit_size] = '\0';

		/*
		 * If we find a duplicate, then just finish up the set.
		 * This will cause dead ends in the state space, leading
		 * to exploring elsewhere.
		 */
		duplicates = false;
		for (size_t i = 0; i < lit_i; i++) {
			if (res->pairs[i].size != lit_size) {
				continue;
			}
			if (0 == memcmp(res->pairs[i].string, buf, lit_size)) {
				duplicates = true;
				break;
			}
		}
		if (duplicates) {
			break;
		}

		lit = xmalloc(lit_size + 1);

		memcpy(lit, buf, lit_size);
		lit[lit_size] = '\0';

		res->pairs[lit_i].string = lit;
		res->pairs[lit_i].size   = lit_size;
		//hexdump(stdout, lit, lit_size);
	}

	res->tag       = 'L';
	res->verbosity = env->verbosity;
	res->count     = lit_i;

	*output = res;

	return THEFT_ALLOC_OK;
}

static void
fsm_literal_free(void *instance, void *env)
{
	struct fsm_literal_scen *scen = instance;

	(void) env;

	for (size_t i = 0; i < scen->count; i++) {
		free(scen->pairs[i].string);
		scen->pairs[i].string = NULL;
	}

	free(scen);
}

static void
fsm_literal_print(FILE *f, const void *instance, void *env)
{
	const struct fsm_literal_scen *scen = instance;

	(void) env;

	fprintf(f, "#scen{%p}, %zd strings:\n", (void *) scen, scen->count);
	for (size_t li = 0; li < scen->count; li++) {
		uint8_t *lit;
		size_t len;
		bool printable;

		lit = scen->pairs[li].string;
		len = scen->pairs[li].size;

		printable = true;
		for (size_t i = 0; i < len; i++) {
			if (!isprint(lit[i])) {
				printable = false;
				break;
			}
		}

		if (printable) {
			fprintf(f, "-- %zd [%zd]: %s\n", li, len, lit);
		} else {
			fprintf(f, "-- %zd [%zd]: \n", li, len);
			hexdump(f, (uint8_t *) lit, len);
		}
	}
}

const struct theft_type_info type_info_fsm_literal = {
	.alloc = fsm_literal_alloc,
	.free  = fsm_literal_free,
	.print = fsm_literal_print,
	.autoshrink_config = {
		.enable = true,
	}
};

