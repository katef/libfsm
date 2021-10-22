/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "ast_compile_internal.h"

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

		for (i = 0; i < b->statecount; i++) {
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
		if (!fsm_addedge_literal(fsm, from, to, tolower((unsigned char) c))) {
			return 0;
		}

		if (!fsm_addedge_literal(fsm, from, to, toupper((unsigned char) c))) {
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
	fprintf(stderr, "start_any: loop %d, inner: %d\n", loop, inner);
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
intern_end_any_loop(struct comp_env *env, struct comp_stack *stack)
{
	fsm_state_t loop, inner;

	assert(env != NULL);

	if (stack->have_end_any_loop) {
		return 1;
	}

	assert(~env->re_flags & RE_ANCHORED);
	assert(stack->end_outer < env->fsm->statecount);

	if (!fsm_addstate(env->fsm, &loop)) {
		return 0;
	}
	if (!fsm_addstate(env->fsm, &inner)) {
		return 0;
	}

#if LOG_LINKAGE
	fprintf(stderr, "end_any: %d, inner: %d\n", loop, inner);
#endif

	if (!fsm_addedge_any(env->fsm, loop, loop)) {
		return 0;
	}

	if (!fsm_addedge_epsilon(env->fsm, inner, loop)) {
		return 0;
	}
	if (!fsm_addedge_epsilon(env->fsm, loop, stack->end_outer)) {
		return 0;
	}

	stack->end_any_loop = loop;
	stack->end_any_inner = inner;
	stack->have_end_any_loop = 1;

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
		/* XXX: not sure */
		return 1;

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

static enum link_types
decide_linking(struct comp_env *env, struct comp_stack *stack,
	fsm_state_t x, fsm_state_t y,
	struct ast_expr *n, enum link_side side)
{
	enum link_types res = LINK_NONE;

	assert(n != NULL);
	assert(env != NULL);

	if ((env->re_flags & RE_ANCHORED)) {
		return LINK_TOP_DOWN;
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		if ((n->flags & (AST_FLAG_FIRST|AST_FLAG_LAST)) == 0) {
			return LINK_TOP_DOWN;
		}
		break;

	case AST_EXPR_GROUP:
		return LINK_TOP_DOWN;

	case AST_EXPR_ANCHOR:
		if (n->u.anchor.type == AST_ANCHOR_START && side == LINK_START) {
			return LINK_GLOBAL;
		}
		if (n->u.anchor.type == AST_ANCHOR_END && side == LINK_END) {
			return LINK_GLOBAL;
		}

		break;

	case AST_EXPR_SUBTRACT:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:

	case AST_EXPR_CONCAT:
	case AST_EXPR_ALT:
	case AST_EXPR_REPEAT:
	case AST_EXPR_RANGE:
	case AST_EXPR_TOMBSTONE:
		break;

	default:
		assert(!"unreached");
	}

	switch (side) {
	case LINK_START: {
		const int start    = (n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_START);
		const int first    = (n->flags & AST_FLAG_FIRST) != 0;
		const int nullable = (n->flags & AST_FLAG_NULLABLE) != 0;

		(void) nullable;

		if (!start && first) {
			if (x == env->start_inner) {
				/* Avoid a cycle back to env->start that may
				 * lead to incorrect matches, e.g. /a?^b*/
				return LINK_GLOBAL_SELF_LOOP;
			} else {
				/* Link in the starting self-loop, but also the
				 * previous state (if any), because it can
				 * indicate matching a nullable state. */
				return LINK_GLOBAL_SELF_LOOP | LINK_TOP_DOWN;
			}
		}

		if (start && !first) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;
	}

	case LINK_END: {
		const int end      = (n->type == AST_EXPR_ANCHOR && n->u.anchor.type == AST_ANCHOR_END);
		const int last     = (n->flags & AST_FLAG_LAST) != 0;
		const int nullable = (n->flags & AST_FLAG_NULLABLE) != 0;

		(void) nullable;

		/* FIXME: Clarify why this is asymmetric to the cases for START,
		 * or confirm that it doesn't need to be. */

		if (end && last) {
			if (y == stack->end_inner) {
				return LINK_GLOBAL;
			} else {
				return LINK_GLOBAL | LINK_TOP_DOWN;
			}
		}

		if (!end && last) {
			if (y == stack->end_inner) {
				return LINK_GLOBAL_SELF_LOOP;
			} else {
				return LINK_GLOBAL_SELF_LOOP | LINK_TOP_DOWN;
			}
		}

		if (end && !last) {
			return LINK_GLOBAL;
		}

		return LINK_TOP_DOWN;
	}

	default:
		assert(!"unreached");
	}

	assert(res != LINK_NONE);

	return res;
}

static void
print_linkage(enum link_types t)
{
	if (t == LINK_NONE) {
		fprintf(stderr, "NONE");
		return;
	}

	if (t & LINK_TOP_DOWN) {
		fprintf(stderr, "[TOP_DOWN]");
	}
	if (t & LINK_GLOBAL) {
		fprintf(stderr, "[GLOBAL]");
	}
	if (t & LINK_GLOBAL_SELF_LOOP) {
		fprintf(stderr, "[SELF_LOOP]");
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

#define RETURN(STK) comp_stack_pop(STK)

#define RECURSE(ENV, STACK, FROM, TO, NODE)				\
	if (!comp_stack_push(ENV, STACK, (FROM), (TO), (NODE))) { return 0; }

#define TAILCALL(STACK, FROM, TO, NODE)					\
	comp_stack_tailcall(STACK, (FROM), (TO), (NODE));


#if LOG_LINKAGE || LOG_TRAMPOLINE
static const char *
node_type_name(enum ast_expr_type t)
{
	switch (t) {
	default:
		return "<match fail>";
	case AST_EXPR_EMPTY: return "EMPTY";
	case AST_EXPR_CONCAT: return "CONCAT";
	case AST_EXPR_ALT: return "ALT";
	case AST_EXPR_LITERAL: return "LITERAL";
	case AST_EXPR_CODEPOINT: return "CODEPOINT";
	case AST_EXPR_REPEAT: return "REPEAT";
	case AST_EXPR_GROUP: return "GROUP";
	case AST_EXPR_ANCHOR: return "ANCHOR";
	case AST_EXPR_SUBTRACT: return "SUBTRACT";
	case AST_EXPR_RANGE: return "RANGE";
	case AST_EXPR_TOMBSTONE: return "TOMBSTONE";
	}
}
#endif

static int
set_linking(struct comp_env *env, struct comp_stack *stack, struct ast_expr *n,
    enum link_types link_start, enum link_types link_end,
    fsm_state_t *px, fsm_state_t *py)
{
	fsm_state_t x = *px;
	fsm_state_t y = *py;

#if LOG_LINKAGE
	fprintf(stderr, "%s: decide_linking %p [%s]: start ",
	    __func__, (void *) n, node_type_name(n->type));
	print_linkage(link_start);
	fprintf(stderr, ", end ");
	print_linkage(link_end);
	fprintf(stderr, ", x %d, y %d\n", x, y);
#else
	(void) print_linkage;
	(void)n;
#endif

	if ((link_start & LINK_TOP_DOWN) == LINK_NONE) {
		/*
		 * The top-down link is rejected, so replace x with
		 * either the NFA's global start state or the self-loop
		 * at the start. These _are_ mutually exclusive.
		 */
		if (link_start & LINK_GLOBAL) {
			assert((link_start & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			x = env->start_inner;
		} else if (link_start & LINK_GLOBAL_SELF_LOOP) {
			assert((link_start & LINK_GLOBAL) == LINK_NONE);

			if (!intern_start_any_loop(env)) {
				return 0;
			}

			assert(env->have_start_any_loop);
			x = env->start_any_inner;
		}
	} else {
		/*
		 * The top-down link is still being used, so connect to the
		 * global start/start-self-loop state with an epsilon.
		 */
		if (link_start & LINK_GLOBAL) {
			assert((link_start & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			EPSILON(env->start_inner, x);
		} else if (link_start & LINK_GLOBAL_SELF_LOOP) {
			assert((link_start & LINK_GLOBAL) == LINK_NONE);
		}
	}

	if ((link_end & LINK_TOP_DOWN) == LINK_NONE) {
		/*
		 * The top-down link is rejected, so replace x with
		 * either the NFA's global end state or the self-loop
		 * at the end. These _are_ mutually exclusive.
		 */
		if (link_end & LINK_GLOBAL) {
			assert((link_end & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			y = stack->end_inner;
		} else if (link_end & LINK_GLOBAL_SELF_LOOP) {
			assert((link_end & LINK_GLOBAL) == LINK_NONE);

			if (!intern_end_any_loop(env, stack)) {
				return 0;
			}

			assert(stack->have_end_any_loop);
			y = stack->end_any_inner;
		}
	} else {
		/*
		 * The top-down link is still being used, so connect to the
		 * global end/end-self-loop state with an epsilon.
		 */
		if (link_end & LINK_GLOBAL) {
			assert((link_end & LINK_GLOBAL_SELF_LOOP) == LINK_NONE);
			EPSILON(y, stack->end_inner);
		} else if (link_end & LINK_GLOBAL_SELF_LOOP) {
			assert((link_end & LINK_GLOBAL) == LINK_NONE);
		}
	}

#if LOG_LINKAGE
	fprintf(stderr, " ---> x: %d, y: %d\n", x, y);
#endif
	*px = x;
	*py = y;
	return 1;
}

static void
comp_stack_pop(struct comp_stack *stack)
{
	assert(stack != NULL);
	assert(stack->depth > 0);
	stack->depth--;
}

static int
comp_stack_push(struct comp_env *env, struct comp_stack *stack,
	fsm_state_t x, fsm_state_t y, struct ast_expr *n)
{
	assert(stack != NULL);
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
comp_stack_tailcall(struct comp_stack *stack,
	fsm_state_t x, fsm_state_t y, struct ast_expr *n)
{
	assert(stack != NULL);

	assert(stack->depth > 0);

	/* Replace current stack frame. */
	struct comp_stack_frame *sf = &stack->frames[stack->depth - 1];
	memset(sf, 0x00, sizeof(*sf));
	sf->n = n;
	sf->x = x;
	sf->y = y;
}

static int
comp_iter_trampoline(struct comp_env *env,
	fsm_state_t x, struct ast_expr *n)
{
	int res = 1;
	assert(n != NULL);

	struct comp_stack *stack = NULL;
	struct comp_stack_frame *frames = NULL;

	stack = f_calloc(env->alloc, 1, sizeof(*stack));
	if (stack == NULL) {
		goto alloc_fail;
	}
	frames = f_calloc(env->alloc,
	    DEF_COMP_STACK_CEIL, sizeof(stack[0]));
	if (frames == NULL) {
		goto alloc_fail;
	}

	/* Add inner and outer end states. Like start_outer and start_inner,
	 * these represent the boundary between match group 0 (inner) and
	 * states outside it (the unanchored end loop). */
	if (!fsm_addstate(env->fsm, &stack->end_inner)) {
		goto alloc_fail;
	}
	if (!fsm_addstate(env->fsm, &stack->end_outer)) {
		goto alloc_fail;
	}
	if (!fsm_addedge_epsilon(env->fsm, stack->end_inner, stack->end_outer)) {
		goto alloc_fail;
	}

	fsm_setend(env->fsm, stack->end_outer, 1);

#if LOG_LINKAGE
	fprintf(stderr, "end: outer %d, inner %d\n",
	    stack->end_outer, stack->end_inner);
#endif

#if LOG_TRAMPOLINE
	fprintf(stderr, "%s: x %d, y %d\n", __func__, x, stack->end_inner);
#endif

	stack->ceil = DEF_COMP_STACK_CEIL;
	stack->depth = 1;
	stack->frames = frames;

	{			/* set up the first stack frame */
		struct comp_stack_frame *sf = &stack->frames[0];
		sf->n = n;
		sf->x = x;
		sf->y = stack->end_inner;
		sf->step = 0;
	}

	/* Evaluate call stack.
	 *
	 * This is a loop because changes on a branch (not yet integrated) will
	 * add support for forking execution by scheduling a branched copy of the
	 * call stack to resume when evaluation of this one completes. */
	while (stack != NULL) {
#if LOG_TRAMPOLINE
		fprintf(stderr, "%s: executing stack %p\n", __func__, (void *)stack);
#endif

		/* evaluate call stack until termination */
		while (res && stack->depth > 0) {
			if (!eval_stack_frame(env, stack)) {
#if LOG_TRAMPOLINE
				fprintf(stderr, "%s: res -> 0\n", __func__);
#endif
				res = 0;
				break;
			}
		}

		f_free(env->alloc, stack->frames);
		f_free(env->alloc, stack);
		stack = NULL;
	}

	return res;

alloc_fail:
	/* TODO: set env->err to indicate alloc failure */
	if (stack != NULL) {
		f_free(env->alloc, stack);
	}
	if (frames != NULL) {
		f_free(env->alloc, frames);
	}

	return 0;
}

static struct comp_stack_frame *
get_comp_stack_top(struct comp_stack *stack)
{
	assert(stack->depth > 0);
	struct comp_stack_frame *sf = &stack->frames[stack->depth - 1];
	assert(sf->n != NULL);
	return sf;
}

static int
eval_stack_frame(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);

#if LOG_TRAMPOLINE
	fprintf(stderr, "%s: depth %zu/%zu, type %s, step %u\n", __func__,
	    stack->depth, stack->ceil, node_type_name(sf->n->type), sf->step);
#endif

	/* If this is the first time the trampoline has called this
	 * state, decide the linking. Some of the states below (such as
	 * AST_EXPR_CONCAT) can have multiple child nodes, so they will
	 * increment step and use it to resume where they left off as
	 * the trampoline returns execution to them. */
	if (sf->step == 0) {	/* entering state */
		enum link_types link_start, link_end;
		link_start = decide_linking(env, stack, sf->x, sf->y, sf->n, LINK_START);
		link_end   = decide_linking(env, stack, sf->x, sf->y, sf->n, LINK_END);
		if (!set_linking(env, stack, sf->n, link_start, link_end, &sf->x, &sf->y)) {
			return 0;
		}
	}

#if LOG_TRAMPOLINE > 1
	fprintf(stderr, "%s: x %d, y %d\n", __func__, sf->x, sf->y);
#endif

	switch (sf->n->type) {
	case AST_EXPR_EMPTY:
		return eval_EMPTY(env, stack);
	case AST_EXPR_CONCAT:
		return eval_CONCAT(env, stack);
	case AST_EXPR_ALT:
		return eval_ALT(env, stack);
	case AST_EXPR_LITERAL:
		return eval_LITERAL(env, stack);
	case AST_EXPR_CODEPOINT:
		return eval_CODEPOINT(env, stack);
	case AST_EXPR_REPEAT:
		return eval_REPEAT(env, stack);
	case AST_EXPR_GROUP:
		return eval_GROUP(env, stack);
	case AST_EXPR_ANCHOR:
		return eval_ANCHOR(env, stack);
	case AST_EXPR_SUBTRACT:
		return eval_SUBTRACT(env, stack);
	case AST_EXPR_RANGE:
		return eval_RANGE(env, stack);
	case AST_EXPR_TOMBSTONE:
		return eval_TOMBSTONE(env, stack);
	default:
		assert(!"unreached");
		return 0;
	}
}

static int
eval_EMPTY(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
	EPSILON(sf->x, sf->y);
	RETURN(stack);
	return 1;
}

static int
eval_CONCAT(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
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
		struct ast_expr *next = sf->step == count - 1
		    ? NULL
		    : n->u.concat.n[sf->step + 1];

		fsm_state_t z;
		if (sf->step + 1 < count) {
			if (!fsm_addstate(env->fsm, &z)) {
				return 0;
			}
		} else {
			z = sf->y; /* connect to right parent to close off subtree */
		}

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
		RECURSE(env, stack, curr_x, z, curr);
		return 1;
	}

	RETURN(stack);
	return 1;
}

static int
eval_ALT(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
	const size_t count = sf->n->u.alt.count;
	assert(count >= 1);

#if LOG_LINKAGE
	fprintf(stderr, "eval_ALT: stack %p, step %u\n", (void *)stack, sf->step);
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
		RECURSE(env, stack, sf->x, sf->y, n);
		return 1;
	}

	RETURN(stack);
	return 1;
}

static int
eval_LITERAL(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
	struct ast_expr *n = sf->n;
	LITERAL(sf->x, sf->y, n->u.literal.c);
	RETURN(stack);
	return 1;
}

static int
eval_CODEPOINT(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
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

	RETURN(stack);
	return 1;
}

static int
eval_REPEAT(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
	fsm_state_t a, b;
	unsigned i, min, max;

	assert(sf->n->type == AST_EXPR_REPEAT);
	struct ast_expr_repeat *n = &sf->n->u.repeat;

	min = n->min;
	max = n->max;

	assert(min <= max);

	if (min == 0 && max == 0) {                          /* {0,0} */
		EPSILON(sf->x, sf->y);
		RETURN(stack);
		return 1;
	} else if (min == 0 && max == 1) {                   /* '?' */
		EPSILON(sf->x, sf->y);
		TAILCALL(stack, sf->x, sf->y, n->e);
		return 1;
	} else if (min == 1 && max == 1) {                   /* {1,1} */
		TAILCALL(stack, sf->x, sf->y, n->e);
		return 1;
	} else if (min == 0 && max == AST_COUNT_UNBOUNDED) { /* '*' */
		fsm_state_t na, nz;
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(sf->x,na);
		EPSILON(nz,sf->y);

		EPSILON(na, nz);
		EPSILON(nz, na);
		TAILCALL(stack, na, nz, n->e);
		return 1;
	} else if (min == 1 && max == AST_COUNT_UNBOUNDED) { /* '+' */
		fsm_state_t na, nz;
		NEWSTATE(na);
		NEWSTATE(nz);
		EPSILON(sf->x, na);
		EPSILON(nz, sf->y);

		EPSILON(nz, na);
		TAILCALL(stack, na, nz, n->e);
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
		RECURSE(env, stack, sf->u.repeat.na, sf->u.repeat.nz, n->e);
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
		RETURN(stack);
		return 1;
	}
}

static int
eval_GROUP(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);

	/* TODO: This could currently just TAILCALL in step 0, but
	 * will soon do additional bookkeeping to reflect which captures
	 * are active within the subtree. */

	if (sf->step == 0) {
		struct ast_expr *n = sf->n;
#if LOG_LINKAGE
		fprintf(stderr, "comp_iter: recurse GROUP: x %d, y %d\n",
		    sf->x, sf->y);
#endif
		sf->step++;
		RECURSE(env, stack, sf->x, sf->y, n->u.group.e);
		return 1;
	} else {
		assert(sf->step == 1);
		RETURN(stack);
		return 1;
	}
}

static int
eval_ANCHOR(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
	switch (sf->n->u.anchor.type) {
	case AST_ANCHOR_START:
		EPSILON(env->start_inner, sf->y);
		break;

	case AST_ANCHOR_END:
		EPSILON(sf->x, stack->end_inner);
		break;

	default:
		assert(!"unreached");
		return 0;
	}

	RETURN(stack);
	return 1;
}

static int
eval_SUBTRACT(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);

	struct fsm *a, *b;
	struct fsm *q;
	enum re_flags re_flags = sf->n->re_flags;

	/* wouldn't want to reverse twice! */
	re_flags &= ~(unsigned)RE_REVERSE;

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
		RETURN(stack);
		return 1;
	}

	if (!fsm_unionxy(env->fsm, q, sf->x, sf->y)) {
		return 0;
	}

	RETURN(stack);
	return 1;
}

static int
eval_RANGE(struct comp_env *env, struct comp_stack *stack)
{
	struct comp_stack_frame *sf = get_comp_stack_top(stack);
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
			RETURN(stack);
			return 1;
		}

	for (i = n->u.range.from.u.literal.c; i <= n->u.range.to.u.literal.c; i++) {
		LITERAL(sf->x, sf->y, i);
	}

	RETURN(stack);
	return 1;
}

static int
eval_TOMBSTONE(struct comp_env *env, struct comp_stack *stack)
{
	/* do not link -- intentionally pruned */
	(void)env;
	(void)stack;
	RETURN(stack);
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

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		return NULL;
	}

	/* TODO: move to the call stack, for symmetry? */
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

		if (!comp_iter_trampoline(&env, start_inner, ast->expr)) {
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
