#ifndef AST_COMPILE_INTERNAL_H
#define AST_COMPILE_INTERNAL_H

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
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

#include "class.h"
#include "ast.h"
#include "ast_compile.h"

#include "libfsm/internal.h" /* XXX */

#define LOG_LINKAGE 0
#define LOG_CAPTURE_PATHS 0
#define LOG_TRAMPOLINE 0

enum link_side {
	LINK_START,
	LINK_END
};

/*
 * How should this state be linked for the relevant state?
 * Note: These are not mutually exclusive!
 *
 * - LINK_TOP_DOWN
 *   Use the passed in start/end states (x and y)
 *
 * - LINK_GLOBAL
 *   Link to the global start/end state (env->start_outer or env->end_outer),
 *   because this node has a ^ or $ anchor
 *
 * - LINK_GLOBAL_SELF_LOOP
 *   Link to the unanchored self loop adjacent to the start/end
 *   states (env->start_any_loop or env->end_any_loop), because
 *   this node is in a FIRST or LAST position, but unanchored.
 */
enum link_types {
	LINK_TOP_DOWN         = 1 << 0,
	LINK_GLOBAL           = 1 << 1,
	LINK_GLOBAL_SELF_LOOP = 1 << 2,

	LINK_NONE = 0x00
};

/* Call stack for AST -> NFA conversion.
 *
 * This is executed depth first and could otherwise use the C call
 * stack, but it needs to support forking execution in order to handle
 * AST_EXPR_ALTs where ALT subtrees have different capture behavior.
 * Passing through AST_EXPR_GROUP nodes affect the remainder of
 * evaluation (even after returning from their subtree), because they
 * modify which captures will be associated with particular end states.
 * When they only appear in some ALTs we need to fork the remainder of
 * execution, including evaluating everything pending on the call stack
 * multiple times. */
#define DEF_COMP_STACK_CEIL 4
struct comp_stack {
	size_t ceil;
	size_t depth;		/* 0 -> empty */

	fsm_state_t end_inner;
	fsm_state_t end_outer;
	fsm_state_t end_any_loop;
	fsm_state_t end_any_inner;
	int have_end_any_loop;

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
};

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
};

static int
comp_iter_trampoline(struct comp_env *env,
	fsm_state_t x, struct ast_expr *n);

static int
eval_stack_frame(struct comp_env *env, struct comp_stack *stack);

static int
eval_EMPTY(struct comp_env *env, struct comp_stack *stack);
static int
eval_CONCAT(struct comp_env *env, struct comp_stack *stack);
static int
eval_ALT(struct comp_env *env, struct comp_stack *stack);
static int
eval_LITERAL(struct comp_env *env, struct comp_stack *stack);
static int
eval_CODEPOINT(struct comp_env *env, struct comp_stack *stack);
static int
eval_REPEAT(struct comp_env *env, struct comp_stack *stack);
static int
eval_GROUP(struct comp_env *env, struct comp_stack *stack);
static int
eval_ANCHOR(struct comp_env *env, struct comp_stack *stack);
static int
eval_SUBTRACT(struct comp_env *env, struct comp_stack *stack);
static int
eval_RANGE(struct comp_env *env, struct comp_stack *stack);
static int
eval_TOMBSTONE(struct comp_env *env, struct comp_stack *stack);

#endif
