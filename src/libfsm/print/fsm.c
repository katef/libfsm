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

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

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

/* TODO: centralise */
static const struct fsm_state *
findany(const struct fsm_state *state)
{
	struct fsm_state *f, *s;
	struct fsm_edge *e;
	struct set_iter it;
	struct bm bm;

	assert(state != NULL);

	bm_clear(&bm);

	e = set_first(state->edges, &it);
	if (e == NULL) {
		return NULL;
	}

	/* if the first edge is not the first character,
	 * then we can't possibly have an "any" transition */
	if (e->symbol != '\0') {
		return NULL;
	}

	f = set_first(e->sl, &it);
	if (f == NULL) {
		return NULL;
	}

	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (e->symbol > UCHAR_MAX) {
			return NULL;
		}

		if (set_count(e->sl) != 1) {
			return NULL;
		}

		s = set_only(e->sl);
		if (f != s) {
			return NULL;
		}

		bm_set(&bm, e->symbol);
	}

	if (bm_count(&bm) != UCHAR_MAX + 1U) {
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
		struct set_iter it;

		{
			const struct fsm_state *a;

			a = findany(s);
			if (a != NULL) {
				fprintf(f, "%-2u -> %2u ?;\n", indexof(fsm, s), indexof(fsm, a));
				continue;
			}
		}

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			struct fsm_state *st;
			struct set_iter jt;

			for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
				assert(st != NULL);

				fprintf(f, "%-2u -> %2u", indexof(fsm, s), indexof(fsm, st));

				/* TODO: print " ?" if all edges are equal */

				switch (e->symbol) {
				case FSM_EDGE_EPSILON:
					break;

				default:
					fputs(" \"", f);
					fsm_escputc(f, fsm->opt, e->symbol);
					putc('\"', f);
					break;
				}

				fprintf(f, ";");

				if (fsm->opt->comments) {
					if (st == fsm->start) {
						fprintf(f, " # start");
					} else if (fsm->start != NULL) {
						char buf[50];
						int n;

						n = fsm_example(fsm, st, buf, sizeof buf);
						if (-1 == n) {
							perror("fsm_example");
							return;
						}

						fprintf(f, " # e.g. \"");
						escputs(f, fsm->opt, fsm_escputc, buf);
						fprintf(f, "%s\"",
							n >= (int) sizeof buf - 1 ? "..." : "");
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

