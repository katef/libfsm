#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

#include "../internal.h"

int
fsm_all(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	const struct fsm_state *s;

	assert(fsm != NULL);
	assert(predicate != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (!predicate(fsm, s)) {
			return 0;
		}
	}

	return 1;
}

