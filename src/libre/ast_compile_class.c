/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/options.h>

#include <re/re.h>

#include "class.h"
#include "ast.h"
#include "ast_compile.h"

#include "libfsm/internal.h" /* XXX */

/*
 * Sort of an IR; this is our intermediate state when converting from a
 * character class's AST to link into an existing FSM. First we construct
 * stand-alone FSM for just the class, and these are held here.
 *
 * This struct is private to this file only, not even exposed as a
 * partial declaration. I'm reading "cc" as "compiled class", rather
 * than "character class".
 */
struct cc {
	enum re_flags re_flags;
	struct re_err *err;
};

/* FIXME: duplication */
static int
addedge_literal(struct fsm *fsm, enum re_flags flags,
	struct fsm_state *from, struct fsm_state *to, char c)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);
	
	if (flags & RE_ICASE) {
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
cc_add_named(struct cc *cc, struct fsm *fsm,
	class_constructor *ctor,
	struct fsm_state *x, struct fsm_state *y)
{
	assert(cc != NULL);
	assert(fsm != NULL);
	assert(ctor != NULL);
	assert(x != NULL);
	assert(y != NULL);

	if (!ctor(fsm, x, y)) {
		return 0;
	}

	return 1;
}

int
cc_add_char(struct cc *cc, struct fsm *fsm,
	unsigned char c,
	struct fsm_state *x, struct fsm_state *y)
{
	assert(cc != NULL);
	assert(fsm != NULL);
	assert(x != NULL);
	assert(y != NULL);

	if (!addedge_literal(fsm, cc->re_flags, x, y, c)) {
		return 0;
	}

	return 1;
}

static int
cc_add_range(struct cc *cc, struct fsm *fsm,
	const struct ast_endpoint *from,
	const struct ast_endpoint *to,
	struct fsm_state *x, struct fsm_state *y)
{
	unsigned char lower, upper;
	unsigned int i;

	assert(fsm != NULL);
	assert(x != NULL);
	assert(y != NULL);

	if (from->type != AST_ENDPOINT_LITERAL || to->type != AST_ENDPOINT_LITERAL) {
		/* not yet supported */
		return 0;
	}

	lower = from->u.literal.c;
	upper = to->u.literal.c;

	assert(cc != NULL);
	assert(lower <= upper);

	for (i = lower; i <= upper; i++) {
		if (!cc_add_char(cc, fsm, i, x, y)) {
			return 0;
		}
	}

	return 1;
}

static int
comp_iter(struct cc *cc, struct fsm *fsm,
	const struct ast_class *n,
	struct fsm_state *x, struct fsm_state *y)
{
	assert(cc != NULL);
	assert(fsm != NULL);
	assert(n != NULL);
	assert(x != NULL);
	assert(y != NULL);

	switch (n->type) {
	case AST_CLASS_LITERAL:
		if (!cc_add_char(cc, fsm, n->u.literal.c, x, y)) {
			return 0;
		}
		break;

	case AST_CLASS_RANGE:
		if (!cc_add_range(cc, fsm, &n->u.range.from, &n->u.range.to, x, y)) {
			return 0;
		}
		break;

	case AST_CLASS_NAMED:
		if (!cc_add_named(cc, fsm, n->u.named.ctor, x, y)) {
			return 0;
		}
		break;

	default:
		assert(!"unreached");
	}

	return 1;
}

int
ast_compile_class(struct ast_class **n, size_t count,
	struct fsm *fsm, enum re_flags re_flags,
	struct re_err *err,
	struct fsm_state *x, struct fsm_state *y)
{
	struct cc cc;
	size_t i;

	assert(n != NULL);
	assert(count > 0); /* due to AST simplification */
	assert(fsm != NULL);
	assert(x != NULL);
	assert(y != NULL);

	memset(&cc, 0x00, sizeof(cc));
	
	cc.err = err;
	cc.re_flags = re_flags;

	for (i = 0; i < count; i++) {
		if (!comp_iter(&cc, fsm, n[i], x, y)) {
			goto error;
		}
	}

	return 1;

error:

	if (err != NULL) {
		assert(err->e != RE_ESUCCESS);
	}

	return 0;
}

