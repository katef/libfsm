/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_ADT_PRIQ_H
#define TYPE_INFO_ADT_PRIQ_H

#include "theft_libfsm.h"

enum priq_op_type {
	PRIQ_OP_POP,
	PRIQ_OP_PUSH,
	PRIQ_OP_UPDATE,
	PRIQ_OP_TYPE_COUNT,

	PRIQ_OP_MOVE /* not yet implemented */
};

struct priq_op {
	enum priq_op_type t;
	union {
		struct {
			int unused;
		} pop;
		struct {
			uintptr_t id;
			unsigned int cost;
		} push;
		struct {
			uintptr_t id;
			unsigned int cost;
		} update;
		struct {
			uintptr_t old_id;
			uintptr_t new_id;
		} move;
	} u;
};

struct priq_scenario {
	size_t count;
	struct priq_op ops[];
};

struct priq_hook_env {
	char tag;
	struct theft_print_trial_result_env print_env;
	uint8_t verbosity;
	uint8_t count_bits;
	uint8_t id_bits;
	uint8_t cost_bits;
};

extern struct theft_type_info type_info_adt_priq;

#endif
