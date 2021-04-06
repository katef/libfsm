/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_adt_ipriq.h"

#include <adt/ipriq.h>
#include <adt/xalloc.h>

struct model {
	size_t used;
	size_t entries[];
};

static enum ipriq_cmp_res
cmp_size_t(size_t a, size_t b, void *opaque)
{
	(void)opaque;
	return a < b ? IPRIQ_CMP_LT :
	    a > b ? IPRIQ_CMP_GT : IPRIQ_CMP_EQ;
}

static int exec_add(size_t x, struct model *m, struct ipriq *pq)
{
	if (!ipriq_add(pq, x)) {
		return 0;
	}

	m->entries[m->used] = x;
	m->used++;
	return 1;
}

static int find_min_pos(const struct model *m, size_t *pos)
{
	size_t i;
	if (m->used == 0) {
		return 0;
	}

	size_t res, min;
	res = 0;
	min = m->entries[0];

	for (i = 1; i < m->used; i++) {
		if (m->entries[i] < min) {
			res = i;
			min = m->entries[i];
		}
	}
	*pos = res;
	return 1;
}

static int exec_peek(struct model *m, struct ipriq *pq)
{
	size_t res;

	if (!ipriq_peek(pq, &res)) {
		return m->used == 0;
	}

	size_t pos;
	if (!find_min_pos(m, &pos)) {
		assert(!"unreachable (peek)");
	}

	return res == m->entries[pos];
}

static int exec_pop(struct model *m, struct ipriq *pq)
{
	size_t res;

	if (!ipriq_pop(pq, &res)) {
		return m->used == 0;
	}

	size_t pos;
	if (!find_min_pos(m, &pos)) {
		assert(!"unreachable (pop)");
	}

	if (res != m->entries[pos]) {
		return 0;
	}

	assert(m->used > 0);
	if (pos < m->used - 1) {
		m->entries[pos] = m->entries[m->used - 1];
	}
	m->used--;
	return 1;
}

static enum theft_trial_res
compare_against_model(const struct ipriq_scenario *scen)
{
	enum theft_trial_res res = THEFT_TRIAL_FAIL;
	size_t i;

	struct model *m = malloc(sizeof(*m)
	    + scen->count * sizeof(m->entries[0]));
	if (m == NULL) {
		return THEFT_TRIAL_ERROR;
	}
	m->used = 0;

	struct ipriq *pq = ipriq_new(NULL, cmp_size_t, NULL);
	if (pq == NULL) {
		return THEFT_TRIAL_ERROR;
	}

	for (i = 0; i < scen->count; i++) {
		const struct ipriq_op *op = &scen->ops[i];

		switch (op->t) {
		case IPRIQ_OP_ADD:
			if (!exec_add(op->u.add.x, m, pq)) {
				goto cleanup;
			}
			break;

		case IPRIQ_OP_PEEK:
			if (!exec_peek(m, pq)) {
				goto cleanup;
			}
			break;

		case IPRIQ_OP_POP:
			if (!exec_pop(m, pq)) {
				goto cleanup;
			}
			break;

		default:
			assert(false); break;
		}
	}

	res = THEFT_TRIAL_PASS;

cleanup:
	free(m);

	return res;
}

static enum theft_trial_res
prop_ipriq_model(struct theft *t, void *arg1)
{
	const struct ipriq_scenario *scen = arg1;
	(void)t;
	return compare_against_model(scen);
}

static bool
test_ipriq(theft_seed seed, uintptr_t limit)
{
	enum theft_run_res res;

	struct ipriq_hook_env env = {
		.tag = 'I',
		.limit = limit,
	};

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_ipriq_model,
		.type_info = { &type_info_adt_ipriq },
		.trials = 1000,
		.hooks = {
			.trial_pre  = theft_hook_first_fail_halt,
			.env = &env,
		},
		.fork = {
			.enable = true,
		},

		.seed = seed,
	};

	(void)limit;

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));

	return res == THEFT_RUN_PASS;
}

void
register_test_adt_ipriq(void)
{
	reg_test1("adt_ipriq", test_ipriq, 10000);
}
