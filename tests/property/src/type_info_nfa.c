#include "type_info_nfa.h"

#include <ctype.h>

/* Expose the NFA-of-PRNG-stream callback, because it's used
 * as part of other tests too. */
enum theft_alloc_res
type_info_nfa_alloc(struct theft *t, void *type_env, void **output)
{
	(void)type_env;
	/* TODO: these could be configurable */
	const uint8_t state_bits = 8;
	const uint8_t edge_bits = 4;
	const uint8_t end_bits = 4;

	/* This can get truncated later */
	size_t state_count = theft_random_bits(t, state_bits);
	size_t state_ceil2 = 1;
	while (state_ceil2 < state_count) { state_ceil2 <<= 1; }

	struct nfa_spec *res = calloc(1, sizeof(*res));
	if (res == NULL) { return THEFT_ALLOC_ERROR; }

	struct nfa_state **states = calloc(state_count, sizeof(*states));
	if (states == NULL) { return THEFT_ALLOC_ERROR; }

	uint8_t op_count = theft_random_bits(t, NFA_OPS_BITS);
	for (size_t i = 0; i < op_count; i++) {
		res->ops[i] = theft_random_choice(t, NFA_OP_TYPE_COUNT);
	}

	/* forward-prune so operations are only applied once */
	#define UNIQUE_OPS
	#ifdef UNIQUE_OPS
	for (size_t i = 0; i < op_count; i++) {
		if (res->ops[i] == NFA_OP_NOP) { continue; }
		for (size_t j = i + 1; j < op_count; j++) {
			if (res->ops[i] == res->ops[j]) {
				res->ops[j] = NFA_OP_NOP;
			}
		}
	}
	#endif

	for (size_t i = 0; i < state_count; i++) {
		/* Introduce a second way to reduce the state count;
		 * this should make shrinking a lot faster. */
		if (0 == theft_random_bits(t, 7)) {
			state_count = i;
			break;
		}

		size_t edge_count = theft_random_bits(t, edge_bits);
		const size_t alloc_size = sizeof(struct nfa_state) +
		    edge_count * sizeof(struct nfa_edge);
		struct nfa_state *state = malloc(alloc_size);
		if (state == NULL) { return THEFT_ALLOC_ERROR; }
		memset(state, 0x00, alloc_size);

		state->id = i;
		/* 1 in (1<<N) odds of being an end state; using the highest
		 * value means it will get simplied away quickly */
		state->is_end = ((1U << end_bits) - 1 == theft_random_bits(t, end_bits));
		state->shuffle_seed = theft_random_bits(t, 29);

		for (size_t ei = 0; ei < edge_count; ei++) {
			if (0 == theft_random_bits(t, 7)) {
				edge_count = ei;
				break;
			}
			assert(ei < edge_count);
			struct nfa_edge *e = &state->edges[ei];
			e->t = theft_random_bits(t, 2);
			/* Make literals more common, but quickly simplified away */
			if (e->t > NFA_EDGE_LITERAL) { e->t = NFA_EDGE_LITERAL; }
			assert(e->t <= NFA_EDGE_LITERAL);

			if (e->t == NFA_EDGE_LITERAL) {
				e->u.literal.byte = theft_random_bits(t, 8);
			}
			e->to = theft_random_choice(t, state_count);
		}
		state->edge_count = edge_count;
		
		states[i] = state;
	}

	res->tag = 'N';
	res->states = states;
	res->state_count = state_count;
	*output = res;
	return THEFT_ALLOC_OK;
}

static void
free_nfa_spec(void *instance, void *type_env)
{
	struct nfa_spec *spec = (struct nfa_spec *)instance;
	(void)type_env;

	for (size_t i = 0; i < spec->state_count; i++) {
		free(spec->states[i]);
	}
	free(spec);
}

static void
print_nfa_spec(FILE *f, const void *instance, void *type_env)
{
	struct nfa_spec *spec = (struct nfa_spec *)instance;
	(void)type_env;
	fprintf(f, "NFA#%p, [", (void *)spec);
	for (size_t i = 0; i < MAX_NFA_OPS; i++) {
		if (spec->ops[i] != NFA_OP_NOP) {
			fprintf(f, "%s ", nfa_op_name(spec->ops[i]));
		}
	}

	fprintf(f, "], %zd states:\n", spec->state_count);
	    
	for (size_t si = 0; si < spec->state_count; si++) {
		struct nfa_state *s = spec->states[si];
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
			case NFA_EDGE_LITERAL:
			{
				uint8_t b = s->edges[ei].u.literal.byte;
				fprintf(f, "        === Edge %zd, literal 0x%02x ('%c') => %zd\n",
				    ei, b, (isprint(b) ? b : '.'), s->edges[ei].to);
				break;
			}
			default: assert(false); return;
			}
		}
	}
}

const struct theft_type_info type_info_nfa = {
	.alloc = type_info_nfa_alloc,
	.free = free_nfa_spec,
	.print = print_nfa_spec,
	.autoshrink_config = {
		.enable = true,
		.print_mode = THEFT_AUTOSHRINK_PRINT_USER | 0x80 /* bug */,
	},
};

const char *nfa_op_name(enum nfa_op op)
{
	switch (op) {
	case NFA_OP_NOP: return "NOP";
	case NFA_OP_MINIMISE: return "MINIMISE";
	case NFA_OP_DETERMINISE: return "DETERMINISE";
	case NFA_OP_TRIM: return "TRIM";
	case NFA_OP_DOUBLE_REVERSE: return "DOUBLE_REVERSE";
	case NFA_OP_DOUBLE_COMPLEMENT: return "DOUBLE_COMPLEMENT";
	default:
		assert(false); return "matchfail";
	}
}
