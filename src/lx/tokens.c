/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "libfsm/internal.h" /* XXX */

#include "ast.h"
#include "tokens.h"

int
tok_contains(const struct fsm *fsm, const char *s)
{
	fsm_state_t i;

	assert(fsm != NULL);
	assert(s != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		const struct ast_mapping *m;

		if (!fsm_isend(fsm, i)) {
			continue;
		}

		m = fsm_getopaque(fsm, i);
		assert(m != NULL);

		if (m->token == NULL) {
			continue;
		}

		if (0 == strcmp(m->token->s, s)) {
			return 1;
		}
	}

	return 0;
}

int
tok_subsetof(const struct fsm *a, const struct fsm *b)
{
	fsm_state_t i;

	assert(a != NULL);
	assert(b != NULL);

	for (i = 0; i < a->statecount; i++) {
		const struct ast_mapping *m;

		if (!fsm_isend(a, i)) {
			continue;
		}

		m = fsm_getopaque(a, i);
		assert(m != NULL);

		if (m->token == NULL) {
			continue;
		}

		if (!tok_contains(b, m->token->s)) {
			return 0;
		}
	}

	return 1;
}

int
tok_equal(const struct fsm *a, const struct fsm *b)
{
	return tok_subsetof(a, b) && tok_subsetof(b, a);
}

