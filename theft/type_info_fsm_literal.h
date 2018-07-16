/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_FSM_LITERAL_H
#define TYPE_INFO_FSM_LITERAL_H

#include "theft_libfsm.h"

/* This will allocate up to this many literal strings. */
#define FSM_MAX_LITERALS 64

/* An array of string literals.
 * All strings are unique and zero terminated, but can also contain
 * zeroes within the body of the string. */
struct fsm_literal_scen {
	char tag;
	uint8_t verbosity;
	size_t count;
	struct string_pair pairs[];
};

extern const struct theft_type_info type_info_fsm_literal;

#endif
