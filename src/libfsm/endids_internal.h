#ifndef ENDIDS_INTERNAL_H
#define ENDIDS_INTERNAL_H

#include <stdlib.h>
#include <stdint.h>

#include <fsm/alloc.h>
#include <fsm/capture.h>
#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/stateset.h>

#include <string.h>
#include <assert.h>
#include <errno.h>

#include "internal.h"
#include "endids.h"

/* temporary */
#define MAX_END_STATES 4
#define MAX_END_IDS 4

struct fsm_endid_info {
	const struct fsm_alloc *alloc;
	size_t count;
	struct endid_info {
		size_t count;
		fsm_state_t state;
		fsm_end_id_t ids[MAX_END_IDS];
	} states[MAX_END_STATES];
};

#endif
