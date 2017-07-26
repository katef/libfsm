/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/table.h>

#include "libfsm/out.h"

void
fsm_out_table(const struct fsm *fsm, FILE *f)
{
	struct fsm_table *tbl;
	size_t i;
	size_t start;

	assert(fsm != NULL);
	assert(f != NULL);

	tbl = fsm_table(fsm);
	start = tbl->start;
	if (start == (size_t)-1) {
		start = 0;
	}

        /* XXX - C89 has no format specifier for size_t, so cast to
         * unsigned long
         */
	fprintf(f, "%lu %lu %lu %lu\n",
		(unsigned long)tbl->nstates, (unsigned long)(start+1),
                (unsigned long)tbl->nend, (unsigned long)tbl->nedges);

	for (i=0; i < tbl->nend; i++) {
		fprintf(f, "%lu\n", (unsigned long)(tbl->endstates[i].state+1));
	}

	for (i=0; i < tbl->nedges; i++) {
		fprintf(f, "%lu %u %lu\n",
			(unsigned long)(tbl->edges[i].src+1),
                        /* lbl is unsigned int, not size_t, so no casting */
                        tbl->edges[i].lbl,
                        (unsigned long)(tbl->edges[i].dst+1));
	}

	fsm_table_free(tbl);
}

