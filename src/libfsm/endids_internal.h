#ifndef ENDIDS_INTERNAL_H
#define ENDIDS_INTERNAL_H

#include <stdlib.h>
#include <stdint.h>

#include <fsm/alloc.h>
#include <fsm/capture.h>
#include <fsm/fsm.h>

#include <adt/edgeset.h>

#include <string.h>
#include <assert.h>
#include <errno.h>

#include "internal.h"
#include "endids.h"

struct fsm_endid_info {
	int todo;
};

#endif
