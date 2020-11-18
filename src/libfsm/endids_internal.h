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

#define BUCKET_NO_STATE ((fsm_state_t)-1)
#define DEF_BUCKET_COUNT 4
#define DEF_BUCKET_ID_COUNT 16

struct endid_info {
	/* Add-only hash table, with a state ID and an associated
	 * non-empty ordered array of unique end IDs. The state is the
	 * key. Grows when the buckets are more than half full. */
	unsigned bucket_count;
	unsigned buckets_used;

	struct endid_info_bucket {
		fsm_state_t state;
		struct end_info_ids {
			unsigned count;
			unsigned ceil;
			fsm_end_id_t ids[1];
		} *ids;
	} *buckets;
};

#endif
