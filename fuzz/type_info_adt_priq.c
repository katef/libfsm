/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_adt_priq.h"

#include <adt/xalloc.h>

static enum theft_alloc_res
alloc(struct theft *t, void *type_env, void **output)
{
	struct priq_hook_env *env;
	struct priq_scenario *res;
	size_t count;

	(void) type_env;

	env = theft_hook_get_env(t);
	assert(env->tag == 'p');

	if (env->count_bits == 0) {
		env->count_bits = 10;
	}

	if (env->id_bits == 0) {
		env->id_bits = env->count_bits - 2;
	}

	if (env->cost_bits == 0) {
		env->cost_bits = 10;
	}

	count = theft_random_bits(t, env->count_bits) + 1;

	res = xmalloc(sizeof *res + count * sizeof *res->ops);

	res->count = count;

	for (size_t i = 0; i < count; i++) {
		struct priq_op *op;

		op = &res->ops[i];
		op->t = theft_random_choice(t, PRIQ_OP_TYPE_COUNT);
		switch (op->t) {
		case PRIQ_OP_POP:
			break;

		case PRIQ_OP_PUSH:
			op->u.push.id = 1 + theft_random_bits(t, env->id_bits);
			op->u.push.cost = theft_random_bits(t, env->cost_bits);
			break;

		case PRIQ_OP_UPDATE:
			op->u.update.id = 1 + theft_random_bits(t, env->id_bits);
			op->u.update.cost = theft_random_bits(t, env->cost_bits);
			break;

		case PRIQ_OP_MOVE:
			op->u.move.old_id = 1 + theft_random_bits(t, env->id_bits);
			break;

		default:
			assert(false);
			break;
		}
	}

	*output = res;

	return THEFT_ALLOC_OK;
}

static void
print(FILE *f, const void *instance, void *type_env)
{
	const struct priq_scenario *scen;

	(void) type_env;

	scen = instance;

	for (size_t i = 0; i < scen->count; i++) {
		const struct priq_op *op;

		op = &scen->ops[i];
		switch (op->t) {
		case PRIQ_OP_POP:
			fprintf(f, "%zd: POP\n", i);
			break;

		case PRIQ_OP_PUSH:
			fprintf(f, "%zd: PUSH -- id %zd, cost %u\n",
				i, op->u.push.id, op->u.push.cost);
			break;

		case PRIQ_OP_UPDATE:
			fprintf(f, "%zd: UPDATE -- id %zd, cost %u\n",
				i, op->u.update.id, op->u.update.cost);
			break;

		case PRIQ_OP_MOVE:
			fprintf(f, "%zd: MOVE -- old_id %zd new_id %zd\n",
				i, op->u.move.old_id, op->u.move.new_id);
			break;

		default:
			assert(false); break;
		}
	}
}

struct theft_type_info type_info_adt_priq = {
	.alloc = alloc,
	.print = print,
	.free  = theft_generic_free_cb,
	.autoshrink_config = {
		.enable = true,
	}
};

