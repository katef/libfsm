/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_ADT_IPRIQ_H
#define TYPE_INFO_ADT_IPRIQ_H

#include "theft_libfsm.h"

enum ipriq_op_type {
	IPRIQ_OP_ADD,
	IPRIQ_OP_PEEK,
	IPRIQ_OP_POP,
	IPRIQ_OP_TYPE_COUNT,
};

struct ipriq_op {
	enum ipriq_op_type t;
	union {
		struct {
			size_t x;
		} add;
	} u;
};

struct ipriq_scenario {
	size_t count;
	struct ipriq_op ops[];
};

struct ipriq_hook_env {
	char tag;
	size_t limit;
};

extern struct theft_type_info type_info_adt_ipriq;

#endif
