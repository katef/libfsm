#ifndef CAPTURE_INTERNAL_H
#define CAPTURE_INTERNAL_H

#include <stdlib.h>
#include <stdint.h>

#include <fsm/alloc.h>
#include <fsm/capture.h>
#include <fsm/fsm.h>

#include <adt/edgeset.h>
#include <adt/hash.h>

#include <string.h>
#include <assert.h>
#include <errno.h>

#include "internal.h"
#include "capture.h"

/* Bucket count for capture action hash table.
 * Must be a power of 2. */

#define DEF_CAPTURE_ACTION_BUCKET_COUNT 32
#define DEF_TRAIL_CEIL 8

#define LOG_CAPTURE 0

/* Most significant bit of a size_t. */
#define COMMITTED_CAPTURE_FLAG ((SIZE_MAX) ^ (SIZE_MAX >> 1))

struct fsm_capture_info {
	unsigned max_capture_id;

	/* Add-only hash table. */
	unsigned bucket_count;
	unsigned buckets_used; /* grow if >= 1/2 used */

	/* Hash buckets. If state is CAPTURE_NO_STATE,
	 * the bucket is empty. */
	struct fsm_capture_action_bucket {
		fsm_state_t state; /* key */
		struct fsm_capture_action {
			enum capture_action_type type;
			unsigned id;
			/* only used by START and EXTEND */
			fsm_state_t to;
		} action;
	} *buckets;
};

enum trail_step {
	TRAIL_STEP_START,
	TRAIL_STEP_ITER_EDGES,
	TRAIL_STEP_ITER_EPSILONS,
	TRAIL_STEP_DONE
};

/* env->seen is used as a bit set for tracking which states have already
 * been processed. These macros set/check/clear the bits. */
#define SEEN_BITOP(ENV, STATE, OP) ENV->seen[STATE/64] OP ((uint64_t)1 << (STATE&63))
#define MARK_SEEN(ENV, STATE) SEEN_BITOP(ENV, STATE, |=)
#define CHECK_SEEN(ENV, STATE) SEEN_BITOP(ENV, STATE, &)
#define CLEAR_SEEN(ENV, STATE) SEEN_BITOP(ENV, STATE, &=~)

struct capture_set_path_env {
	struct fsm *fsm;
	unsigned capture_id;
	fsm_state_t start;
	fsm_state_t end;

	unsigned trail_i;
	unsigned trail_ceil;
	struct trail_cell {
		fsm_state_t state;
		enum trail_step step;
		char has_self_edge;
		struct edge_iter iter;
	} *trail;

	/* bitset for which states have already been seen. */
	uint64_t *seen;
};

static int
init_capture_action_htab(struct fsm *fsm, struct fsm_capture_info *ci);

static int
mark_capture_path(struct capture_set_path_env *env);

static int
add_capture_action(struct fsm *fsm, struct fsm_capture_info *ci,
    fsm_state_t state, const struct fsm_capture_action *action);

static int
grow_capture_action_buckets(const struct fsm_alloc *alloc,
    struct fsm_capture_info *ci);

static int
grow_trail(struct capture_set_path_env *env);

static int
step_trail_start(struct capture_set_path_env *env);
static int
step_trail_iter_edges(struct capture_set_path_env *env);
static int
step_trail_iter_epsilons(struct capture_set_path_env *env);
static int
step_trail_done(struct capture_set_path_env *env);

static int
cmp_action(const struct fsm_capture_action *a,
    const struct fsm_capture_action *b);

#endif
