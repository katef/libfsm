/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_nfa.h"

#include <ctype.h>

#include <adt/xalloc.h>

/*
 * Expose the NFA-of-PRNG-stream callback, because it's used
 * as part of other tests too.
 */
enum theft_alloc_res
type_info_nfa_alloc(struct theft *t, void *type_env, void **output)
{
	struct nfa_spec *res;
	struct nfa_state **states;
	uint8_t op_count;

	/* These could be configurable once all known issues are resolved */
	const uint8_t state_bits = 8;
	const uint8_t edge_bits  = 4;
	const uint8_t end_bits   = 4;

	/* This can get truncated later */
	size_t state_count = theft_random_bits(t, state_bits);
	size_t state_ceil2 = 1;
	while (state_ceil2 < state_count) {
		state_ceil2 <<= 1;
	}

	(void) type_env;

	res = xmalloc(sizeof *res);

	res->tag    = 'N';
	res->states = xmalloc(state_count * sizeof *states);

	op_count = theft_random_bits(t, NFA_OPS_BITS);
	for (size_t i = 0; i < op_count; i++) {
		res->ops[i] = theft_random_choice(t, NFA_OP_TYPE_COUNT);
	}

	/* forward-prune so operations are only applied once */
#define UNIQUE_OPS
#ifdef UNIQUE_OPS
	for (size_t i = 0; i < op_count; i++) {
		if (res->ops[i] == NFA_OP_NOP) {
			continue;
		}

		for (size_t j = i + 1; j < op_count; j++) {
			if (res->ops[i] == res->ops[j]) {
				res->ops[j] = NFA_OP_NOP;
			}
		}
	}
#endif

	for (size_t i = 0; i < state_count; i++) {
		struct nfa_state *state;
		size_t edge_count;

		/*
		 * Introduce a second way to reduce the state count;
		 * this should make shrinking a lot faster.
		 */
		if (0 == theft_random_bits(t, 7)) {
			state_count = i;
			break;
		}

		edge_count = theft_random_bits(t, edge_bits);

		state = xmalloc(sizeof *state + edge_count * sizeof *state->edges);

		state->id = i;

		/*
		 * 1 in (1<<N) odds of being an end state; using the highest
		 * value means it will get simplified away quickly
		 */
		state->is_end = ((1U << end_bits) - 1 == theft_random_bits(t, end_bits));
		state->shuffle_seed = theft_random_bits(t, 29);

		for (size_t ei = 0; ei < edge_count; ei++) {
			struct nfa_edge *e;

			if (0 == theft_random_bits(t, 7)) {
				edge_count = ei;
				break;
			}

			assert(ei < edge_count);

			e = &state->edges[ei];
			e->t = theft_random_bits(t, 2);

			/* Make literals more common, but quickly simplified away */
			if (e->t > NFA_EDGE_LITERAL) {
				e->t = NFA_EDGE_LITERAL;
			}
			assert(e->t <= NFA_EDGE_LITERAL);

			if (e->t == NFA_EDGE_LITERAL) {
				e->u.literal.byte = theft_random_bits(t, 8);
			}

			e->to = theft_random_choice(t, state_count);
		}

		state->edge_count = edge_count;

		res->states[i]   = state;
	}

	res->state_count = state_count;

	*output = res;

	return THEFT_ALLOC_OK;
}

static void
free_nfa_spec(void *instance, void *type_env)
{
	struct nfa_spec *spec = instance;

	(void) type_env;

	for (size_t i = 0; i < spec->state_count; i++) {
		free(spec->states[i]);
	}

	free(spec);
}

//#define AS_C

static void
print_nfa_spec(FILE *f, const void *instance, void *type_env)
{
	const struct nfa_spec *spec = instance;

	(void) type_env;

#ifdef AS_C
	fprintf(f, "struct fsm *nfa = fsm_new(&fsm_options);\n");
	fprintf(f, "fsm_state_t states[%zd] = { 0 };\n",
		spec->state_count);

	fprintf(f, "\n// first pass: states\n");
	for (size_t i = 0; i < spec->state_count; i++) {
		fprintf(f, "(void) fsm_addstate(nfa, &states[%zd]); ", i);
		if (spec->states[i]->is_end) {
			fprintf(f, "fsm_setend(nfa, states[%zd], 1);\n", i);
		} else {
			fprintf(f, "\n");
		}
	}
	fprintf(f, "fsm_setstart(nfa, states[0]);\n");

	fprintf(f, "\n// second pass: edges\n");

	for (size_t si = 0; si < spec->state_count; si++) {
		struct nfa_state *s;

		s = spec->states[si];

		for (size_t ei = 0; ei < s->edge_count; ei++) {
			struct nfa_edge *e;
			size_t to;

			e = &s->edges[ei];
			to = e->to % spec->state_count;

			switch (e->t) {
			case NFA_EDGE_EPSILON:
				fprintf(f,
				    "if (!fsm_addedge_epsilon(nfa, states[%zd], %zd)) { assert(false); }\n",
				    si, to);
				break;

			case NFA_EDGE_ANY:
				fprintf(f,
				    "if (!fsm_addedge_any(nfa, states[%zd], %zd)) { assert(false); }\n",
				    si, to);
				break;

			case NFA_EDGE_LITERAL:
				fprintf(f,
				    "if (!fsm_addedge_literal(nfa, states[%zd], %zd, (char)0x%02x)) { assert(false); }\n",
				    si, to, e->u.literal.byte);
				break;
			}
		}
	}
#else
	fprintf(f, "NFA#%p, [", (void *) spec);
	for (size_t i = 0; i < MAX_NFA_OPS; i++) {
		if (spec->ops[i] != NFA_OP_NOP) {
			fprintf(f, "%s ", nfa_op_name(spec->ops[i]));
		}
	}

	fprintf(f, "], %zd states:\n", spec->state_count);

	for (size_t si = 0; si < spec->state_count; si++) {
		struct nfa_state *s;

		s = spec->states[si];
		assert(s->id == si);
		fprintf(f, "    == State %zd, end? %d, shuffle_seed %u, %zd edges:\n",
		    si, s->is_end, s->shuffle_seed, s->edge_count);

		for (size_t ei = 0; ei < s->edge_count; ei++) {
			switch (s->edges[ei].t) {
			case NFA_EDGE_EPSILON:
				fprintf(f, "        === Edge %zd, epsilon => %zd\n",
				    ei, s->edges[ei].to);
				break;

			case NFA_EDGE_ANY:
				fprintf(f, "        === Edge %zd, any => %zd\n",
				    ei, s->edges[ei].to);
				break;

			case NFA_EDGE_LITERAL: {
				uint8_t b = s->edges[ei].u.literal.byte;
				fprintf(f, "        === Edge %zd, literal 0x%02x ('%c') => %zd\n",
				    ei, b, (isprint(b) ? b : '.'), s->edges[ei].to);
				break;
			}

			default: assert(false); return;
			}
		}
	}
#endif
}

const struct theft_type_info type_info_nfa = {
	.alloc = type_info_nfa_alloc,
	.free  = free_nfa_spec,
	.print = print_nfa_spec,
	.autoshrink_config = {
		.enable = true,
		.print_mode = THEFT_AUTOSHRINK_PRINT_USER | 0x80 /* bug */,
	}
};

const char *
nfa_op_name(enum nfa_op op)
{
fprintf(stderr, "nfa_op %u\n", op);
	switch (op) {
	case NFA_OP_NOP:               return "NOP";
	case NFA_OP_MINIMISE:          return "MINIMISE";
	case NFA_OP_DETERMINISE:       return "DETERMINISE";
	case NFA_OP_TRIM:              return "TRIM";
	case NFA_OP_DOUBLE_REVERSE:    return "DOUBLE_REVERSE";
	case NFA_OP_DOUBLE_COMPLEMENT: return "DOUBLE_COMPLEMENT";

	default:
		assert(false);
		return "matchfail";
	}
}

