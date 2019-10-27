/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "class.h"
#include "ast.h"

static struct fsm *
new_blank(const struct fsm_options *opt)
{
	struct fsm *new;
	struct fsm_state *start;

	new = fsm_new(opt);
	if (new == NULL) {
		return NULL;
	}

	start = fsm_addstate(new);
	if (start == NULL) {
		goto error;
	}

	fsm_setstart(new, start);

return new;

	error:

	fsm_free(new);

	return NULL;
}

struct fsm *
ast_compile(const struct ast *ast,
	enum re_flags flags,
	const struct fsm_options *opt,
	struct re_err *err)
{
	struct fsm *fsm;
	struct fsm_state *x, *y;

	assert(ast != NULL);

	fsm = new_blank(opt);
	if (fsm == NULL) { return NULL; }

	x = fsm_getstart(fsm);
	assert(x != NULL);

	y = fsm_addstate(fsm);
	if (y == NULL) { goto error; }

	fsm_setend(fsm, y, 1);

	if (!ast_compile_expr(ast->expr, fsm, flags, err, x, y)) {
		goto error;
	}

	if (-1 == fsm_trim(fsm)) { goto error; }

	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (flags & RE_REVERSE) {
		if (!fsm_reverse(fsm)) { goto error; }
	}

	return fsm;

error:
	fsm_free(fsm);
	return NULL;
}

