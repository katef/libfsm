/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_ADT_SET_H
#define TYPE_INFO_ADT_SET_H

#include "theft_libfsm.h"

enum set_op_type {
	/* set_create and set_free are called once, by the test fixture. */
	SET_OP_ADD,
	SET_OP_REMOVE,
	SET_OP_CLEAR,
	SET_OP_TYPE_COUNT
};

struct set_op {
	enum set_op_type t;
	union {
		struct {
			uintptr_t item;
		} add;
		struct {
			uintptr_t item;
		} remove;
	} u;
};

struct set_scenario {
	size_t count;
	struct set_op ops[];
};

struct set_sequence {
	uint16_t shuffle_seed;
	size_t count;
	uint16_t values[];
};


struct set_hook_env {
	char tag;
	struct theft_print_trial_result_env print_env;
	uint8_t verbosity;
	uint8_t count_bits;
	uint8_t item_bits;
};

extern const struct theft_type_info type_info_adt_set_scenario;
extern const struct theft_type_info type_info_adt_set_seq;

uint16_t *theft_info_adt_sequence_permuted(struct set_sequence *seq);

#endif
