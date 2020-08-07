#include "type_info_adt_edge_set.h"

#include <ctype.h>
#include <inttypes.h>

static enum theft_alloc_res
edge_set_ops_alloc(struct theft *t, void *penv, void **output)
{
	(void)penv;
	const struct edge_set_ops_env *env = theft_hook_get_env(t);

#define GET_BITS(FIELD, DEF) (uint8_t)(env-> FIELD ? env -> FIELD : DEF)
	assert(env->tag == 'E');
	const uint8_t ceil_bits = GET_BITS(ceil_bits, DEF_CEIL_BITS);
	const uint8_t symbol_bits = GET_BITS(symbol_bits, DEF_SYMBOL_BITS);
	const uint8_t state_bits = GET_BITS(state_bits, DEF_STATE_BITS);

	const size_t ceil = theft_random_bits(t, ceil_bits);

	struct edge_set_ops *res = malloc(sizeof(*res));
	if (res == NULL) { return THEFT_ALLOC_ERROR; }
	memset(res, 0x00, sizeof(*res));

	const size_t alloc_size = ceil * sizeof(res->ops[0]);
	res->ops = malloc(alloc_size);
	if (res == NULL) { return THEFT_ALLOC_ERROR; }
	memset(res->ops, 0x00, alloc_size);

	for (size_t op_i = 0; op_i < ceil; op_i++) {
		struct edge_set_op *op = &res->ops[res->count];
		memset(op, 0x00, sizeof(*op));

		op->t = theft_random_choice(t, ESO_TYPE_COUNT);
		switch (op->t) {
		case ESO_ADD:
			op->u.add.symbol = theft_random_bits(t, symbol_bits);
			op->u.add.state = theft_random_bits(t, state_bits);
			break;
		case ESO_ADD_STATE_SET:
			op->u.add_state_set.symbol = theft_random_bits(t, symbol_bits);
			op->u.add_state_set.state_count = theft_random_choice(t, MAX_STATE_SET);
			for (size_t i = 0; i < MAX_STATE_SET; i++) {
				op->u.add_state_set.states[i] = theft_random_bits(t, state_bits);
			}
			break;
		case ESO_REMOVE:
			op->u.remove.symbol = theft_random_bits(t, symbol_bits);
			break;
		case ESO_REMOVE_STATE:
			op->u.remove_state.state = theft_random_bits(t, state_bits);
			break;
		case ESO_REPLACE_STATE:
			op->u.replace_state.old = theft_random_bits(t, state_bits);
			op->u.replace_state.new = theft_random_bits(t, state_bits);
			break;
		case ESO_COMPACT:
			for (size_t i = 0; i < MAX_KEPT; i++) {
				op->u.compact.kept[i] = theft_random_bits(t, 64);
			}
			break;
		case ESO_COPY:
			break;	/* no arguments */
		default:
			assert(!"match fail");
			return THEFT_ALLOC_ERROR;
		}

		/* small odds of randomly discarding result,
		 * which will increase during shrinking */
		const bool keep = theft_random_bits(t, 3) > 0;
		if (keep) { res->count++; }
	}

	*output = res;
	return THEFT_ALLOC_OK;
}

static void
edge_set_ops_free(void *instance, void *env)
{
	struct edge_set_ops *ops = instance;
	free(ops->ops);
	free(ops);
	(void)env;
}

static void
edge_set_ops_print
(FILE *f, const void *instance, void *env)
{
	(void)env;
	const struct edge_set_ops *ops = instance;
	fprintf(f, "== edge_set_ops#%p(%zu):\n",
	    (void *)ops, ops->count);

	for (size_t op_i = 0; op_i < ops->count; op_i++) {
		const struct edge_set_op *op = &ops->ops[op_i];
		fprintf(f, "  - %zu: ", op_i);
		switch (op->t) {
		case ESO_ADD:
			fprintf(f, "ADD 0x%x (%c) -> %d\n",
			    op->u.add.symbol,
			    isprint(op->u.add.symbol) ? op->u.add.symbol : '.',
			    op->u.add.state);
			break;
		case ESO_ADD_STATE_SET:
			fprintf(f, "ADD_STATE_SET: 0x%x (%c) -> [",
			    op->u.add_state_set.symbol,
			    isprint(op->u.add_state_set.symbol)
			    ? op->u.add_state_set.symbol : '.');
			for (size_t i = 0; i < op->u.add_state_set.state_count; i++) {
				fprintf(f, "%s%d",
				    i > 0 ? ", " : "",
				    op->u.add_state_set.states[i]);
			}
			fprintf(f, "]\n");
			break;
		case ESO_REMOVE:
			fprintf(f, "REMOVE 0x%x (%c)\n",
			    op->u.remove.symbol,
			    isprint(op->u.remove.symbol) ? op->u.remove.symbol : '.');
			break;
		case ESO_REMOVE_STATE:
			fprintf(f, "REMOVE_STATE %d\n",
			    op->u.remove_state.state);
			break;
		case ESO_REPLACE_STATE:
			fprintf(f, "REPLACE_STATE %d -> %d\n",
			    op->u.replace_state.old,
			    op->u.replace_state.new);
			break;
		case ESO_COMPACT:
			fprintf(f, "COMPACT: ");
			for (size_t i = 0; i < MAX_KEPT; i++) {
				fprintf(f, "%016"PRIx64"",
				    op->u.compact.kept[i]);
			}
			fprintf(f, "\n");
			break;
		case ESO_COPY:
			fprintf(f, "COPY\n");
			break;
		default:
			assert(!"match fail");
		}
	}
}

const struct theft_type_info type_info_adt_edge_set_ops = {
	.alloc = edge_set_ops_alloc,
	.free  = edge_set_ops_free,
	.print = edge_set_ops_print,
	.autoshrink_config = {
		.enable = true,
	}
};
