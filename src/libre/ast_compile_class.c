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
comp_iter(struct fsm *fsm, enum re_flags re_flags,
	struct re_err *err,
	const struct ast_class *n,
	struct fsm_state *x, struct fsm_state *y)
{
	assert(fsm != NULL);
	assert(n != NULL);
	assert(x != NULL);
	assert(y != NULL);

	switch (n->type) {
	case AST_CLASS_LITERAL:
		if (!addedge_literal(fsm, re_flags, x, y, n->u.literal.c)) {
			return 0;
		}
		break;

	case AST_CLASS_RANGE: {
		unsigned int i;

		if (n->u.range.from.type != AST_ENDPOINT_LITERAL || n->u.range.to.type != AST_ENDPOINT_LITERAL) {
			/* not yet supported */
			return 0;
		}

		assert(n->u.range.from.u.literal.c <= n->u.range.to.u.literal.c);

		for (i = n->u.range.from.u.literal.c; i <= n->u.range.to.u.literal.c; i++) {
			if (!addedge_literal(fsm, re_flags, x, y, i)) {
				return 0;
			}
		}

		break;
	}

	case AST_CLASS_NAMED:
		if (!n->u.named.ctor(fsm, x, y)) {
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
	size_t i;

	assert(n != NULL);
	assert(count > 0); /* due to AST simplification */
	assert(fsm != NULL);
	assert(x != NULL);
	assert(y != NULL);

	for (i = 0; i < count; i++) {
		if (!comp_iter(fsm, re_flags, err, n[i], x, y)) {
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

