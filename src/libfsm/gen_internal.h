#ifndef GEN_INTERNAL_H
#define GEN_INTERNAL_H

#include <assert.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/edgeset.h>
#include <adt/stateset.h>

#include <ctype.h>

#include "internal.h"

#define DEF_STACK_FRAMES 4
#define DEF_BUF 4

#define DEF_SEEN_CEIL2 10

#define NO_COUNT ((unsigned)-1)
#define NO_EDGE_DISTANCE ((unsigned)-1)
#define NO_SELF_EDGE ((unsigned)-1)

#define LOG_GEN 0
#define LOG_FILE stderr

#if LOG_GEN > 0
#include <stdio.h>
#define LOG(LVL, ...)				\
	if (LOG_GEN >= LVL) {			\
		fprintf(LOG_FILE, __VA_ARGS__);	\
	}
#else
#define LOG(LVL, ...)  /* unused */
#endif

#define DUMP_EDGES 0

struct gen_stack_frame {
	unsigned self_edge;	/* NO_SELF_EDGE or edge symbol */
	fsm_state_t s_id;
	enum gen_stack_frame_state {
		GEN_SFS_ENTERING_STATE,
		GEN_SFS_MATCH_CALLBACK,
		GEN_SFS_STEP_EDGES,
		GEN_SFS_STEP_SELF_REF,
		GEN_SFS_LEAVING_STATE
	} t;
	union {
		struct {
			bool initialized;
			union {
				struct edge_group_iter ei;
			} u;
		} step_edges;
	} u;
};

struct gen_ctx {
	const struct fsm_alloc *alloc;
	const struct fsm *fsm;
	size_t max_length;
	fsm_generate_matches_cb *cb;
	void *opaque;

	size_t buf_ceil;
	size_t buf_used;
	char *buf;

	/* This is used to avoid useless cycles -- if a state
	 * was reached since the same match_count, then don't
	 * explore it again. */
	unsigned match_count;
	unsigned *state_counts;

	unsigned depth;
	unsigned steps;
	bool done;

	/* Shortest end distance for a state: sed[s_id] */
	unsigned *sed;

	struct gen_stack {
		size_t i;
		size_t ceil;
		struct gen_stack_frame *frames;
	} stack;
};

static bool
gen_init_outer(struct fsm *fsm, size_t max_length,
    fsm_generate_matches_cb *cb, void *opaque,
    bool randomized, unsigned seed);

static bool
gen_init(struct gen_ctx *ctx, struct fsm *fsm);

static bool
gen_iter(struct gen_ctx *ctx);

static bool
enter_state(struct gen_ctx *ctx, fsm_state_t s_id);

static bool
sfs_entering_state(struct gen_ctx *ctx, struct gen_stack_frame *sf);

static bool
sfs_call_match_callback(struct gen_ctx *ctx, struct gen_stack_frame *sf);

static bool
sfs_step_edges(struct gen_ctx *ctx, struct gen_stack_frame *sf);

static bool
sfs_step_self_ref(struct gen_ctx *ctx, struct gen_stack_frame *sf);

static bool
sfs_leaving_state(struct gen_ctx *ctx, struct gen_stack_frame *sf);

static bool
grow_buf(struct gen_ctx *ctx);

static bool
grow_stack(struct gen_ctx *ctx);

#endif
