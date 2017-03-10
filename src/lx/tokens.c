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
	const struct fsm_state *p;

	assert(fsm != NULL);
	assert(s != NULL);

	for (p = fsm->sl; p != NULL; p = p->next) {
		const struct ast_mapping *m;

		if (!fsm_isend(fsm, p)) {
			continue;
		}

		assert(p->opaque != NULL);
		m = p->opaque;

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
	const struct fsm_state *p;

	assert(a != NULL);
	assert(b != NULL);

	for (p = a->sl; p != NULL; p = p->next) {
		const struct ast_mapping *m;

		if (!fsm_isend(a, p)) {
			continue;
		}

		assert(p->opaque != NULL);
		m = p->opaque;

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

