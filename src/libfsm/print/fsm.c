/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>

/* TODO: centralise */
static const struct fsm_state *
findany(const struct fsm_state *state)
{
	struct fsm_state *f, *s;
	struct fsm_edge *e;
	struct edge_iter it;
	struct state_iter jt;
	struct bm bm;

	assert(state != NULL);

	/*
	 * This approach is a little unsatisfying because it will only identify
	 * situations with one single "any" edge between states. I'd much prefer
	 * to be able to emit "any" edges for situations like:
	 *
	 *  1 -> 2 'x';
	 *  1 -> 3;
	 *
	 * where a given state also has an unrelated edge transitioning elsewhere.
	 * The current implementation conservatively bails out on that situation
	 * (because state_set_only() is false), and will emit each edge separately.
	 */

	bm_clear(&bm);

	e = edge_set_first(state->edges, &it);
	if (e == NULL) {
		return NULL;
	}

	/* if the first edge is not the first character,
	 * then we can't possibly have an "any" transition */
	if (e->symbol != '\0') {
		return NULL;
	}

	f = state_set_first(e->sl, &jt);
	if (f == NULL) {
		return NULL;
	}

	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		if (state_set_count(e->sl) != 1) {
			return NULL;
		}

		s = state_set_only(e->sl);
		if (f != s) {
			return NULL;
		}

		bm_set(&bm, e->symbol);
	}

	if (bm_count(&bm) != FSM_SIGMA_COUNT) {
		return NULL;
	}

	assert(f != NULL);

	return f;
}

void
fsm_print_fsm(FILE *f, const struct fsm *fsm)
{
	struct fsm_state *s, *start;
	int end;

	assert(f != NULL);
	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_edge *e;
		struct edge_iter it;

		{
			struct fsm_state *st;
			struct state_iter jt;

			for (st = state_set_first(s->epsilons, &jt); st != NULL; st = state_set_next(&jt)) {
				assert(st != NULL);

				fprintf(f, "%-2u -> %2u;\n", indexof(fsm, s), indexof(fsm, st));
			}
		}

		{
			const struct fsm_state *a;

			a = findany(s);
			if (a != NULL) {
				fprintf(f, "%-2u -> %2u ?;\n", indexof(fsm, s), indexof(fsm, a));
				continue;
			}
		}

		for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
			struct fsm_state *st;
			struct state_iter jt;

			for (st = state_set_first(e->sl, &jt); st != NULL; st = state_set_next(&jt)) {
				assert(st != NULL);

				fprintf(f, "%-2u -> %2u", indexof(fsm, s), indexof(fsm, st));

				fputs(" \"", f);
				fsm_escputc(f, fsm->opt, e->symbol);
				putc('\"', f);

				fprintf(f, ";");

				if (fsm->opt->comments) {
					if (st == fsm->start) {
						fprintf(f, " # start");
					} else if (fsm->start != NULL && !fsm_has(fsm, fsm_hasepsilons)) {
						char buf[50];
						int n;

						n = fsm_example(fsm, st, buf, sizeof buf);
						if (-1 == n) {
							perror("fsm_example");
							return;
						}

						if (n > 0) {
							fprintf(f, " # e.g. \"");
							escputs(f, fsm->opt, fsm_escputc, buf);
							fprintf(f, "%s\"",
								n >= (int) sizeof buf - 1 ? "..." : "");
						}
					}
				}

				fprintf(f, "\n");
			}
		}
	}

	fprintf(f, "\n");

	start = fsm_getstart(fsm);
	if (start == NULL) {
		return;
	}

	fprintf(f, "start: %u;\n", indexof(fsm, start));

	end = 0;
	for (s = fsm->sl; s != NULL; s = s->next) {
		end += !!fsm_isend(fsm, s);
	}

	if (end == 0) {
		return;
	}

	fprintf(f, "end:   ");
	for (s = fsm->sl; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s)) {
			end--;

			fprintf(f, "%u%s", indexof(fsm, s), end > 0 ? ", " : ";\n");
		}
	}
}

