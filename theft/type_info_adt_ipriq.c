#include "type_info_adt_ipriq.h"

#include <adt/xalloc.h>

static enum theft_alloc_res
alloc(struct theft *t, void *type_env, void **output)
{
	struct ipriq_scenario *res;
	size_t limit;
	const uint8_t id_bits = 16;

	(void) type_env;

	struct ipriq_hook_env *env = theft_hook_get_env(t);
	assert(env->tag == 'I');

	limit = theft_random_choice(t, env->limit);

	res = xmalloc(sizeof *res + limit * sizeof *res->ops);

	res->count = 0;
	for (size_t i = 0; i < limit; i++) {
		struct ipriq_op *op;
		op = &res->ops[res->count];
		op->t = theft_random_choice(t, IPRIQ_OP_TYPE_COUNT);
		switch (op->t) {
		case IPRIQ_OP_PEEK:
		case IPRIQ_OP_POP:
			break;

		case IPRIQ_OP_ADD:
			op->u.add.x = theft_random_bits(t, id_bits);
			break;

		default:
			assert(false);
			break;
		}

		/* foothold to shrink away operations */
		if (theft_random_bits(t, 3) > 0) { /* keep? */
			res->count++;
		}
	}

	*output = res;

	return THEFT_ALLOC_OK;
}

static void
print(FILE *f, const void *instance, void *type_env)
{
	const struct ipriq_scenario *scen;

	(void) type_env;

	scen = instance;

	for (size_t i = 0; i < scen->count; i++) {
		const struct ipriq_op *op = &scen->ops[i];

		switch (op->t) {
		case IPRIQ_OP_ADD:
			fprintf(f, "%zd: ADD %zu\n", i, op->u.add.x);
			break;

		case IPRIQ_OP_PEEK:
			fprintf(f, "%zd: PEEK\n", i);
			break;

		case IPRIQ_OP_POP:
			fprintf(f, "%zd: POP\n", i);
			break;

		default:
			assert(false); break;
		}
	}
}

struct theft_type_info type_info_adt_ipriq = {
	.alloc = alloc,
	.print = print,
	.free  = theft_generic_free_cb,
	.autoshrink_config = {
		.enable = true,
	}
};
