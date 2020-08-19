/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_adt_edge_set.h"

#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/xalloc.h>

/* Currently needed for struct fsm_edge. */
#include "../src/libfsm/internal.h"

static enum theft_trial_res
prop_model_check(struct theft *t, void *arg1);

static bool
prop_model_check_eval(const struct edge_set_op *op,
    struct edge_set **e, struct edge_set_model *m);

static bool
prop_model_check_postcondition(const struct edge_set *es,
    const struct edge_set_model *m);

static fsm_state_t
kept_rank(const uint64_t *kept, fsm_state_t i);

static fsm_state_t
kept_remap(fsm_state_t id, const void *opaque);

/* Test the edge_set by evaluating a randomly generated
 * series of operations, then comparing the result after
 * each step with a simpler model implementation (which
 * adds/removes/updates a dynamically resized array). */
static bool
test_edge_set_operations(theft_seed seed,
    uintptr_t ceil_bits, uintptr_t symbol_bits, uintptr_t state_bits)
{
	enum theft_run_res res;

	struct edge_set_ops_env ops_env = {
		.tag = 'E',
		.ceil_bits = (uint8_t)ceil_bits,
		.symbol_bits = (uint8_t)symbol_bits,
		.state_bits = (uint8_t)state_bits,
	};

	const char *gen_seed = getenv("GEN_SEED");

	if (gen_seed != NULL) {
		seed = strtoul(gen_seed, NULL, 0);
		theft_generate(stderr, seed,
		    &type_info_adt_edge_set_ops, &ops_env);
		return true;
	}

	struct theft_run_config config = {
		.name = __func__,
		.prop1 = prop_model_check,
		.type_info = { &type_info_adt_edge_set_ops, },
		.hooks = {
			.trial_pre = theft_hook_first_fail_halt,
			.env = &ops_env,
		},
		.seed = seed,
		.trials = 10000,
		.fork = {
			.enable = true,
			.timeout = 1000,
		},
	};

	res = theft_run(&config);
	printf("%s: %s\n", __func__, theft_run_res_str(res));
	return res == THEFT_RUN_PASS;
}

static enum theft_trial_res
ops_model_check(const struct edge_set_ops *ops)
{
	enum theft_trial_res res = THEFT_TRIAL_FAIL;

	const struct fsm_alloc *alloc = NULL; /* use defaults */

	struct edge_set_model m = {
		.count = 0,
		.ceil = DEF_MODEL_CEIL,
	};
	m.edges = malloc(m.ceil * sizeof(m.edges[0]));
	if (m.edges == NULL) { return THEFT_TRIAL_ERROR; }

	struct edge_set *es = edge_set_new();

	for (size_t op_i = 0; op_i < ops->count; op_i++) {
		if (!prop_model_check_eval(&ops->ops[op_i], &es, &m)) {
			goto cleanup;
		}
		if (!prop_model_check_postcondition(es, &m)) {
			goto cleanup;
		}
	}

	res = THEFT_TRIAL_PASS;
cleanup:
	edge_set_free(alloc, es);
	free(m.edges);
	return res;
}

static enum theft_trial_res
prop_model_check(struct theft *t, void *arg1)
{
	(void)t;
	struct edge_set_ops *ops = arg1;
	return ops_model_check(ops);
}

static bool
grow_model(struct edge_set_model *m)
{
	size_t nceil = 2*m->ceil;
	assert(nceil > m->ceil);
	struct model_edge *nedges = realloc(m->edges,
	    nceil * sizeof(m->edges[0]));
	if (nedges == NULL) { return false; }

	m->ceil = nceil;
	m->edges = nedges;
	return true;
}

static bool
prop_model_check_eval(const struct edge_set_op *op,
    struct edge_set **es, struct edge_set_model *m)
{
	switch (op->t) {
	case ESO_ADD:
		if (m->count == m->ceil) {
			if (!grow_model(m)) {
				return false;
			}
		}

		/* only add if not already present */
		bool present = false;
		for (size_t i = 0; i < m->count; i++) {
			if (m->edges[i].symbol == op->u.add.symbol
			    && m->edges[i].state == op->u.add.state) {
				present = true;
				break;
			}
		}

		if (!present) {
			m->edges[m->count].symbol = op->u.add.symbol;
			m->edges[m->count].state = op->u.add.state;
			m->count++;
		}

		if (!edge_set_add(es, NULL,
			op->u.add.symbol, op->u.add.state)) {
			return false;
		}
		break;

	case ESO_ADD_STATE_SET:
	{
		/* grow model to fit entire state set */
		while (m->count + op->u.add_state_set.state_count > m->ceil) {
			if (!grow_model(m)) {
				return false;
			}
		}

		struct state_set *state_set = NULL;

		for (size_t s_i = 0; s_i < op->u.add_state_set.state_count; s_i++) {
			bool present = false;
			const unsigned char symbol = op->u.add_state_set.symbol;
			const fsm_state_t state = op->u.add_state_set.states[s_i];
			for (size_t i = 0; i < m->count; i++) {
				if (m->edges[i].symbol == symbol
				    && m->edges[i].state == state) {
					present = true;
					break;
				}
			}

			if (!present) {
				m->edges[m->count].symbol = symbol;
				m->edges[m->count].state = state;
				m->count++;
			}

			if (!state_set_add(&state_set, NULL, state)) {
				return false;
			}
		}

		if (!edge_set_add_state_set(es, NULL,
			op->u.add_state_set.symbol,
			state_set)) {
			return false;
		}

		state_set_free(state_set);
		break;
	}

	case ESO_REMOVE:
	{
		/* remove all with same symbol */
		size_t i = 0;
		while (i < m->count) {
			if (m->edges[i].symbol == op->u.remove.symbol) {
				if (i < m->count - 1) {
					memcpy(&m->edges[i],
					    &m->edges[m->count - 1],
					    sizeof(m->edges[i]));
				}
				m->count--;
			} else {
				i++;
			}
		}
		edge_set_remove(es, op->u.remove.symbol);
		break;
	}

	case ESO_REMOVE_STATE:
	{
		/* remove all instances of state */
		size_t i = 0;
		while (i < m->count) {
			if (m->edges[i].state == op->u.remove_state.state) {
				if (i < m->count - 1) {
					memcpy(&m->edges[i],
					    &m->edges[m->count - 1],
					    sizeof(m->edges[i]));
				}
				m->count--;
			} else {
				i++;
			}
		}
		edge_set_remove_state(es, op->u.remove_state.state);
		break;
	}

	case ESO_REPLACE_STATE:
	{
		/* replace all instances of state */
		for (size_t i = 0; i < m->count; i++) {
			if (m->edges[i].state == op->u.replace_state.old) {
				m->edges[i].state = op->u.replace_state.new;
			}
		}

		/* if this introduced any duplicates, remove them */
		size_t i = 0;
		while (i < m->count) {
			size_t removed = 0;
			if (m->edges[i].state != op->u.replace_state.new) {
				i++;
				continue;
			}
			for (size_t j = i + 1; j < m->count; j++) {
				if (m->edges[j].state == op->u.replace_state.new
				    && m->edges[j].symbol == m->edges[i].symbol) {
					if (j < m->count - 1) {
						memcpy(&m->edges[j],
						    &m->edges[m->count - 1],
						    sizeof(m->edges[i]));
					}
					removed++;
					m->count--;
				}
			}

			if (removed == 0) {
				i++;
			}
		}

		edge_set_replace_state(es,
		    op->u.replace_state.old,
		    op->u.replace_state.new);
		break;
	}

	case ESO_COMPACT:
	{
		size_t i = 0;
		const uint64_t *kept = op->u.compact.kept;
		while (i < m->count) {
			fsm_state_t state = m->edges[i].state;
			if (kept[state/64] & ((uint64_t)1 << (state & 63))) {
				/* keep edge, but update state ID to rank */
				fsm_state_t nstate = kept_rank(kept, state);
				m->edges[i].state = nstate;
				i++;
			} else {
				/* remove edge */
				if (i < m->count - 1) {
					memcpy(&m->edges[i],
					    &m->edges[m->count - 1],
					    sizeof(m->edges[i]));
				}
				m->count--;
			}
		}

		/* By construction, this does not cover the case where
		 * compaction produces duplicates. That's a misuse of
		 * the function -- compaction should remove states and
		 * shift IDs down, but not combine multiple edges that
		 * were previously distinct. The function currently does
		 * not detect and reject that. */
		edge_set_compact(es,
		    kept_remap, kept);

		break;
	}
	case ESO_COPY:
	{
		struct edge_set *dst = NULL;
		if (!edge_set_copy(&dst, NULL, *es)) {
			return false;
		}
		edge_set_free(NULL, *es);
		*es = dst;

		/* no impact on model */
		break;
	}
	default:
		assert(!"match fail");
		return false;
	}

	return true;
}

/* Count how many 1 bits there are in the bit-set before s.
 * This corresponds to how many states before it were kept, so
 * its new ID no longer counts the states that were removed.
 *
 * This is kind of expensive and could be precomputed to block
 * boundaries, but it's test code. */
static fsm_state_t
kept_rank(const uint64_t *kept, fsm_state_t s)
{
	fsm_state_t res = 0;
	assert(s < (64 * MAX_KEPT));

	for (size_t i = 0; i < s; i++) {
		if (kept[i/64] & ((uint64_t)1 << (i&63))) {
			res++;
		}
	}
	return res;
}

/* A state's new ID is nothing, if not kept, otherwise it's
 * the new rank (number of kept states preceding it). */
static fsm_state_t
kept_remap(fsm_state_t id, const void *opaque)
{
	const uint64_t *kept = opaque;
	if (kept[id/64] & ((uint64_t)1 << (id&63))) {
		return kept_rank(kept, id);
	} else {
		return FSM_STATE_REMAP_NO_STATE;
	}
}

static bool
prop_model_check_postcondition(const struct edge_set *es,
    const struct edge_set_model *m) {

	/* same count and empty/non-empty state*/
	const size_t es_count = edge_set_count(es);

#define POSTCONDITION_LOG_COUNTS 0
	if (POSTCONDITION_LOG_COUNTS) {
		fprintf(stderr, "-- postcondition: es_count %zu, model_count %zu\n",
		    es_count, m->count);
	}

	if (es_count != m->count) {
		fprintf(stderr, "-- fail: count mismatch: %zu (es) vs %zu (model)\n",
		    edge_set_count(es), m->count);
		return false;
	}
	if (edge_set_empty(es) != (m->count == 0)) {
		fprintf(stderr, "-- fail: edge_set_empty disagrees with model\n");
		return false;
	}

	struct fsm_edge e;
	struct edge_ordered_iter eoi;

	/* every edge in m should be in es */
	for (size_t i = 0; i < m->count; i++) {
		bool found = false;

		/* There can be multiple edges associated with the same
		 * symbol (to different states), so if `edge_set_find`
		 * gets a different state, it isn't necessarily a
		 * mismatch. `edge_set_find` should be used with DFAs.
		 * In this case we need to iterate, but we can still
		 * reset the ordered iterator to a particular symbol. */
		for (edge_set_ordered_iter_reset_to(es, &eoi, m->edges[i].symbol);
		     edge_set_ordered_iter_next(&eoi, &e); ) {
			if (e.symbol == m->edges[i].symbol
			    && e.state == m->edges[i].state) {
				found = true;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "-- fail: matching edge for symbol 0x%x, state %d not found\n",
			    m->edges[i].symbol, m->edges[i].state);
			return false;
		}

		if (!edge_set_find(es, m->edges[i].symbol, &e)) {
			fprintf(stderr, "-- fail: edge for symbol 0x%x not found\n",
			    m->edges[i].symbol);
			return false;
		}
	}

	/* unordered iteration has same count */
	struct edge_iter ei;
	size_t count = 0;
	for (edge_set_reset(es, &ei);
	     edge_set_next(&ei, &e); ) {
		count++;
	}
	if (count != m->count) {
		fprintf(stderr, "-- fail: unordered iter count mismatch: exp %zu, got %zu\n",
		    m->count, count);
		return false;
	}

	/* ordered iteration has same count */
	count = 0;
	for (edge_set_ordered_iter_reset(es, &eoi);
	     edge_set_ordered_iter_next(&eoi, &e); ) {
		count++;
	}
	if (count != m->count) {
		fprintf(stderr, "-- fail: unordered iter count mismatch: exp %zu, got %zu\n",
		    m->count, count);
		return false;
	}

	return true;
}

static bool
regression0(theft_seed unused)
{
	(void)unused;
	struct edge_set_op op_list[] = {
		{		/* add once */
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 0, },
		},
		{		/* redundant add */
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 0, },
		},
		{		/* remove */
			.t = ESO_REMOVE,
			.u.remove = { .symbol = 0, },
		},
		{		/* redundant remove */
			.t = ESO_REMOVE,
			.u.add = { .symbol = 0, },
		},
		{		/* add back */
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 0, },
		},
	};
	struct edge_set_ops ops = {
		.count = sizeof(op_list)/sizeof(op_list[0]),
		.ops = op_list,
	};

	enum theft_trial_res res = ops_model_check(&ops);
	return res == THEFT_TRIAL_PASS;
}

static bool
regression1(theft_seed unused)
{
	(void)unused;
	struct edge_set_op op_list[] = {
		{
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 1, },
		},
		{
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 0, },
		},
		{
			.t = ESO_REMOVE,
			.u.remove = { .symbol = 0, },
		},
	};
	struct edge_set_ops ops = {
		.count = sizeof(op_list)/sizeof(op_list[0]),
		.ops = op_list,
	};

	enum theft_trial_res res = ops_model_check(&ops);
	return res == THEFT_TRIAL_PASS;
}

static bool
regression2(theft_seed unused)
{
	(void)unused;
	struct edge_set_op op_list[] = {
		{
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 32, },
		},
		{
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 0, },
		},
		{
			.t = ESO_REMOVE_STATE,
			.u.remove_state = { .state = 32, },
		},
		{
			.t = ESO_ADD,
			.u.add = { .symbol = 0, .state = 0, },
		},
	};
	struct edge_set_ops ops = {
		.count = sizeof(op_list)/sizeof(op_list[0]),
		.ops = op_list,
	};

	enum theft_trial_res res = ops_model_check(&ops);
	return res == THEFT_TRIAL_PASS;
}

static bool
regression3(theft_seed unused)
{
	(void)unused;

	/* edge_set_add_state_set could previously be used
	 * to sneak in duplicate edges. */
	struct edge_set_op op_list[] = {
		{
			.t = ESO_ADD_STATE_SET,
			.u.add_state_set = {
				.symbol = 0x00,
				.state_count = 2,
				.states = { 0, 13 },
			},
		},
		{
			.t = ESO_REPLACE_STATE,
			.u.replace_state = {
				.old = 0,
				.new = 13,
			},
		},
		{
			.t = ESO_COPY
		},
	};
	struct edge_set_ops ops = {
		.count = sizeof(op_list)/sizeof(op_list[0]),
		.ops = op_list,
	};

	enum theft_trial_res res = ops_model_check(&ops);
	return res == THEFT_TRIAL_PASS;
}

static bool
regression4(theft_seed unused)
{
	(void)unused;

	/* A series of calls to edge_set_add and edge_set_compact could
	 * fill the hash table with entries and tombstones without growing,
	 * which caused ordered iteration to get stuck in an infinite loop,
	 * or add to trigger an assert (because it didn't replace the first
	 * tombstone after wrapping the table). */
	struct edge_set_op op_list[] = {
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x1,
				.state = 0,
			},
		},
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x0,
				.state = 0,
			},
		},
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x0,
				.state = 1,
			},
		},
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x0,
				.state = 2,
			},
		},

		{
			.t = ESO_COMPACT,
			.u.compact.kept = { 0xc },
		},

		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x5,
				.state = 0,
			},
		},

		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x4,
				.state = 0,
			},
		},

		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x3,
				.state = 0,
			},
		},

		{
			.t = ESO_COMPACT,
			.u.compact.kept = { 0x0 },
		},

		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x7,
				.state = 0,
			},
		},
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x0,
				.state = 0,
			},
		},
	};
	struct edge_set_ops ops = {
		.count = sizeof(op_list)/sizeof(op_list[0]),
		.ops = op_list,
	};

	enum theft_trial_res res = ops_model_check(&ops);
	return res == THEFT_TRIAL_PASS;
}

static bool
regression5(theft_seed unused)
{
	(void)unused;

	/* While adding edge_set_ordered_iter_reset_to, theft
	 * found that the eoi->pos setting for the first bucket
	 * with matching symbol was wrong. It may have been
	 * asymptomatic until now, but this specific case causes
	 * the (0x2 -> 0) edge to get lost. */
	struct edge_set_op op_list[] = {
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x02,
				.state = 0,
			},
		},
		{
			.t = ESO_ADD,
			.u.add = {
				.symbol = 0x00,
				.state = 0,
			},
		},
	};
	struct edge_set_ops ops = {
		.count = sizeof(op_list)/sizeof(op_list[0]),
		.ops = op_list,
	};

	enum theft_trial_res res = ops_model_check(&ops);
	return res == THEFT_TRIAL_PASS;
}

void
register_test_adt_edge_set(void)
{
	reg_test3("adt_edge_set_operations", test_edge_set_operations,
	    10, 6, 6);

	reg_test("adt_edge_set_regression0", regression0);
	reg_test("adt_edge_set_regression1", regression1);
	reg_test("adt_edge_set_regression2", regression2);
	reg_test("adt_edge_set_regression3", regression3);
	reg_test("adt_edge_set_regression4", regression4);
	reg_test("adt_edge_set_regression5", regression5);
}
