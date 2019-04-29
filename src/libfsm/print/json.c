/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include <print/esc.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "libfsm/internal.h"

/* XXX: horrible */
static int
hasmore(const struct fsm_state *s, int i)
{
	struct fsm_edge *e, search;
	struct edge_iter it;

	assert(s != NULL);

	i++;
	search.symbol = i;
	for (e = edge_set_firstafter(s->edges, &it, &search); e != NULL; e = edge_set_next(&it)) {
		if (!state_set_empty(e->sl)) {
			return 1;
		}
	}

	return 0;
}

void
fsm_print_json(FILE *f, const struct fsm *fsm)
{
	struct fsm_state *s;

	assert(f != NULL);
	assert(fsm != NULL);

	fprintf(f, "{\n");

	{
		fprintf(f, "\t\"states\": [\n");

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_edge *e;
			struct edge_iter it;

			fprintf(f, "\t\t{\n");

			fprintf(f, "\t\t\t\"end\": %s,\n",
				fsm_isend(fsm, s) ? "true" : "false");

			fprintf(f, "\t\t\t\"edges\": [\n");

			{
				struct fsm_state *st;
				struct state_iter jt;

				for (st = state_set_first(s->epsilons, &jt); st != NULL; st = state_set_next(&jt)) {
					assert(st != NULL);

					fprintf(f, "\t\t\t\t{ ");

					fprintf(f, "\"char\": ");
					fputs(" false", f);
					fprintf(f, ", ");

					fprintf(f, "\"to\": %u",
						indexof(fsm, st));

					/* XXX: should count .sl inside an edge */
					fprintf(f, "}%s\n", !edge_set_empty(s->edges) ? "," : "");
				}
			}

			for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
				struct fsm_state *st;
				struct state_iter jt;

				for (st = state_set_first(e->sl, &jt); st != NULL; st = state_set_next(&jt)) {
					assert(st != NULL);

					fprintf(f, "\t\t\t\t{ ");

					fprintf(f, "\"char\": ");
					fputs(" \"", f);
					json_escputc(f, fsm->opt, e->symbol);
					putc('\"', f);

					fprintf(f, ", ");

					fprintf(f, "\"to\": %u",
						indexof(fsm, st));

					fprintf(f, "}%s\n", edge_set_hasnext(&it) && hasmore(s, e->symbol) ? "," : "");
				}
			}

			fprintf(f, "\t\t\t]\n");

			fprintf(f, "\t\t}%s\n", s->next ? "," : "");
		}

		fprintf(f, "\t],\n");
	}

	{
		struct fsm_state *start;

		start = fsm_getstart(fsm);
		if (start == NULL) {
			return;
		}

		fprintf(f, "\t\"start\": %u\n", indexof(fsm, start));
	}

	fprintf(f, "}\n");
}

