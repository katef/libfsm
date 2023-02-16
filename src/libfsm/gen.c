/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/edgeset.h>
#include <adt/stateset.h>

#include <assert.h>

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

int
fsm_generate_matches(struct fsm *fsm, size_t max_length,
    fsm_generate_matches_cb *cb, void *opaque)
{
	INIT_TIMERS();
	TIME(&pre);
	int res = gen_init_outer(fsm, max_length, cb, opaque, false, 0);
	TIME(&post);

	DIFF_MSEC("fsm_generate_matches", pre, post, NULL);
	return res;
}

static bool
gen_init_outer(struct fsm *fsm, size_t max_length,
    fsm_generate_matches_cb *cb, void *opaque,
    bool randomized, unsigned seed)
{
	int res = 0;
	if (fsm == NULL || cb == NULL || max_length == 0) {
		return false;
	}

	assert(fsm_all(fsm, fsm_isdfa)); /* DFA-only */

	assert(!randomized);		 /* not yet supported */
	(void)seed;

#if LOG_GEN > 1
	fprintf(stderr, "%s: %u states\n", __func__, fsm_countstates(fsm));
#endif
	if (fsm_countstates(fsm) < 1) {
		return false;
	}

	struct gen_ctx ctx = {
		.alloc = fsm->opt->alloc,
		.fsm = fsm,
		.max_length = max_length,
		.cb = cb,
		.opaque = opaque,
	};

	if (!gen_init(&ctx, fsm)) {
		goto cleanup;
	}

	res = 1;

	while (!ctx.done) {
		if (!gen_iter(&ctx)) {
			res = 0;
			break;
		}
	}

cleanup:
	if (ctx.buf != NULL) {
		f_free(ctx.alloc, ctx.buf);
	}

	if (ctx.state_counts != NULL) {
		f_free(ctx.alloc, ctx.state_counts);
	}

	if (ctx.stack.frames != NULL) {
		f_free(ctx.alloc, ctx.stack.frames);
	}

	if (ctx.sed != NULL) {
		f_free(ctx.alloc, ctx.sed);
	}

	return res;
}

#if LOG_GEN > 0
static const char *
sfs_name(enum gen_stack_frame_state s)
{
	switch (s) {
	case GEN_SFS_ENTERING_STATE:
		return "ENTERING_STATE";
	case GEN_SFS_MATCH_CALLBACK:
		return "MATCH_CALLBACK";
	case GEN_SFS_STEP_EDGES:
		return "STEP_EDGES";
	case GEN_SFS_STEP_SELF_REF:
		return "STEP_SELF_REF";
	case GEN_SFS_LEAVING_STATE:
		return "LEAVING_STATE";
	default:
		assert(!"MATCH FAIL");
		return NULL;
	}
}
#endif

static bool
gen_init(struct gen_ctx *ctx, struct fsm *fsm)
{
	fsm_state_t start;
	if (!fsm_getstart(ctx->fsm, &start)) {
		return false;
	}

	if (!grow_buf(ctx)) {
		return false;
	}

	if (!grow_stack(ctx)) {
		return false;
	}

	const size_t count = fsm_countstates(ctx->fsm);

	ctx->state_counts = f_malloc(ctx->alloc,
	    count * sizeof(ctx->state_counts[0]));
	if (ctx->state_counts == NULL) {
		return false;
	}

	/* Trim the FSM. This eliminates paths will never produce
	 * matching input, and also populates an array of the shortest
	 * distance to an end for each state -- which lets us eliminate
	 * pointless searching when the remaining space in the buffer
	 * wouldn't be enough to reach any end state, which has a
	 * dramatic impact on performance. */
	long trim_res = fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE, &ctx->sed);
	if (trim_res < 0) {
		return false;
	}

	for (size_t i = 0; i < count; i++) {
		ctx->state_counts[i] = NO_COUNT;
	}

	if (!enter_state(ctx, start)) {
		return false;
	}

	return true;
}

static bool
gen_iter(struct gen_ctx *ctx)
{
	if (ctx->done) {
		return true;
	}

	ctx->steps++;

	assert(ctx->stack.i < ctx->stack.ceil);
	struct gen_stack_frame *sf = &ctx->stack.frames[ctx->stack.i - 1];

	LOG(2, "gen_iter: state %d, stk_i %lu, sf->t %s, depth %u, buf \"%s\", match_count %u\n",
	    sf->s_id, ctx->stack.i, sfs_name(sf->t), ctx->depth, ctx->buf, ctx->match_count);

	switch (sf->t) {
	case GEN_SFS_ENTERING_STATE:
		return sfs_entering_state(ctx, sf);
	case GEN_SFS_MATCH_CALLBACK:
		return sfs_call_match_callback(ctx, sf);
	case GEN_SFS_STEP_EDGES:
		return sfs_step_edges(ctx, sf);
	case GEN_SFS_STEP_SELF_REF:
		return sfs_step_self_ref(ctx, sf);
	case GEN_SFS_LEAVING_STATE:
		return sfs_leaving_state(ctx, sf);
	default:
		assert(!"match fail");
		return false;
	}
}

static bool
grow_buf(struct gen_ctx *ctx)
{
	const size_t nceil = (ctx->buf_ceil == 0
	    ? DEF_BUF : 2 * ctx->buf_ceil);
	assert(nceil > 0);

	char *nbuf = f_realloc(ctx->alloc,
	    ctx->buf, nceil * sizeof(nbuf[0]));

	LOG(2, "grow_buf: %p (%zu) -> %p (%zu)\n",
	    (void *)ctx->buf, ctx->buf_ceil,
	    (void *)nbuf, nceil);

	if (nbuf == NULL) {
		return false;
	}
	nbuf[ctx->buf_used] = '\0';

	ctx->buf = nbuf;
	ctx->buf_ceil = nceil;
	return true;
}

static bool
grow_stack(struct gen_ctx *ctx)
{
	const size_t nceil = (ctx->stack.ceil == 0
	    ? DEF_STACK_FRAMES : 2 * ctx->stack.ceil);
	assert(nceil > 0);

	struct gen_stack_frame *nframes = f_realloc(ctx->alloc,
	    ctx->stack.frames, nceil * sizeof(nframes[0]));

	LOG(2, "grow_stack: %p (%zu) -> %p (%zu)\n",
	    (void *)ctx->stack.frames, ctx->stack.ceil,
	    (void *)nframes, nceil);

	if (nframes == NULL) {
		return false;
	}

	ctx->stack.frames = nframes;
	ctx->stack.ceil = nceil;
	return true;
}

static bool
enter_state(struct gen_ctx *ctx, fsm_state_t s_id)
{
	assert(s_id < fsm_countstates(ctx->fsm));

	if (ctx->stack.i + 1 >= ctx->stack.ceil) {
		if (!grow_stack(ctx)) {
			return false;
		}
	}

	struct gen_stack_frame *sf = &ctx->stack.frames[ctx->stack.i];
	sf->s_id = s_id;
	sf->self_edge = NO_SELF_EDGE;
	sf->t = GEN_SFS_ENTERING_STATE;
	ctx->stack.i++;

	if (ctx->buf_used >= ctx->buf_ceil) {
		if (!grow_buf(ctx)) {
			return false;
		}
	}
	ctx->buf[ctx->buf_used] = '\0';

	ctx->depth++;
	return true;
}

static bool
sfs_entering_state(struct gen_ctx *ctx, struct gen_stack_frame *sf)
{
	if (fsm_isend(ctx->fsm, sf->s_id)) {
		sf->t = GEN_SFS_MATCH_CALLBACK;
	} else {
		sf->t = GEN_SFS_STEP_EDGES;
		sf->u.step_edges.initialized = false;
	}

	return true;
}

static bool
sfs_call_match_callback(struct gen_ctx *ctx, struct gen_stack_frame *sf)
{
	if (ctx->buf_used >= ctx->buf_ceil) {
		if (!grow_buf(ctx)) {
			return false;
		}
	}

	assert(ctx->buf_used < ctx->buf_ceil);
	ctx->buf[ctx->buf_used] = '\0';

	enum fsm_generate_matches_cb_res cb_res;
	cb_res = ctx->cb(ctx->fsm,
	    ctx->depth, ctx->match_count, ctx->steps,
	    ctx->buf, ctx->buf_used,
	    sf->s_id, ctx->opaque);
	ctx->match_count++;

	switch (cb_res) {
	default:
		assert(!"match fail");
	case FSM_GENERATE_MATCHES_CB_RES_HALT:
		ctx->done = true;
		return true;
	case FSM_GENERATE_MATCHES_CB_RES_PRUNE:
		sf->t = GEN_SFS_LEAVING_STATE;
		return true;
	case FSM_GENERATE_MATCHES_CB_RES_CONTINUE:
		sf->t = GEN_SFS_STEP_EDGES;
		sf->u.step_edges.initialized = false;
		return true;
	}
}

static unsigned char
first_symbol(const uint64_t *symbols)
{
	bool has_zero = false;
	unsigned i = 0;
	bool has_any = false;
	unsigned char any_char;

	while (i < 256) {
		const uint64_t w = symbols[i/64];
		if ((i & 63) == 0 && w == 0) {
			i += 64;
			continue;
		}
		if (w & (1ULL << (i & 63))) {
			if (i == 0) {
				has_zero = true;
			} else if (isprint(i) && i != ' ') {
				/* Prefer the first printable character */
				return (unsigned char)i;
			} else if (!has_any) {
				has_any = true;
				any_char = (unsigned char)i;
			}
		}
		i++;
	}

	/* Prefer anything besides 0x00 if present, since that will truncate the string. */
	if (has_zero) {
		return 0;
	}

	/* Otherwise, return the first non-printable character. */
	if (has_any) {
		return any_char;
	}

	assert(!"empty set");
	return 0;
}

#if DUMP_EDGES
static void
dump_edges(fsm_state_t state, struct edge_set *edges)
{
	struct edge_group_iter ei;
	struct edge_group_iter_info eg;
	size_t count = edge_set_count_transitions(edges);

	edge_set_group_iter_reset(edges,
	    EDGE_GROUP_ITER_ALL, &ei);

	size_t i = 0;
	while (edge_set_group_iter_next(&ei, &eg)) {
		const unsigned char symbol = first_symbol(eg.symbols);
		fprintf(stderr, "%s: %d -- %zu/%zu -- 0x%02x (%c) -> %d\n",
		    __func__, state, i, count,
		    symbol, isprint(symbol) ? symbol : '.', eg.to);
	}
}
#endif

static bool
iter_next_transition(const struct gen_ctx *ctx,
    struct gen_stack_frame *sf, struct edge_group_iter_info *eg)
{
	(void)ctx;
	return edge_set_group_iter_next(&sf->u.step_edges.u.ei, eg);
}

static bool
sfs_step_edges(struct gen_ctx *ctx, struct gen_stack_frame *sf)
{
	if (!sf->u.step_edges.initialized) {
		assert(sf->s_id < fsm_countstates(ctx->fsm));
		const struct fsm_state *s = &ctx->fsm->states[sf->s_id];

#if DUMP_EDGES
		dump_edges(sf->s_id, s->edges);
#endif

		edge_set_group_iter_reset(s->edges,
		    EDGE_GROUP_ITER_UNIQUE,
		    &sf->u.step_edges.u.ei);
		sf->u.step_edges.initialized = true;
	}

	if (ctx->buf_used + ctx->sed[sf->s_id] >= ctx->max_length) {
		LOG(2, "PRUNING due to max length: used:%zu + sed[%d]:%u >= max_length:%zu\n",
		    ctx->buf_used, sf->s_id, ctx->sed[sf->s_id], ctx->max_length);
		sf->t = GEN_SFS_LEAVING_STATE;
		return true;
	}

	if (ctx->max_length == ctx->buf_used) {
		sf->t = GEN_SFS_LEAVING_STATE;
		return true;
	}

	struct edge_group_iter_info eg;

	if (iter_next_transition(ctx, sf, &eg)) {
		const unsigned char symbol = first_symbol(eg.symbols);
		const fsm_state_t state = eg.to;

		LOG(2, "sfs_step_edges: got edge 0x%x ('%c')\n",
		    symbol, isprint(symbol) ? symbol : '.');

		if (state == sf->s_id) { /* self edge */
			if (sf->self_edge == NO_SELF_EDGE) {
				LOG(2, "sfs_step_edges: saving self edge with '%c'\n",
				    isprint(symbol) ? symbol : '.');
				sf->self_edge = symbol;
			} else {
				LOG(2, "sfs_step_edges: skipping extra self edge\n");
			}
			return true;
		}

		if (ctx->buf_used + 1 >= ctx->buf_ceil) {
			if (!grow_buf(ctx)) {
				return false;
			}
		}

		ctx->buf[ctx->buf_used] = (char)symbol;
		ctx->buf_used++;
		ctx->buf[ctx->buf_used] = '\0';
		LOG(2, "sfs_step_edges: entering state %u, buffer \"%s\"(%zu)\n",
		    state, ctx->buf, ctx->buf_used);

 		ctx->state_counts[state] = ctx->match_count;

		if (!enter_state(ctx, state)) {
			return false;
		}

		return true;
	} else {		/* done with edge set */
		LOG(3, "sfs_step_edges: done with edge set\n");
		sf->t = GEN_SFS_STEP_SELF_REF;
		return true;
	}
}

static bool
sfs_step_self_ref(struct gen_ctx *ctx, struct gen_stack_frame *sf)
{
	LOG(2, "sfs_step_self_ref: edge == 0x%x\n", sf->self_edge);

	if (sf->self_edge == NO_SELF_EDGE) {
		sf->t = GEN_SFS_LEAVING_STATE;
		return true;
	}

	ctx->buf[ctx->buf_used] = (char)sf->self_edge;
	sf->self_edge = NO_SELF_EDGE;

	ctx->buf_used++;

	if (!enter_state(ctx, sf->s_id)) {
		return false;
	}

	return true;
}

static bool
sfs_leaving_state(struct gen_ctx *ctx, struct gen_stack_frame *sf)
{
	(void)sf;
	LOG(2, "sfs_leaving_state: state %d, stack.i == %zu\n",
		    sf->s_id, ctx->stack.i);

	if (ctx->stack.i == 1) { /* leaving start state */
		ctx->done = true;
		ctx->stack.i--;
		return true;
	}

	assert(ctx->stack.i > 1);
	ctx->stack.i--;

	assert(ctx->depth > 0);
	ctx->depth--;

	const struct gen_stack_frame *prev = &ctx->stack.frames[ctx->stack.i - 1];

	LOG(2, "sfs_leaving_state: prev->t == %s\n", sfs_name(prev->t));

	switch (prev->t) {
	case GEN_SFS_STEP_EDGES:
	case GEN_SFS_STEP_SELF_REF:
		/* remove last character */
		assert(ctx->buf_used > 0);
		ctx->buf_used--;
		ctx->buf[ctx->buf_used] = '\0';
		break;

	default:
		assert(!"unreachable");
		return false;
	}

	return true;
}
