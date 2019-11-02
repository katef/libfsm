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

typedef uintptr_t item_t;

#include "set.inc"

struct model {
	struct set_hook_env *env;
	size_t bit_count;
	uint64_t bits[];
};

static enum theft_trial_res prop_set_operations(struct theft *t, void *arg1);
static enum theft_trial_res prop_set_equality(struct theft *t, void *arg1);
static bool op_add(struct model *m, struct set_op *op, struct set *set);
static bool op_remove(struct model *m, struct set_op *op, struct set *set);
static bool op_clear(struct model *m, struct set *set);
static bool postcondition(struct model *m, struct set *set);

static int
cmp_item(const void *pa, const void *pb)
{
	uintptr_t a = (uintptr_t) pa;
	uintptr_t b = (uintptr_t) pb;

	assert(pa != NULL);
	assert(pb != NULL);

	/*
	 * This is intentional abuse of the set API; here we compare
	 * pointers by value, because for the purposes of our tests
	 * they do not point to any items.
	 */
	return a < b ? -1 : a > b ? 1 : 0;
}

static enum theft_hook_trial_post_res
repeat_with_verbose(const struct theft_hook_trial_post_info *info,
	void *venv)
{
	struct set_hook_env *env;

	env = venv;

	assert(env->tag == 's');

	if (info->result == THEFT_TRIAL_FAIL) {
		env->verbosity = (info->repeat ? 0 : 1);
		return THEFT_HOOK_TRIAL_POST_REPEAT_ONCE;
	}

	return theft_hook_trial_post_print_result(info, &env->print_env);
}

static bool
test_set_operations(theft_seed seed, uintptr_t limit2)
{
	enum theft_run_res res;

	struct set_hook_env env = {
		.tag = 's',
		.count_bits = limit2,
	};

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_set_operations,
		.type_info = { &type_info_adt_set_scenario },
		.hooks = {
			.trial_pre = theft_hook_first_fail_halt,
			.trial_post = repeat_with_verbose,
			.env = &env,
		},
		.seed = seed,
		.trials = 100000,
		.fork = {
			.enable = true,
			.timeout = 100,
		},
	};

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));

	return res == THEFT_RUN_PASS;
}

static bool
test_set_equality(theft_seed seed)
{
	enum theft_run_res res;

	struct set_hook_env env = {
		.tag = 's',
	};

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_set_equality,
		.type_info = { &type_info_adt_set_seq, },
		.hooks = {
			.trial_pre = theft_hook_first_fail_halt,
			.trial_post = repeat_with_verbose,
			.env = &env,
		},
		.seed = seed,
		.trials = 10000,
		.fork = {
			.enable = true,
			.timeout = 100,
		},
	};

	//theft_generate(stdout, 12345, &type_info_adt_set_seq, &env);

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));

	return res == THEFT_RUN_PASS;
}

static enum
theft_trial_res prop_set_equality(struct theft *t, void *arg1)
{
	struct set_hook_env *env;
	struct set_sequence *seq = arg1;
	uint16_t *permuted;
	struct set *set_a, *set_b;
	size_t i;

	env = theft_hook_get_env(t);
	assert(env->tag == 's');

	permuted = theft_info_adt_sequence_permuted(seq);

	set_a = set_create(NULL, cmp_item);
	set_b = set_create(NULL, cmp_item);

	for (i = 0; i < seq->count; i++) {
		assert(seq->values[i] != 0);
		assert(permuted[i] != 0);

		(void) set_add(set_a, (void *) ((uintptr_t) seq->values[i])); /* XXX */
		(void) set_add(set_b, (void *) ((uintptr_t) permuted[i]));
	}

	if (!set_equal(set_a, set_b)) {
		if (env->verbosity > 0) {
			fprintf(stderr, "SET_EQUAL failed\n");
		}
		return THEFT_TRIAL_FAIL;
	}

	if (0 != set_cmp(set_a, set_b)) {
		if (env->verbosity > 0) {
			fprintf(stderr, "SET_CMP != 0\n");
		}
		return THEFT_TRIAL_FAIL;
	}

	/* Other invariants: possibly iterate over both pairwise? */

	set_free(set_a);
	set_free(set_b);

	return THEFT_TRIAL_PASS;
}

static void
get_pos(uintptr_t item, size_t *offset, uint64_t *bit)
{
	*offset = item / 64;
	*bit = 1LLU << (item & 63);
}

static bool
op_add(struct model *m, struct set_op *op, struct set *set)
{
	size_t offset;
	uint64_t bit;

	assert(op->u.add.item != 0);

	(void) set_add(set, (void *) ((uintptr_t) op->u.add.item)); /* XXX */

	if (m->env->verbosity > 0) {
		fprintf(stderr, "ITEM IS %zu (decimal)\n", (size_t) op->u.add.item);
	}

	/* update model */
	get_pos(op->u.add.item, &offset, &bit);
	m->bits[offset] |= bit;

	return true;
}

static bool
op_remove(struct model *m, struct set_op *op, struct set *set)
{
	size_t offset;
	uint64_t bit;

	assert(op->u.remove.item != 0);

	set_remove(set, (void *) ((uintptr_t) op->u.remove.item));

	if (m->env->verbosity > 0) {
		fprintf(stderr, "REMOVE 0x%" PRIxPTR " to\n",
			op->u.remove.item);
	}

	/* update model */
	get_pos(op->u.add.item, &offset, &bit);
	m->bits[offset] &=~ bit;

	return true;
}

static bool
op_clear(struct model *m, struct set *set)
{
	if (m->env->verbosity > 0) {
		fprintf(stderr, "CLEAR\n");
	}

	set_clear(set);

	memset(m->bits, 0x00, m->bit_count / 8);

	return true;
}

static bool
postcondition(struct model *m, struct set *set)
{
	size_t actual_count;
	size_t model_count;
	const uint8_t *bits8;
	size_t i;
	uint8_t b;
	size_t model_first;

	// check set_count
	model_count = 0;
	bits8 = (const uint8_t *) m->bits;
	for (i = 0; i < m->bit_count/8; i++) {
		for (b = 0x01; b; b <<= 1) {
			/* could use a LUT */
			if (bits8[i] & b) {
				model_count++;
			}
		}
	}

	actual_count = set_count(set);
	ASSERT(model_count == actual_count,
		"POSTCONDITION: count mismatch, model %zd, set %zd\n",
		model_count, actual_count);

	ASSERT(set_empty(set) == (actual_count == 0),
		"POSTCONDITION: set_empty doesn't match count\n");

	model_first = (size_t) -1;
	for (i = 0; i < m->bit_count/64 && model_first == (size_t) -1; i++) {
		const uint64_t word = m->bits[i];
		if (word > 0) {
			for (uint8_t bit_i = 0; bit_i < 64; bit_i++) {
				if (word & (1LLU << bit_i)) {
					model_first = 64*i + bit_i;
					break;
				}
			}
		}
	}

	{
		struct set_iter it = {
			.set = set,
		};

		uintptr_t first_item = (uintptr_t) set_first(set, &it);

		if (model_first == (size_t) -1) {
			/* this is a bit weird */
			ASSERT(it.set == NULL, "set_first should indicate it's empty");
		} else {
			ASSERT(first_item == model_first,
				"POSTCONDITION: set_first returned 0x%" PRIxPTR " but model has 0x%" PRIxPTR "\n",
				first_item, model_first);
		}
	}

	return true;
}

static enum theft_trial_res
prop_set_operations(struct theft *t, void *arg1)
{
	struct set_scenario *s = arg1;
	struct set_hook_env *env;
	size_t item_ceil;
	struct model *m;
	struct set *set;
	size_t i;

	env = theft_hook_get_env(t);
	assert(env->tag == 's');

	item_ceil = 1LLU << env->item_bits;
	assert(item_ceil > 64);

	m = xmalloc(sizeof *m + (item_ceil / 8) + sizeof *m->bits);

	m->env       = env;
	m->bit_count = item_ceil;

	assert((m->bit_count % 64) == 0);

	memset(m->bits, 0x00, (item_ceil / 8) + sizeof *m->bits);

	set = set_create(NULL, cmp_item);
	if (set == NULL) {
		return THEFT_TRIAL_ERROR;
	}

	/* eval */
	for (i = 0; i < s->count; i++) {
		struct set_op *op;
		bool r;

		op = &s->ops[i];

		switch (op->t) {
		case SET_OP_ADD:    r = op_add   (m, op, set); break;
		case SET_OP_REMOVE: r = op_remove(m, op, set); break;
		case SET_OP_CLEAR:  r = op_clear (m, set);     break;

		default:
			return THEFT_TRIAL_ERROR;
		}

		if (!r) {
			return THEFT_TRIAL_FAIL;
		}

		if (!postcondition(m, set)) {
			return THEFT_TRIAL_FAIL;
		}
	}

	set_free(set);
	free(m);

	return THEFT_TRIAL_PASS;
}

void
register_test_adt_set(void)
{
	reg_test1("adt_set_operations", test_set_operations, 10);
	reg_test("adt_set_equality", test_set_equality);
}

