/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/capture.h>
#include <fsm/pred.h>
#include <fsm/subgraph.h>

#include <re/re.h>

#include <adt/idmap.h>
#include <adt/u64bitset.h>

#include "class.h"
#include "ast.h"
#include "ast_compile.h"
#include "re_capvm_compile.h"

#include "libfsm/capture.h"
#include "libfsm/internal.h" /* XXX */

#define LOG_LINKAGE 0
#define LOG_TRAMPOLINE 0

#if LOG_LINKAGE
#include "print.h"
#endif

enum link_side {
	LINK_START,
	LINK_END
};

/*
 * How should this state be linked for the relevant state?
 *
 * - LINK_TOP_DOWN
 *   Use the passed in start/end states (x and y)
 *
 * - LINK_GLOBAL
 *   Link to the global start/end state (env->start_inner or env->end_inner),
 *   because this node has a ^ or $ anchor
 *
 * - LINK_GLOBAL_SELF_LOOP
 *   Link to the unanchored self loop adjacent to the start/end
 *   states (env->start_any_inner or env->end_any_inner), because
 *   this node is in a FIRST or LAST position, but unanchored.
 */
enum link_types {
	LINK_TOP_DOWN,
	LINK_GLOBAL,
	LINK_GLOBAL_SELF_LOOP,
};

/* Call stack for AST -> NFA conversion. */
#define DEF_COMP_STACK_CEIL 4
#define NO_MAX_CAPTURE_IDS ((unsigned)-1)

struct comp_env {
	const struct fsm_alloc *alloc;
	struct fsm *fsm;
	enum re_flags re_flags;
	struct re_err *err;

	/*
	 * These are saved so that dialects without implicit
	 * anchoring can create states with '.' edges to self
	 * on demand, and link them to the original start and
	 * end states.
	 *
	 * Also, some states in a first/last context need to link
	 * directly to the overall start/end states, either in
	 * place of or along with the adjacent states.
	 *
	 * The inner start and end states are considered inside of
	 * match group 0, outer are not.
	 */
	fsm_state_t start_outer;
	fsm_state_t start_inner;
	fsm_state_t start_any_loop;
	fsm_state_t start_any_inner;
	int have_start_any_loop;

	/* States leading to the end, with and without an unanchored
	 * `.*` loop that consumes any trailing characters. */
	fsm_state_t end_inner;
	fsm_state_t end_outer;
	fsm_state_t end_nl_inner;
	fsm_state_t end_any_loop;
	fsm_state_t end_any_inner;
	int has_end_any_loop;
	int has_end_nl_inner;

	/* bitset for active capture IDs */
	uint64_t *active_capture_ids;
	long max_capture_id;	/* upper bound */

	/* Evaluation stack */
	struct comp_stack {
		size_t ceil;
		size_t depth;		/* 0 -> empty */

		struct comp_stack_frame {
			struct ast_expr *n;
			fsm_state_t x;
			fsm_state_t y;
			unsigned step;

			union {
				struct {
					fsm_state_t link;
				} concat;
				struct {
					unsigned count;
				} alt;
				struct {
					struct fsm_subgraph subgraph;
					fsm_state_t na;
					fsm_state_t nz;
				} repeat;
			} u;
		} *frames;
	} stack;
};

static int
comp_iter(struct comp_env *env, fsm_state_t x, const struct ast *ast);

static int
eval_stack_frame(struct comp_env *env);

static int
eval_EMPTY(struct comp_env *env);
static int
eval_CONCAT(struct comp_env *env);
static int
eval_ALT(struct comp_env *env);
static int
eval_LITERAL(struct comp_env *env);
static int
eval_CODEPOINT(struct comp_env *env);
static int
eval_REPEAT(struct comp_env *env);
static int
eval_GROUP(struct comp_env *env);
static int
eval_ANCHOR(struct comp_env *env);
static int
eval_SUBTRACT(struct comp_env *env);
static int
eval_RANGE(struct comp_env *env);
static int
eval_TOMBSTONE(struct comp_env *env);

static int
compile_capvm_program_for_stack_end_states(struct comp_env *env,
	const struct ast *ast, uint32_t *prog_id);

static int
utf8(uint32_t cp, char c[])
{
	if (cp <= 0x7f) {
		c[0] =  cp;
		return 1;
	}

	if (cp <= 0x7ff) {
		c[0] = (cp >>  6) + 192;
		c[1] = (cp  & 63) + 128;
		return 2;
	}

	if (0xd800 <= cp && cp <= 0xdfff) {
		/* invalid */
		goto error;
	}

	if (cp <= 0xffff) {
		c[0] =  (cp >> 12) + 224;
		c[1] = ((cp >>  6) &  63) + 128;
		c[2] =  (cp  & 63) + 128;
		return 3;
	}

	if (cp <= 0x10ffff) {
		c[0] =  (cp >> 18) + 240;
		c[1] = ((cp >> 12) &  63) + 128;
		c[2] = ((cp >>  6) &  63) + 128;
		c[3] =  (cp  & 63) + 128;
		return 4;
	}

error:

	return 0;
}

/* TODO: centralise as fsm_unionxy() perhaps */
static int
fsm_unionxy(struct fsm *a, struct fsm *b, fsm_state_t x, fsm_state_t y)
{
	fsm_state_t sa, sb;
	fsm_state_t end;
	struct fsm *q;
	fsm_state_t base_b;

	assert(a != NULL);
	assert(b != NULL);

	/* x,y both belong to a */
	assert(x < a->statecount);
	assert(y < a->statecount);

	if (!fsm_getstart(a, &sa)) {
		return 0;
	}

	if (!fsm_getstart(b, &sb)) {
		return 0;
	}

	if (!fsm_collate(b, &end, fsm_isend)) {
		return 0;
	}

	/* TODO: centralise as fsm_clearends() or somesuch */
	{
		size_t i;

		const size_t statecount = fsm_countstates(b);
		for (i = 0; i < statecount; i++) {
			fsm_setend(b, i, 0);
		}
	}

	q = fsm_mergeab(a, b, &base_b);
	if (q == NULL) {
		return 0;
	}

	sb  += base_b;
	end += base_b;

	fsm_setstart(q, sa);

	if (!fsm_addedge_epsilon(q, x, sb)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(q, end, y)) {
		return 0;
	}

	return 1;
}

static struct fsm *
expr_compile(struct ast_expr *e, enum re_flags flags,
	const struct fsm_options *opt, struct re_err *err)
{
	struct ast ast;

	ast.expr = e;
	ast.max_capture_id = 0;
	ast.has_unanchored_start = 0;
	ast.has_unanchored_end = 0;

	return ast_compile(&ast, flags, opt, err);
}

static int
addedge_literal(struct comp_env *env, enum re_flags re_flags,
	fsm_state_t from, fsm_state_t to, char c)
{
	struct fsm *fsm = env->fsm;
	assert(fsm != NULL);

	assert(from < env->fsm->statecount);
	assert(to < env->fsm->statecount);

	if (re_flags & RE_ICASE) {
		if (!fsm_addedge_literal(fsm, from, to, (char)tolower((unsigned char) c))) {
			return 0;
		}

		if (!fsm_addedge_literal(fsm, from, to, (char)toupper((unsigned char) c))) {
			return 0;
		}
	} else {
		if (!fsm_addedge_literal(fsm, from, to, c)) {
			return 0;
		}
	}

	return 1;
}

static int
intern_start_any_loop(struct comp_env *env)
{
	fsm_state_t loop, inner;

	assert(env != NULL);

	if (env->have_start_any_loop) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(env->start_outer < env->fsm->statecount);

	if (!fsm_addstate(env->fsm, &loop)) {
		return 0;
	}

	if (!fsm_addstate(env->fsm, &inner)) {
		return 0;
	}

#if LOG_LINKAGE
	fprintf(stderr, "%s: start_any: loop %d, inner: %d\n", __func__, loop, inner);
#endif

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(env->fsm, env->start_outer, loop)) {
		return 0;
	}
	if (!fsm_addedge_epsilon(env->fsm, loop, inner)) {
		return 0;
	}

	env->start_any_loop = loop;
	env->start_any_inner = inner;
	env->have_start_any_loop = 1;

	return 1;
}

static int
intern_end_any_loop(struct comp_env *env)
{
	fsm_state_t loop, inner;

	assert(env != NULL);

	if (env->has_end_any_loop) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(env->end_outer < env->fsm->statecount);

	if (!fsm_addstate(env->fsm, &loop)) {
		return 0;
	}
	if (!fsm_addstate(env->fsm, &inner)) {
		return 0;
	}

#if LOG_LINKAGE
	fprintf(stderr, "%s: end_any: %d, inner: %d\n", __func__, loop, inner);
#endif

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(env->fsm, inner, loop)) {
		return 0;
	}
	if (!fsm_addedge_epsilon(env->fsm, loop, env->end_outer)) {
		return 0;
	}

	env->end_any_loop = loop;
	env->end_any_inner = inner;
	env->has_end_any_loop = 1;

	return 1;
}

static int
intern_end_nl(struct comp_env *env)
{
	/* PCRE's end anchor $ matches a single optional newline,
	 * which should exist outside of match group 0.
	 *
	 * Intern states for a `\n?` that links to */
	assert(env != NULL);

	if (env->has_end_nl_inner) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(env->re_flags & RE_END_NL);
	assert(~env->re_flags & RE_END_NL_DISABLE);
	assert(env->end_outer < env->fsm->statecount);

	fsm_state_t inner;
	if (!fsm_addstate(env->fsm, &inner)) {
		return 0;
	}

#if LOG_LINKAGE
	fprintf(stderr, "%s: end_nl_inner: %d\n", __func__, inner);
#endif

	if (!fsm_addedge_epsilon(env->fsm, inner, env->end_outer)) {
		return 0;
	}
	if (!fsm_addedge_literal(env->fsm, inner, env->end_outer, (char)'\n')) {
		return 0;
	}

	env->end_nl_inner = inner;
	env->has_end_nl_inner = 1;
	return 1;
}

static int
can_have_backward_epsilon_edge(const struct ast_expr *e)
{
	switch (e->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_ALT:
	case AST_EXPR_ANCHOR:
	case AST_EXPR_RANGE:
		/* These nodes cannot have a backward epsilon edge */
		return 0;

	case AST_EXPR_SUBTRACT:
		/* Single character class */
		return 0;

	case AST_EXPR_REPEAT:
		/* 0 and 1 don't have backward epsilon edges */
		if (e->u.repeat.max <= 1) {
			return 0;
		}

		/*
		 * The general case for counted repetition already
		 * allocates one-way guard states around it
		 */
		if (e->u.repeat.max != AST_COUNT_UNBOUNDED) {
			return 0;
		}

		return 1;

	case AST_EXPR_GROUP:
		return can_have_backward_epsilon_edge(e->u.group.e);

	default:
		break;
	}

	return 1;
}

static int
can_skip_concat_state_and_epsilon(const struct ast_expr *l,
	const struct ast_expr *r)
{
	assert(l != NULL);

	/*
	 * CONCAT only needs the extra state and epsilon edge when there
	 * is a backward epsilon edge for repetition - without it, a
	 * regex such as /a*b*c/ could match "ababc" as well as "aabbc",
	 * because the backward epsilon for repeating the 'b' would lead
	 * to a state which has another backward epsilon for repeating
	 * the 'a'. The extra state functions as a one-way guard,
	 * keeping the match from looping further back in the FSM than
	 * intended.
	 */

	if (!can_have_backward_epsilon_edge(l)) {
		return 1;
	}

	if (r != NULL && r->type == AST_EXPR_REPEAT) {
		if (!can_have_backward_epsilon_edge(r)) {
			return 1;
		}
	}

	return 0;
}

static const struct ast_expr *
get_parent_node_from_stack(const struct comp_stack *stack)
{
	if (stack->depth < 2) { return NULL; }
	return stack->frames[stack->depth - 2].n;
}

static enum link_types
decide_linking(struct comp_env *env, fsm_state_t x, fsm_state_t y,
	struct ast_expr *n, enum link_side side)
{
	assert(n != NULL);
	assert(env != NULL);

	(void)x;
	(void)y;

	struct comp_stack *stack = &env->stack;

	/* If the regex is implicitly anchored and the dialect does
	 * not support anchoring, linking is always top-down. */
	if ((env->re_flags & RE_ANCHORED)) {
		return LINK_TOP_DOWN;
	}

	const struct ast_expr *parent = get_parent_node_from_stack(stack);
	assert(parent != n);

	/* Note: any asymmetry here should be due to special cases
	 * involving `$` matching exactly one '\n'. */
	switch (side) {
	case LINK_START: {
		const int first    = (n->flags & AST_FLAG_FIRST) != 0;
		const int anchored = (n->flags & AST_FLAG_ANCHORED_START) != 0;
		const int anchored_parent = (parent && (parent->flags & AST_FLAG_ANCHORED_START) != 0);

		if (first && anchored) { return LINK_GLOBAL; }
		if (first && !anchored) { return LINK_GLOBAL_SELF_LOOP; }

		if (anchored && !first && !anchored_parent) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;

	}

	case LINK_END: {
		const int last     = (n->flags & AST_FLAG_LAST) != 0;
		const int anchored = (n->flags & AST_FLAG_ANCHORED_END) != 0;

		if (last && anchored) { return LINK_GLOBAL; }
		if (last && !anchored) { return LINK_GLOBAL_SELF_LOOP; }

		if (anchored && !last) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;
	}

	default:
		assert(!"unreached");
	}

	return LINK_GLOBAL;
}

static void
print_linkage(enum link_types t)
{
	switch (t) {
	case LINK_TOP_DOWN:
		fprintf(stderr, "[TOP_DOWN]");
		break;
	case LINK_GLOBAL:
		fprintf(stderr, "[GLOBAL]");
		break;
	case LINK_GLOBAL_SELF_LOOP:
		fprintf(stderr, "[SELF_LOOP]");
		break;
	default:
		assert(!"match fail");
		break;
	}
}

#define NEWSTATE(NAME)              \
    if (!fsm_addstate(env->fsm, &(NAME))) { return 0; }

#define EPSILON(FROM, TO)           \
    assert((FROM) != (TO));         \
    if (!fsm_addedge_epsilon(env->fsm, (FROM), (TO))) { return 0; }

#define ANY(FROM, TO)               \
    if (!fsm_addedge_any(env->fsm, (FROM), (TO))) { return 0; }

#define LITERAL(FROM, TO, C)        \
    if (!addedge_literal(env, n->re_flags, (FROM), (TO), ((char)C))) { return 0; }

#define RETURN(ENV) comp_stack_pop(ENV)

#define RECURSE(ENV, FROM, TO, NODE)					\
	if (!comp_stack_push(ENV, (FROM), (TO), (NODE))) { return 0; }

#define TAILCALL(ENV, FROM, TO, NODE)					\
	comp_stack_tailcall(ENV, (FROM), (TO), (NODE));

static int
set_linking(struct comp_env *env, struct ast_expr *n,
    enum link_types link_start, enum link_types link_end,
    fsm_state_t *px, fsm_state_t *py)
{
	fsm_state_t x = *px;
	fsm_state_t y = *py;

#if LOG_LINKAGE
	fprintf(stderr, "%s: decide_linking %p [%s]: start ",
	    __func__, (void *) n, ast_node_type_name(n->type));
	print_linkage(link_start);
	fprintf(stderr, ", end ");
	print_linkage(link_end);
	fprintf(stderr, ", x %d, y %d\n", x, y);
#else
	(void) print_linkage;
	(void)n;
#endif

	switch (link_start) {
	case LINK_TOP_DOWN:
		break;
	case LINK_GLOBAL:
		x = env->start_inner;
		break;
	case LINK_GLOBAL_SELF_LOOP:
		if (!intern_start_any_loop(env)) {
			return 0;
		}
		assert(env->have_start_any_loop);

		x = env->start_any_inner;
		break;
	default:
		assert(!"match fail"); /* these should be mutually exclusive now */
	}

	switch (link_end) {
	case LINK_TOP_DOWN:
		break;
	case LINK_GLOBAL:
		if (env->re_flags & RE_END_NL && !(env->re_flags & RE_END_NL_DISABLE)
		    && (n->flags & AST_FLAG_END_NL)) {
			if (!intern_end_nl(env)) {
				return 0;
			}
			y = env->end_nl_inner;
		} else {
			y = env->end_inner;
		}
		break;
	case LINK_GLOBAL_SELF_LOOP:
		if (!intern_end_any_loop(env)) {
			return 0;
		}
		assert(env->has_end_any_loop);

		y = env->end_any_inner;
		break;
	default:
		assert(!"match fail"); /* these should be mutually exclusive now */
	}

#if LOG_LINKAGE
	fprintf(stderr, " ---> x: %d, y: %d\n", x, y);
#endif
	*px = x;
	*py = y;
	return 1;
}

static void
comp_stack_pop(struct comp_env *env)
{
	assert(env->stack.depth > 0);
	env->stack.depth--;
}

static int
comp_stack_push(struct comp_env *env, fsm_state_t x, fsm_state_t y, struct ast_expr *n)
{
	struct comp_stack *stack = &env->stack;
	assert(n != NULL);

	if (stack->depth == stack->ceil) {
		const size_t nceil = 2*stack->ceil;
		struct comp_stack_frame *nframes = f_realloc(env->alloc,
		    stack->frames, nceil * sizeof(stack->frames[0]));
#if LOG_LINKAGE || LOG_TRAMPOLINE
		fprintf(stderr, "comp_stack_push: reallocating comp_stack, %zu -> %zu frames\n",
		    stack->ceil, nceil);
#endif
		if (nframes == NULL) {
			return 0;
		}
		stack->ceil = nceil;
		stack->frames = nframes;
	}

	assert(stack->depth < stack->ceil);

	struct comp_stack_frame *sf = &stack->frames[stack->depth];
	memset(sf, 0x00, sizeof(*sf));
	sf->n = n;
	sf->x = x;
	sf->y = y;

	stack->depth++;
	return 1;
}

static void
comp_stack_tailcall(struct comp_env *env,
	fsm_state_t x, fsm_state_t y, struct ast_expr *n)
{
	struct comp_stack *stack = &env->stack;

	assert(stack->depth > 0);

	/* Replace current stack frame. */
	struct comp_stack_frame *sf = &stack->frames[stack->depth - 1];
	memset(sf, 0x00, sizeof(*sf));
	sf->n = n;
	sf->x = x;
	sf->y = y;
}

#define JUST_ONE_PROG 1

static int
comp_iter(struct comp_env *env,
	fsm_state_t x, const struct ast *ast)
{
	int res = 1;
	assert(ast != NULL);
	assert(ast->expr != NULL);

	struct comp_stack_frame *frames = NULL;
	uint64_t *active_capture_ids = NULL;
	const bool use_captures = (env->re_flags & RE_NOCAPTURE) == 0;

	frames = f_calloc(env->alloc,
	    DEF_COMP_STACK_CEIL, sizeof(env->stack.frames[0]));
	if (frames == NULL) {
		goto alloc_fail;
	}

	{
		const size_t capture_id_words = (env->max_capture_id == AST_NO_MAX_CAPTURE_ID)
		    ? 0
		    : ((env->max_capture_id)/64 + 1);
		active_capture_ids = f_calloc(env->alloc, capture_id_words,
		    sizeof(active_capture_ids[0]));
		if (active_capture_ids == NULL) {
			goto alloc_fail;
		}
	}

	/* Add inner and outer end states. Like start_outer and start_inner,
	 * these represent the boundary between match group 0 (inner) and
	 * states outside it (the unanchored end loop). */
	if (!fsm_addstate(env->fsm, &env->end_inner)) {
		goto alloc_fail;
	}
	if (!fsm_addstate(env->fsm, &env->end_outer)) {
		goto alloc_fail;
	}
	if (!fsm_addedge_epsilon(env->fsm, env->end_inner, env->end_outer)) {
		goto alloc_fail;
	}

	fsm_setend(env->fsm, env->end_outer, 1);

#if LOG_LINKAGE
	fprintf(stderr, "end: outer %d, inner %d\n",
	    env->end_outer, env->end_inner);
#endif

#if LOG_TRAMPOLINE
	fprintf(stderr, "%s: x %d, y %d\n", __func__, x, env->end_inner);
#endif

	env->stack.ceil = DEF_COMP_STACK_CEIL;
	env->stack.depth = 1;
	env->stack.frames = frames;
	env->active_capture_ids = active_capture_ids;

	{			/* set up the first stack frame */
		struct comp_stack_frame *sf = &env->stack.frames[0];
		sf->n = ast->expr;
		sf->x = x;
		sf->y = env->end_inner;
		sf->step = 0;
	}

#if JUST_ONE_PROG
	uint32_t prog_id;
	if (use_captures) {
		if (!compile_capvm_program_for_stack_end_states(env, ast, &prog_id)) {
			goto alloc_fail;
		}
	}
#endif

	/* evaluate call stack until termination */
	while (res && env->stack.depth > 0) {
		if (!eval_stack_frame(env)) {
#if LOG_TRAMPOLINE
			fprintf(stderr, "%s: res -> 0\n", __func__);
#endif
			res = 0;
			break;
		}
	}

	if (use_captures && res && env->max_capture_id != AST_NO_MAX_CAPTURE_ID) {
		/* Set the active captures on the end state. */
		for (unsigned i = 0; i <= (unsigned)env->max_capture_id; i++) {
			if (!u64bitset_get(env->active_capture_ids, i)) {
				continue;
			}
			if (!fsm_capture_set_active_for_end(env->fsm, i, env->end_outer)) {
				goto alloc_fail;
			}
		}

#if !JUST_ONE_PROG
		uint32_t prog_id;
		if (!compile_capvm_program_for_stack_end_states(env, stack, ast, &prog_id)) {
			goto alloc_fail;
		}
#endif

#if LOG_TRAMPOLINE
		fprintf(stderr, "%s: associated prog_id %u with state %d\n",
		    __func__, prog_id, stack->end_outer);
#endif
		if (!fsm_capture_associate_program_with_end_state(env->fsm,
			prog_id, env->end_outer)) {
			goto alloc_fail;
		}
	}

	f_free(env->alloc, env->stack.frames);
	f_free(env->alloc, env->active_capture_ids);

	return res;

alloc_fail:
	/* TODO: set env->err to indicate alloc failure */
	if (frames != NULL) {
		f_free(env->alloc, frames);
	}
	if (active_capture_ids != NULL) {
		f_free(env->alloc, active_capture_ids);
	}

	return 0;
}

static struct comp_stack_frame *
get_comp_stack_top(struct comp_env *env)
{
	struct comp_stack *stack = &env->stack;
	assert(stack->depth > 0);
	struct comp_stack_frame *sf = &stack->frames[stack->depth - 1];
	assert(sf->n != NULL);
	return sf;
}

static int
eval_stack_frame(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);

#if LOG_TRAMPOLINE
	fprintf(stderr, "%s: depth %zu/%zu, type %s, step %u\n", __func__,
	    stack->depth, stack->ceil, ast_node_type_name(sf->n->type), sf->step);
#endif

	/* If this is the first time the trampoline has called this
	 * state, decide the linking. Some of the states below (such as
	 * AST_EXPR_CONCAT) can have multiple child nodes, so they will
	 * increment step and use it to resume where they left off as
	 * the trampoline returns execution to them. */
	enum link_types link_end;
	if (sf->step == 0) {	/* entering state */
		enum link_types link_start;
		link_start = decide_linking(env, sf->x, sf->y, sf->n, LINK_START);
		link_end   = decide_linking(env, sf->x, sf->y, sf->n, LINK_END);
		if (!set_linking(env, sf->n, link_start, link_end, &sf->x, &sf->y)) {
			return 0;
		}
	}

#if LOG_TRAMPOLINE > 1
	fprintf(stderr, "%s: x %d, y %d\n", __func__, sf->x, sf->y);
#endif

	switch (sf->n->type) {
	case AST_EXPR_EMPTY:
		return eval_EMPTY(env);
	case AST_EXPR_CONCAT:
		return eval_CONCAT(env);
	case AST_EXPR_ALT:
		return eval_ALT(env);
	case AST_EXPR_LITERAL:
		return eval_LITERAL(env);
	case AST_EXPR_CODEPOINT:
		return eval_CODEPOINT(env);
	case AST_EXPR_REPEAT:
		return eval_REPEAT(env);
	case AST_EXPR_GROUP:
		return eval_GROUP(env);
	case AST_EXPR_ANCHOR:
		return eval_ANCHOR(env);
	case AST_EXPR_SUBTRACT:
		return eval_SUBTRACT(env);
	case AST_EXPR_RANGE:
		return eval_RANGE(env);
	case AST_EXPR_TOMBSTONE:
		return eval_TOMBSTONE(env);
	default:
		assert(!"unreached");
		return 0;
	}
}

static int
eval_EMPTY(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
#if LOG_LINKAGE
	fprintf(stderr, "eval_EMPTY: step %u, x %d -> y %d\n",
	    sf->step, sf->x, sf->y);
#endif

	EPSILON(sf->x, sf->y);
	RETURN(env);
	return 1;
}

static int
eval_CONCAT(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
	struct ast_expr *n = sf->n;
	const size_t count = n->u.concat.count;
	assert(count >= 1);

#if LOG_LINKAGE
		fprintf(stderr, "comp_iter: eval_CONCAT: x %d, y %d, step %d\n",
		    sf->x, sf->y, sf->step);
#endif

	if (sf->step == 0) {
		sf->u.concat.link = sf->x;
	}

	while (sf->step < count) {
		fsm_state_t curr_x = sf->u.concat.link;
		struct ast_expr *curr = n->u.concat.n[sf->step];

		/* If a subtree is unsatisfiable but also nullable, ignore it. */
		const enum ast_flags nullable_and_unsat = AST_FLAG_NULLABLE
		    | AST_FLAG_UNSATISFIABLE;
		if ((curr->flags & nullable_and_unsat) == nullable_and_unsat) {
			sf->step++;

			/* if necessary, link the end */
			if (sf->step == count) {
				EPSILON(curr_x, sf->y);
			}
			return 1;
		}

		struct ast_expr *next = sf->step == count - 1
		    ? NULL
		    : n->u.concat.n[sf->step + 1];

		fsm_state_t z;
		if (sf->step + 1 < count) {
			if (!fsm_addstate(env->fsm, &z)) {
				return 0;
			}
#if LOG_LINKAGE
			fprintf(stderr, "%s: added state z %d\n", __func__, z);
#endif
		} else {
			z = sf->y; /* connect to right parent to close off subtree */
		}

#if LOG_LINKAGE
		fprintf(stderr, "%s: curr_x %d, z %d\n",
		    __func__, curr_x, z);
#endif

		/*
		 * If nullable, add an extra state & epsilon as a one-way gate
		 */
		if (!can_skip_concat_state_and_epsilon(curr, next)) {
			fsm_state_t diode;

			NEWSTATE(diode);
			EPSILON(curr_x, diode);
			curr_x = diode;
#if LOG_LINKAGE
			fprintf(stderr, "comp_iter: added diode %d\n", diode);
#endif
		}

#if LOG_LINKAGE
		fprintf(stderr, "comp_iter: recurse CONCAT[%u/%zu]: link %d, z %d\n",
		    sf->step, count, sf->u.concat.link, z);
#endif
		/* Set the right side link, which will become the
		 * left side link for the next step (if any). */
		sf->u.concat.link = z;
		sf->step++;
		RECURSE(env, curr_x, z, curr);
		return 1;
	}

	RETURN(env);
	return 1;
}

static int
eval_ALT(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
	const size_t count = sf->n->u.alt.count;
	assert(count >= 1);

#if LOG_LINKAGE
	fprintf(stderr, "eval_ALT: step %u\n", sf->step);
#endif

	if (sf->step < count) {
		struct ast_expr *n;

		/*
		 * CONCAT handles adding extra states and
		 * epsilons when necessary, so there isn't much
		 * more to do here.
		 */
#if LOG_LINKAGE
		fprintf(stderr, "eval_ALT: recurse ALT[%u/%zu]: x %d, y %d\n",
		    sf->step, count, sf->x, sf->y);
#endif

		n = sf->n->u.alt.n[sf->step];
		assert(n != NULL);
		sf->step++;	/* RECURSE can realloc the stack and make sf stale. */

		if (!(n->flags & AST_FLAG_UNSATISFIABLE)) {
			RECURSE(env, sf->x, sf->y, n);
		}
		return 1;
	}

	RETURN(env);
	return 1;
}

static int
eval_LITERAL(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
	struct ast_expr *n = sf->n;
#if LOG_LINKAGE
	fprintf(stderr, "%s: linking %d -> %d with literal '%c' (0x%02x)\n",
	    __func__, sf->x, sf->y, isprint(n->u.literal.c) ? n->u.literal.c : '.',
	    n->u.literal.c);
#endif

	LITERAL(sf->x, sf->y, n->u.literal.c);

	RETURN(env);
	return 1;
}

static int
eval_CODEPOINT(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
	struct ast_expr *n = sf->n;
	fsm_state_t a, b;
	char c[4];
	int r, i;

	r = utf8(n->u.codepoint.u, c);
	if (!r) {
		if (env->err != NULL) {
			env->err->e = RE_EBADCP;
			env->err->cp = n->u.codepoint.u;
		}

		return 0;
	}

	a = sf->x;

	for (i = 0; i < r; i++) {
		if (i + 1 < r) {
			NEWSTATE(b);
		} else {
			b = sf->y;
		}

		LITERAL(a, b, c[i]);

		a = b;
	}

	RETURN(env);
	return 1;
}

static int
eval_REPEAT(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
	fsm_state_t a, b;
	unsigned i, min, max;

	assert(sf->n->type == AST_EXPR_REPEAT);
	struct ast_expr_repeat *n = &sf->n->u.repeat;

	min = n->min;
	max = n->max;

	assert(min <= max);

	if (min == 0 && max == 0) {                          /* {0,0} */
		EPSILON(sf->x, sf->y);
		RETURN(env);
		return 1;
	} else if (min == 0 && max == 1) {                   /* '?' */
		EPSILON(sf->x, sf->y);
		TAILCALL(env, sf->x, sf->y, n->e);
		return 1;
	} else if (min == 1 && max == 1) {                   /* {1,1} */
		TAILCALL(env, sf->x, sf->y, n->e);
		return 1;
	} else if (min == 0 && max == AST_COUNT_UNBOUNDED) { /* '*' */
		fsm_state_t na, nz;
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(sf->x,na);
		EPSILON(nz,sf->y);

		EPSILON(na, nz);
		EPSILON(nz, na);
		TAILCALL(env, na, nz, n->e);
		return 1;
	} else if (min == 1 && max == AST_COUNT_UNBOUNDED) { /* '+' */
		fsm_state_t na, nz;
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(sf->x, na);
		EPSILON(nz, sf->y);

		EPSILON(nz, na);
		TAILCALL(env, na, nz, n->e);
		return 1;
	} else if (sf->step == 0) {
		/*
		 * Make new beginning/end states for the repeated section,
		 * build its NFA, and link to its head.
		 */

		fsm_subgraph_start(env->fsm, &sf->u.repeat.subgraph);

		sf->step++;	/* resume after RECURSE */
		NEWSTATE(sf->u.repeat.na);
		NEWSTATE(sf->u.repeat.nz);
		RECURSE(env, sf->u.repeat.na, sf->u.repeat.nz, n->e);
		return 1;
	} else {
		fsm_state_t tail;
		assert(sf->step == 1);
		EPSILON(sf->x, sf->u.repeat.na); /* link head to repeated NFA head */

		b = sf->u.repeat.nz; /* set the initial tail */

		/* can be skipped */
		if (min == 0) {
			EPSILON(sf->u.repeat.na, sf->u.repeat.nz);
		}
		fsm_subgraph_stop(env->fsm, &sf->u.repeat.subgraph);
		tail = sf->u.repeat.nz;

		if (max != AST_COUNT_UNBOUNDED) {
			for (i = 1; i < max; i++) {
				/* copies the original subgraph; need to set b to the
				 * original tail
				 */
				b = tail;

				if (!fsm_subgraph_duplicate(env->fsm, &sf->u.repeat.subgraph, &b, &a)) {
					return 0;
				}

				EPSILON(sf->u.repeat.nz, a);

				/* To the optional part of the repeated count */
				if (i >= min) {
					EPSILON(sf->u.repeat.nz, b);
				}

				sf->u.repeat.na = a;	/* advance head for next duplication */
				sf->u.repeat.nz = b;	/* advance tail for concenation */
			}
		} else {
			for (i = 1; i < min; i++) {
				/* copies the original subgraph; need to set b to the
				 * original tail
				 */
				b = tail;

				if (!fsm_subgraph_duplicate(env->fsm, &sf->u.repeat.subgraph, &b, &a)) {
					return 0;
				}

				EPSILON(sf->u.repeat.nz, a);

				sf->u.repeat.na = a;	/* advance head for next duplication */
				sf->u.repeat.nz = b;	/* advance tail for concenation */
			}

			/* back link to allow for infinite repetition */
			EPSILON(sf->u.repeat.nz, sf->u.repeat.na);
		}

		/* tail to last repeated NFA tail */
		EPSILON(sf->u.repeat.nz, sf->y);
		RETURN(env);
		return 1;
	}
}

static void
set_active_capture_ids(struct comp_env *env, unsigned id)
{
#if LOG_LINKAGE
	fprintf(stderr, "set_active_capture_ids: id %u\n", id);
#endif
	assert(env->active_capture_ids != NULL);
	u64bitset_set(env->active_capture_ids, id);
}

static int
eval_GROUP(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);

	if (env->re_flags & RE_NOCAPTURE) {
		/* passthrough, disable captures */
		if (sf->step == 0) {
			sf->step++;
			RECURSE(env, sf->x, sf->y, sf->n->u.group.e);
		} else {
			RETURN(env);
		}
		return 1;
	}

	if (sf->step == 0) {
		struct ast_expr *n = sf->n;
		set_active_capture_ids(env, n->u.group.id);

#if LOG_LINKAGE
		fprintf(stderr, "comp_iter: recurse GROUP: x %d, y %d\n",
		    sf->x, sf->y);
#endif
		sf->step++;
		RECURSE(env, sf->x, sf->y, n->u.group.e);
		return 1;
	} else {
		assert(sf->step == 1);

		RETURN(env);
		return 1;
	}
}

static int
eval_ANCHOR(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
#if 1

#if LOG_LINKAGE
	fprintf(stderr, "%s: ignoring anchor node %p, epsilon %d -> %d\n",
	    __func__, (void *)sf->n, sf->x, sf->y);
#endif
	EPSILON(sf->x, sf->y);
#else
	switch (sf->n->u.anchor.type) {
	case AST_ANCHOR_START:
		if (!(sf->n->flags & AST_FLAG_FIRST)) {
#if LOG_LINKAGE
			fprintf(stderr, "%s: ignoring START anchor in non-FIRST location\n",
			    __func__);
#endif
			EPSILON(sf->x, sf->y);
			break;
		}

#if LOG_LINKAGE
		fprintf(stderr, "%s: START anchor %p epsilon-linking %d -> %d\n",
		    __func__, (void *)sf->n, env->start_inner, sf->y);
#endif
		EPSILON(env->start_inner, sf->y);
		break;

	case AST_ANCHOR_END:
		if (!(sf->n->flags & AST_FLAG_LAST)) {
#if LOG_LINKAGE
			fprintf(stderr, "%s: ignoring END anchor in non-LAST location\n",
			    __func__);
#endif
			EPSILON(sf->x, sf->y);
			break;
		}

#if LOG_LINKAGE
		fprintf(stderr, "%s: END anchor %p epsilon-linking %d -> %d\n",
		    __func__, (void *)sf->n, sf->x, stack->end_inner);
#endif
		EPSILON(sf->x, stack->end_inner);
		break;

	default:
		assert(!"unreached");
		return 0;
	}
#endif

	RETURN(env);
	return 1;
}

static int
eval_SUBTRACT(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);

	struct fsm *a, *b;
	struct fsm *q;
	enum re_flags re_flags = sf->n->re_flags;

	/* wouldn't want to reverse twice! */
	re_flags &= ~(unsigned)RE_REVERSE;

	/* Don't compile capture resolution programs again for the
	 * subtrees, just ignore capture behavior. */
	re_flags |= RE_NOCAPTURE;

	a = expr_compile(sf->n->u.subtract.a, re_flags,
	    fsm_getoptions(env->fsm), env->err);
	if (a == NULL) {
		return 0;
	}

	b = expr_compile(sf->n->u.subtract.b, re_flags,
	    fsm_getoptions(env->fsm), env->err);
	if (b == NULL) {
		fsm_free(a);
		return 0;
	}

	q = fsm_subtract(a, b);
	if (q == NULL) {
		return 0;
	}

	/*
	 * Subtraction produces quite a mess. We could trim or minimise here
	 * while q is self-contained, which might work out better than doing it
	 * in the larger FSM after merge. I'm not sure if it works out better
	 * overall or not.
	 */

	if (fsm_empty(q)) {
		EPSILON(sf->x, sf->y);
		RETURN(env);
		return 1;
	}

	if (!fsm_unionxy(env->fsm, q, sf->x, sf->y)) {
		return 0;
	}

	RETURN(env);
	return 1;
}

static int
eval_RANGE(struct comp_env *env)
{
	struct comp_stack_frame *sf = get_comp_stack_top(env);
	struct ast_expr *n = sf->n;
	unsigned int i;

	if (n->u.range.from.type != AST_ENDPOINT_LITERAL || n->u.range.to.type != AST_ENDPOINT_LITERAL) {
		/* not yet supported */
		return 0;
	}

	assert(n->u.range.from.u.literal.c <= n->u.range.to.u.literal.c);

	if (n->u.range.from.u.literal.c == 0x00 &&
	    n->u.range.to.u.literal.c == 0xff)
		{
			ANY(sf->x, sf->y);
			RETURN(env);
			return 1;
		}

	for (i = n->u.range.from.u.literal.c; i <= n->u.range.to.u.literal.c; i++) {
		LITERAL(sf->x, sf->y, i);
	}

	RETURN(env);
	return 1;
}

static int
eval_TOMBSTONE(struct comp_env *env)
{
	/* do not link -- intentionally pruned */
	(void)env;
	RETURN(env);
	return 1;
}

#undef EPSILON
#undef ANY
#undef NEWSTATE
#undef LITERAL
#undef RECURSE
#undef RETURN
#undef TAILCALL

struct fsm *
ast_compile(const struct ast *ast,
	enum re_flags re_flags,
	const struct fsm_options *opt,
	struct re_err *err)
{
	/* Start states inside and outside of match group 0,
	 * which represents the entire matched input, but does not
	 * include the implied /.*?/ loop at the start or end when
	 * a regex is unanchored. */
	fsm_state_t start_outer, start_inner;
	struct fsm *fsm;

	assert(ast != NULL);

#if LOG_LINKAGE
	ast_print_tree(stderr, opt, re_flags, ast);
#endif

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		return NULL;
	}

	/* TODO: move these to the call stack, for symmetry?
	 * Or possibly combine comp_env and stack. */
	if (!fsm_addstate(fsm, &start_outer)) {
		goto error;
	}

	if (!fsm_addstate(fsm, &start_inner)) {
		goto error;
	}

	if (!fsm_addedge_epsilon(fsm, start_outer, start_inner)) {
		goto error;
	}

	fsm_setstart(fsm, start_outer);

#if LOG_LINKAGE
	fprintf(stderr, "start: outer %d, inner %d\n",
	    start_outer, start_inner);
#endif

	{
		struct comp_env env;

		memset(&env, 0x00, sizeof(env));

		env.alloc = fsm->opt->alloc;
		env.fsm = fsm;
		env.re_flags = re_flags;
		env.err = err;

		env.start_inner = start_inner;
		env.start_outer = start_outer;

		env.max_capture_id = ast->max_capture_id;

		if (!comp_iter(&env, start_inner, ast)) {
			if (err != NULL && err->e == 0) {
				err->e = RE_EBADGROUP;
			}
			goto error;
		}
	}

/* XXX:
	if (-1 == fsm_trim(fsm)) {
		goto error;
	}
*/

	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (re_flags & RE_REVERSE) {
		if (!fsm_reverse(fsm)) {
			goto error;
		}
	}

	return fsm;

error:

	fsm_free(fsm);

	return NULL;
}

static int
compile_capvm_program_for_stack_end_states(struct comp_env *env,
	const struct ast *ast, uint32_t *prog_id)
{
	/* compile and save program in ^, associate its id w/ end state */
	enum re_capvm_compile_ast_res res;
	struct capvm_program *prog;
	res = re_capvm_compile_ast(env->alloc,
	    ast, env->re_flags, &prog);
	if (res != RE_CAPVM_COMPILE_AST_OK) {
		if (env->err != NULL && env->err->e == 0 && errno != 0) {
			env->err->e = RE_EERRNO;
		}
		return 0;
	}

	if (!fsm_capture_add_program(env->fsm, prog, prog_id)) {
		return 0;
	}

	return 1;
}
