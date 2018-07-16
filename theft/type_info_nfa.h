/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_NFA_H
#define TYPE_INFO_NFA_H

#include "theft_libfsm.h"

extern const struct fsm_options test_nfa_fsm_options;

enum nfa_edge_type {
	NFA_EDGE_ANY,
	NFA_EDGE_EPSILON,
	NFA_EDGE_LITERAL
};

struct nfa_edge {
	enum nfa_edge_type t;
	union {
		struct {
			uint8_t byte;
		} literal;
	} u;
	size_t to;
};

struct nfa_state {
	size_t id;
	bool is_end;
	size_t edge_count;
	uint32_t shuffle_seed;
	struct nfa_edge edges[];
};

enum nfa_op {
	NFA_OP_NOP,
	NFA_OP_DETERMINISE,
	NFA_OP_TRIM,

	NFA_OP_TYPE_COUNT,

	/* skip all below here for now */
	NFA_OP_DOUBLE_REVERSE,
	NFA_OP_MINIMISE,
	NFA_OP_DOUBLE_COMPLEMENT,
};

#define MAX_NFA_OPS 8
#define NFA_OPS_BITS 3

const char *nfa_op_name(enum nfa_op op);

/* Abstract specifier for an nfa, used to generate an NFA with libfsm. */
struct nfa_spec {
	char tag;
	enum nfa_op ops[MAX_NFA_OPS];
	size_t state_count;
	struct nfa_state **states;
};

extern const struct theft_type_info type_info_nfa;

/* Expose the NFA-of-PRNG-stream callback, because it's used
 * as part of other tests too. */
enum theft_alloc_res
type_info_nfa_alloc(struct theft *t, void *type_env, void **output);

#endif
