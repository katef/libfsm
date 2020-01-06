/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include <print/esc.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "libfsm/internal.h"

void
fsm_print_json(FILE *f, const struct fsm *fsm)
{
	fsm_state_t i;

	assert(f != NULL);
	assert(fsm != NULL);

	fprintf(f, "{\n");

	{
		fprintf(f, "\t\"states\": [\n");

		for (i = 0; i < fsm->statecount; i++) {
			struct fsm_edge e;
			struct edge_iter it;
			int first = 1;

			fprintf(f, "\t\t{\n");

			fprintf(f, "\t\t\t\"end\": %s,\n",
				fsm_isend(fsm, i) ? "true" : "false");

			fprintf(f, "\t\t\t\"edges\": [\n");

			{
				struct state_iter jt;
				fsm_state_t st;

				for (state_set_reset(fsm->states[i].epsilons, &jt); state_set_next(&jt, &st); ) {
					if (!first) {
						fprintf(f, ",\n");
					}

					fprintf(f, "\t\t\t\t{ ");

					fprintf(f, "\"char\": ");
					fputs(" false", f);
					fprintf(f, ", ");

					fprintf(f, "\"to\": %u", st);

					/* XXX: should count .sl inside an edge */
					fprintf(f, " }");

					first = 0;
				}
			}

			for (edge_set_reset(fsm->states[i].edges, &it); edge_set_next(&it, &e); ) {
				if (!first) {
					fprintf(f, ",\n");
				}

				fprintf(f, "\t\t\t\t{ ");

				fprintf(f, "\"char\": ");
				fputs(" \"", f);
				json_escputc(f, fsm->opt, e.symbol);
				putc('\"', f);

				fprintf(f, ", ");

				fprintf(f, "\"to\": %u", e.state);

				fprintf(f, " }");

				first = 0;
			}

			if (!first) {
				fprintf(f, "\n");
			}

			fprintf(f, "\t\t\t]\n");

			fprintf(f, "\t\t}%s\n", (i + 1) < fsm->statecount ? "," : "");
		}

		fprintf(f, "\t],\n");
	}

	{
		fsm_state_t start;

		if (!fsm_getstart(fsm, &start)) {
			return;
		}

		fprintf(f, "\t\"start\": %u\n", start);
	}

	fprintf(f, "}\n");
}

