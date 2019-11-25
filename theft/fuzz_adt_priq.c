/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_adt_priq.h"

#include <adt/priq.h>
#include <adt/xalloc.h>

struct record {
	size_t gen;
	unsigned int cost;
	size_t id;
	struct priq *entry;
};

struct model {
	struct priq_hook_env *env;
	size_t count;
	size_t used;
	struct record *records;
	size_t gen;
};

static bool
model_verify(struct model *m)
{
	/* TODO: other invariants? */

	(void) m;

	return true;
}

/*
 * Cast an ID to a fake state pointer, since the
 * priq doesn't ever dereference them, just compare
 * them by address.
 */
static fsm_state_t
fake_state_of_id(uintptr_t id)
{
	assert(id > 0);

	return (fsm_state_t) id;
}

static enum theft_trial_res
prop_priq_operations(struct theft *t, void *arg1);

enum theft_hook_trial_post_res
repeat_with_verbose(const struct theft_hook_trial_post_info *info,
	void *venv)
{
	struct priq_hook_env *env;

	env = venv;
	assert(env->tag == 'p');
	if (info->result == THEFT_TRIAL_FAIL) {
		env->verbosity = (info->repeat ? 0 : 1);
		return THEFT_HOOK_TRIAL_POST_REPEAT_ONCE;
	}

	return theft_hook_trial_post_print_result(info, &env->print_env);
}

static bool
test_priq_operations(uintptr_t limit, theft_seed seed)
{
	enum theft_run_res res;

	struct priq_hook_env env = {
		.tag = 'p',
	};

	static theft_seed always_seeds[] = {

		/*
		 *  -- Counter-Example: test_priq_operations
		 *     Trial 0, Seed 0x722c79fa639436a2
		 *     Argument 0:
		 * 0: PUSH -- id 1, cost 0
		 * 1: PUSH -- id 1, cost 1
		 * 2: POP
		 * 3: POP
		 */
		0x722c79fa639436a2LLU,

		/*
		 *  -- Counter-Example: test_priq_operations
		 *     Trial 0, Seed 0xef208a5d52bda7f4
		 *     Argument 0:
		 * 0: PUSH -- id 3, cost 0
		 * 1: UPDATE -- id 1, cost 0
		 */

		/* or, when priq_update(p, NULL, cost); is filtered out... */
		/*
		 *  -- Counter-Example: test_priq_operations
		 *     Trial 0, Seed 0xef208a5d52bda7f4
		 *     Argument 0:
		 * 0: PUSH -- id 1, cost 1
		 * 1: UPDATE -- id 1, cost 0
		 */
		0xef208a5d52bda7f4LLU,
	};

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_priq_operations,
		.type_info = { &type_info_adt_priq },
		.hooks = {
			.trial_pre  = theft_hook_first_fail_halt,
			.trial_post = repeat_with_verbose,
			.env = &env,
		},
		.fork = {
			.enable = true,
		},

		.seed = seed,
		.always_seed_count = sizeof always_seeds / sizeof *always_seeds,
		.always_seeds = always_seeds
	};

	(void) limit;

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));

	return res == THEFT_RUN_PASS;
}

static bool
op_pop(struct model *m, struct priq_op *op, struct priq **p)
{
	struct priq *old_p;
	struct priq *popped;
	size_t i;
	bool found;
	size_t ri;
	unsigned int lowest;

	assert(op->t == PRIQ_OP_POP);

	old_p = *p;
	popped = priq_pop(p);

	if (m->env->verbosity > 0) {
		fprintf(stdout, "POP: %p => %p, popped [%p, cost %u], %zd in model before popping\n",
		    (void *) old_p, (void *) *p, (void *) popped,
		    (popped == NULL ? 0 : popped->cost),
			m->used);
	}

	if (m->used == 0) {
		ASSERT(popped == NULL, "FAIL: pop should be empty, %p\n", (void *) popped);
		return true;
	}

	ASSERT(popped != NULL, "FAIL: pop shouldn't return empty, %zd entries remain\n", m->used);
	found = false;
	ri = 0;
	lowest = (unsigned int) -1;

	for (i = 0; i < m->used; i++) {
		struct record *r;

		r = &m->records[i];
		if (lowest > r->cost) {
			lowest = r->cost;
		}

		if (m->env->verbosity > 0) {
			fprintf(stderr, "== %zd: cost %u, id %zd, entry %p, gen %zd == "
				"looking for cost %u, state %zu, entry %p\n",
				i, r->cost, r->id, (void *) r->entry, r->gen,
				popped->cost, (uintptr_t) popped->state, (void *) popped);
		}

		if (r->cost == popped->cost && r->id == (uintptr_t) popped->state && r->entry == popped) {
			found = true;
			ri = i;
			if (m->env->verbosity > 0) {
				fprintf(stderr, "HIT\n");
			}
			break;
		}
	}
	ASSERT(found, "FAIL: popped unrecognized entry\n");
	ASSERT(lowest == popped->cost, "FAIL: did not pop lowest cost entry\n");

	if (ri < m->used - 1) {
		/* clobber this entry with last */
		memcpy(&m->records[ri], &m->records[m->used - 1], sizeof *m->records);
	}
	m->used--;

	return true;
}

static bool
op_push(struct model *m, struct priq_op *op, struct priq **p)
{
	fsm_state_t fake_state;
	struct priq *old_p, *new_node;
	struct priq *entry;
	unsigned int lowest;
	size_t i;

	assert(op->t == PRIQ_OP_PUSH);

	fake_state = fake_state_of_id(op->u.push.id);
	old_p = *p;
	new_node = priq_push(NULL, p, fake_state, op->u.push.cost);

	if (m->env->verbosity > 0) {
		fprintf(stdout, "PUSH: %p w/ [id %zd, cost %u] => %p, new_node %p\n",
		    (void *) old_p, op->u.push.id, op->u.push.cost,
		    (void *) *p, (void *) new_node);
	}

	ASSERT(new_node != NULL, "FAIL: push returned NULL\n");
	lowest = (unsigned int) -1;
	for (i = 0; i < m->used; i++) {
		struct record *r;

		r = &m->records[i];
		if (r->cost < lowest) {
			lowest = r->cost;
		}
	}

	ASSERT(new_node->cost == op->u.push.cost,
	    "PUSH: bad cost: exp %u, got %u\n", op->u.push.cost, new_node->cost);

	if (op->u.push.cost < (*p)->cost) {
		ASSERT(false, "FAIL: push -- head unchanged, head cost %u, just pushed %u\n",
		    (*p)->cost, op->u.push.cost);
	}

	entry = priq_find(*p, fake_state);
	ASSERT(entry != NULL, "FAIL: push -- could not find pushed entry\n");

	/* Add new record to model */
	m->records[m->used] = (struct record) {
		.id    = op->u.push.id,
		.cost  = op->u.push.cost,
		.entry = entry,
		.gen   = m->gen++,
	};
	if (m->env->verbosity > 0) {
		const struct record *r = &m->records[m->used];
		fprintf(stderr, "== Added record %zd: cost %u, id %zd, entry %p, gen %zd\n",
		    m->used, r->cost, r->id, (void *) r->entry, r->gen);
	}
	m->used++;

	return true;
}

static bool
op_update(struct model *m, struct priq_op *op, struct priq **p)
{
	struct priq *entry;
	struct priq *old_p;
	size_t i, ri;

	assert(op->t == PRIQ_OP_UPDATE);

	/* Find entry pointer with id in model, if any */
	entry = NULL;
	ri = 0;

	for (i = 0; i < m->used; i++) {
		if (m->records[i].id == op->u.update.id) {
			entry = m->records[i].entry;
			ri = i;
			break;
		}
	}

	if (entry == NULL) {
		return true; /* not found */
	}

	old_p = *p;
	if (m->env->verbosity > 0) {
		fprintf(stdout, "calling UPDATE: %p...entry [%p, cost %u], setting cost %u\n",
		    (void *) *p, (void *) entry,
		    (entry == NULL ? 0 : entry->cost),
			op->u.update.cost);
	}

	priq_update(p, entry, op->u.update.cost);

	if (m->env->verbosity > 0) {
		fprintf(stdout, "UPDATE: %p => %p, entry [%p, cost %u]\n",
		    (void *) old_p, (void *) *p, (void *) entry,
		    (entry == NULL ? 0 : entry->cost));
	}

	if (m->env->verbosity > 0) {
		fprintf(stdout, "UPDATE: %p w/ [id %zd, cost %u (%p)] => %p\n",
		    (void *) old_p, op->u.update.id, op->u.update.cost,
		    (void *) entry, (void *) *p);
	}

	if (entry == NULL || op->u.update.cost == entry->cost) {
		/* no change */
	} else {
		ASSERT(entry->cost == op->u.update.cost,
		    "FAIL: update -- no change (entry %p has cost %u, not %u)\n",
		    (void *) entry, entry->cost, op->u.update.cost);
	}

	if (entry != NULL) {
		m->records[ri].cost = op->u.update.cost;
	}

	return true;
}

static bool
op_move(struct model *m, struct priq_op *op, struct priq **p)
{
	assert(op->t == PRIQ_OP_MOVE);

	(void) m;
	(void) op;
	(void) p;

	/*
	 * TODO: The way this function is used is unclear;
	 * figure out how to model it before adding it to the test set.
	 */

	assert(false);
}

static enum theft_trial_res
prop_priq_operations(struct theft *t, void *arg1)
{
	struct priq_scenario *s = arg1;
	struct priq_hook_env *env;
	size_t op_i;
	struct priq *p;
	struct model *m;

	env = theft_hook_get_env(t);

	assert(env->tag == 'p');

	m = xmalloc(sizeof *m);

	m->env     = env;
	m->count   = s->count;
	m->used    = 0;
	m->records = xcalloc(s->count, sizeof *m->records);
	m->gen     = 0;

	if (m->records == NULL) {
		return THEFT_TRIAL_ERROR;
	}

	p = NULL;

	/* run operations */
	for (op_i = 0; op_i < s->count; op_i++) {
		struct priq_op *op;
		bool r;

		op = (struct priq_op *) &s->ops[op_i];
		switch (op->t) {
		case PRIQ_OP_POP:    r = op_pop   (m, op, &p); break;
		case PRIQ_OP_PUSH:   r = op_push  (m, op, &p); break;
		case PRIQ_OP_UPDATE: r = op_update(m, op, &p); break;
		case PRIQ_OP_MOVE:   r = op_move  (m, op, &p); break;

		default:
			assert(false);
			r = false;
			break;
		}

		if (!r) {
			return THEFT_TRIAL_FAIL;
		}
	}

	if (!model_verify(m)) {
		return THEFT_TRIAL_FAIL;
	}

	free(m->records);
	free(m);

	return THEFT_TRIAL_PASS;
}

void
register_test_adt_priq(void)
{
	reg_test1("adt_priq_operations", test_priq_operations, 100);
}

