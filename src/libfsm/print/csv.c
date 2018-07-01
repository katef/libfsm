/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>

#include <fsm/fsm.h>
#include <fsm/print.h>

static unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

void
fsm_print_csv(FILE *f, const struct fsm *fsm)
{
	struct fsm_state *s;
	struct bm bm;
	int n;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	bm_clear(&bm);

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct set_iter it;
		struct fsm_edge *e;

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			bm_set(&bm, e->symbol);
		}
	}

	/* header */
	{
		fprintf(f, "\"\"");

		n = -1;

		while (n = bm_next(&bm, n, 1), n != FSM_EDGE_MAX + 1) {
			fprintf(f, ",  ");
			csv_escputc(f, fsm->opt, n);
		}

		fprintf(f, "\n");
	}

	/* body */
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "S%u", indexof(fsm, s));

		n = -1;

		while (n = bm_next(&bm, n, 1), n != FSM_EDGE_MAX + 1) {
			struct fsm_edge *e, search;

			fprintf(f, ", ");

			search.symbol = n;
			e = set_contains(s->edges, &search);
			if (set_empty(e->sl)) {
				fprintf(f, "\"\"");
			} else {
				struct fsm_state *se;
				struct set_iter it;

				for (se = set_first(e->sl, &it); se != NULL; se = set_next(&it)) {
					/* XXX: Used to always print set_first equivalent? */
					fprintf(f, "S%u", indexof(fsm, se));

					if (set_hasnext(&it)) {
						fprintf(f, " ");
					}
				}
			}
		}

		fprintf(f, "\n");
	}
}

