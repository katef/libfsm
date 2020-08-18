#ifndef TYPE_INFO_DFA_H
#define TYPE_INFO_DFA_H

#include "theft_libfsm.h"

#define MAX_EDGES 16

#define DEF_SYMBOL_CEIL_BITS 6
#define DEF_STATE_CEIL_BITS 6
#define DEF_END_BITS 3 		/* 1/8 chance of beind an end */

struct dfa_spec_env {
	char tag;		/* 'D' */
	uint8_t state_ceil_bits;
	uint8_t symbol_ceil_bits;
	uint8_t end_bits;	/* 1:n chance of being an end */
};

/* Spec for a random DFA. If an edge refers to a state
 * >= state_count or a state where !dfa->states[id].used,
 * then the edge is ignored. Limiting the variety of symbols
 * and/or states chan produce a variety of sparser or denser
 * DFAs. */
struct dfa_spec {
	size_t state_count;
	size_t start;
	struct dfa_spec_state {
		/* This flag is a foothold for shrinking. */
		bool used;
		bool end;
		uint8_t edge_count;
		struct dfa_spec_edge {
			unsigned char symbol;
			fsm_state_t state;
		} edges[MAX_EDGES];
	} *states;
};

extern const struct theft_type_info type_info_dfa;

struct fsm *
dfa_spec_build_fsm(const struct dfa_spec *spec);

/* Check which states are live, i.e., reachable from
 * the start state and have a path to at least one end
 * state. This corresponds to trimming -- fsm_trim
 * removes states that are not live.
 *
 * This uses a simple approach and is intended for
 * testing, rather than performance.
 *
 * Returns true on success, or false on alloc failure.
 * If live is non-NULL, set bit flags for reachable states.
 * If live_count is non-NULL, write into it. */
bool
dfa_spec_check_liveness(const struct dfa_spec *spec,
    uint64_t *live, size_t *live_count);

#endif
