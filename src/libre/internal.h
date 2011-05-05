/* $Id$ */

#ifndef RE_INTERNAL_H
#define RE_INTERNAL_H

#include <re/re.h>
#include <fsm/fsm.h>

/*
 * TODO:
 * store an fsm. probably also the origional string, concepts of
 * numbering subexpressions for matches, and what language it was
 * parsed from?
 * (per expression merged?)
 * if it's minimized or not?
 */
struct re {
	struct fsm *fsm;

	struct fsm_state *end;
};

#endif

