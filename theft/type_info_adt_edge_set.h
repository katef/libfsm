/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_ADT_EDGE_SET_H
#define TYPE_INFO_ADT_EDGE_SET_H

#include "theft_libfsm.h"

#define MAX_STATE_SET 13
#define MAX_KEPT 8
#define DEF_MODEL_CEIL 8

#define DEF_CEIL_BITS 10
#define DEF_SYMBOL_BITS 6
#define DEF_STATE_BITS 6
struct edge_set_ops_env {
	char tag;
	uint8_t ceil_bits;
	uint8_t symbol_bits;
	uint8_t state_bits;
};

struct edge_set_ops {
	size_t count;
	struct edge_set_op {
		enum edge_set_op_type {
			ESO_ADD,
			ESO_ADD_STATE_SET,
			ESO_REMOVE,
			ESO_REMOVE_STATE,
			ESO_REPLACE_STATE,
			ESO_COMPACT,
			ESO_COPY,
			ESO_TYPE_COUNT,
		} t;
		union {
			struct eso_add {
				unsigned char symbol;
				fsm_state_t state;
			} add;
			struct eso_add_state_set {
				unsigned char symbol;
				unsigned char state_count;
				fsm_state_t states[MAX_STATE_SET];
			} add_state_set;
			struct eso_remove {
				unsigned char symbol;
			} remove;
			struct eso_remove_state {
				fsm_state_t state;
			} remove_state;
			struct eso_replace_state {
				fsm_state_t old;
				fsm_state_t new;
			} replace_state;
			struct eso_compact {
				uint64_t kept[MAX_KEPT];
			} compact;
		} u;
	} *ops;
};

struct edge_set_model {
	size_t count;
	size_t ceil;
	struct model_edge {
		unsigned char symbol;
		fsm_state_t state;
	} *edges;
};

extern const struct theft_type_info type_info_adt_edge_set_ops;

#endif
