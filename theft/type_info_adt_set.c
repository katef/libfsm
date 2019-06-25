/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_adt_set.h"

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>
#include <adt/xalloc.h>

static enum theft_alloc_res
alloc_scen(struct theft *t, void *type_env, void **output)
{
	struct set_hook_env *env;
	struct set_scenario *res;
	size_t count;

	env = theft_hook_get_env(t);

	assert(env->tag == 's');

	(void) type_env;

	if (env->count_bits == 0) {
		env->count_bits = 10;
	}

	if (env->item_bits == 0) {
		env->item_bits = env->count_bits - 2;
	}

	count = theft_random_bits(t, env->count_bits) + 1;

	res = xmalloc(sizeof *res + count * sizeof *res->ops);

	res->count = count;

	for (size_t i = 0; i < count; i++) {
		struct set_op *op;

		op = &res->ops[i];

		op->t = theft_random_choice(t, SET_OP_TYPE_COUNT);

		switch (op->t) {
		case SET_OP_ADD:
			op->u.add.item = theft_random_bits(t, env->item_bits);
			if (op->u.add.item == 0) {
				goto skip;
			}
			break;

		case SET_OP_REMOVE:
			op->u.remove.item = theft_random_bits(t, env->item_bits);
			if (op->u.remove.item == 0) {
				goto skip;
			}
			break;

		case SET_OP_CLEAR:
			break;

		default:
			assert(false);
			break;
		}
	}

	*output = res;

	return THEFT_ALLOC_OK;

skip:

	free(res);

	return THEFT_ALLOC_SKIP;
}

static void
print_scen(FILE *f, const void *instance, void *type_env)
{
	const struct set_scenario *scen;

	(void) type_env;

	scen = instance;

	for (size_t i = 0; i < scen->count; i++) {
		const struct set_op *op;

		op = &scen->ops[i];
		switch (op->t) {
		case SET_OP_ADD:
			fprintf(f, "%zd: ADD 0x%" PRIxPTR "\n", i, op->u.add.item);
			break;

		case SET_OP_REMOVE:
			fprintf(f, "%zd: REMOVE 0x%" PRIxPTR "\n", i, op->u.remove.item);
			break;

		case SET_OP_CLEAR:
			fprintf(f, "%zd: CLEAR\n", i);
			break;

		default:
			assert(false);
			break;
		}
	}
}

const struct theft_type_info type_info_adt_set_scenario = {
	.alloc = alloc_scen,
	.free = theft_generic_free_cb,
	.print = print_scen,
	.autoshrink_config = {
		.enable = true,
	},
};

static enum theft_alloc_res
alloc_seq(struct theft *t, void *type_env, void **output)
{
	struct set_hook_env *env;
	struct set_sequence *res;
	size_t count;

	(void) type_env;

	env = theft_hook_get_env(t);
	assert(env->tag == 's');

	if (env->count_bits == 0) {
		env->count_bits = 10;
	}

	if (env->item_bits == 0) {
		env->item_bits = env->count_bits - 2;
	}

	count = theft_random_bits(t, env->count_bits) + 1;

	res = xmalloc(sizeof *res + count * sizeof *res->values);

	res->count        = count;
	res->shuffle_seed = theft_random_bits(t, 8);

	for (size_t i = 0; i < count; i++) {
		res->values[i] = theft_random_bits(t, 16);
		if (res->values[i] == 0) {
			goto skip;
		}
	}

	*output = res;

	return THEFT_ALLOC_OK;

skip:

	free(res);

	return THEFT_ALLOC_SKIP;
}

static void
print_seq(FILE *f, const void *instance, void *type_env)
{
	const struct set_sequence *seq = instance;
	uint16_t *permuted;

	(void) type_env;

	fprintf(f, "seq %p: shuffle_seed %u, count %zd:\n",
		(void *) seq, seq->shuffle_seed, seq->count);

	fprintf(f, "== original order ==\n");
	for (size_t i = 0; i < seq->count; i++) {
		fprintf(f, "%u, ", seq->values[i]);
		if ((i & 7) == 7) {
			fprintf(f, "\n");
		}
	}
	if ((seq->count & 7) != 0) {
		fprintf(f, "\n");
	}

	fprintf(f, "== permuted ==\n");
	permuted = theft_info_adt_sequence_permuted((struct set_sequence *) seq);
	for (size_t i = 0; i < seq->count; i++) {
		fprintf(f, "%u, ", permuted[i]);
		if ((i & 7) == 7) {
			fprintf(f, "\n");
		}
	}
	if ((seq->count & 7) != 0) {
		fprintf(f, "\n");
	}
	free(permuted);
}

const struct theft_type_info type_info_adt_set_seq = {
	.alloc = alloc_seq,
	.free  = theft_generic_free_cb,
	.print = print_seq,
	.autoshrink_config = {
		.enable = true,
	}
};

uint16_t *
theft_info_adt_sequence_permuted(struct set_sequence *seq)
{
	uint16_t *res;
	size_t *pv;

	/* Generate a permutation vector, to shuffle */
	pv = gen_permutation_vector(seq->count, seq->shuffle_seed);
	if (pv == NULL) {
		return NULL;
	}

	res = xmalloc(seq->count * sizeof *res);

	for (size_t i = 0; i < seq->count; i++) {
		assert(pv[i] < seq->count);
		res[i] = seq->values[pv[i]];
	}

	free(pv);

	return res;
}

