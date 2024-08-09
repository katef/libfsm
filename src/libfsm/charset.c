/*
 * Copyright 2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"
#include "walk2.h"

/* ^[abc..]*$ */
struct fsm *
fsm_intersect_charset(struct fsm *a, size_t n, const char *charset)
{
	struct fsm *b, *q;

	assert(a != NULL);

	if (charset == NULL) {
		return a;
	}

	/*
	 * Since intersection is destructive, there's no point in making
	 * the charset FSM in advance and then fsm_clone()ing it.
	 * We may as well just make a new one each time.
	 */
	{
		fsm_state_t state;

		b = fsm_new_statealloc(a->alloc, 1);
		if (b == NULL) {
			return NULL;
		}

		if (!fsm_addstate(b, &state)) {
			goto error;
		}

		for (size_t i = 0; i < n; i++) {
			if (!fsm_addedge_literal(b, state, state, charset[i])) {
				goto error;
			}
		}

		fsm_setend(b, state, true);
		fsm_setstart(b, state);
	}

	assert(fsm_all(a, fsm_isdfa));
	assert(fsm_all(b, fsm_isdfa));

	/*
	 * This is equivalent to fsm_intersect(). fsm_intersect() asserts that
	 * both operands are DFA at runtime. But we know this empirically from
	 * our callers. So we call fsm_walk2() directly to avoid needlessly
	 * running the DFA predicate in DNDEBUG builds.
	 *
	 * This is intersection implemented by walking sets of states through
	 * both FSM simultaneously, as described by Hopcroft, Motwani and Ullman
	 * (2001, 2nd ed.) 4.2, Closure under Intersection.
	 *
	 * The intersection of two FSM consists of only those items which
	 * are present in _BOTH.
	 */
	q = fsm_walk2(a, b, FSM_WALK2_BOTH, FSM_WALK2_BOTH);
	if (q == NULL) {
		return NULL;
	}

	fsm_free(a);
	fsm_free(b);

	/* walking two DFA produces a DFA */
	assert(fsm_all(q, fsm_isdfa));

	return q;

error:

	fsm_free(b);

	return NULL;
}

