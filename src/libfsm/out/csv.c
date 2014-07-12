/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <adt/set.h>
#include <adt/bitmap.h>

#include <fsm/fsm.h>

#include "out.h"

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

static int
escputc(int c, FILE *f)
{
	assert(f != NULL);

	switch (c) {
	case FSM_EDGE_EPSILON:
		fprintf(f, "\xCE\xB5"); /* epsilon, U03B5 UTF-8 */
		return 7;

	case '\"':
		fprintf(f, "\\\"");
		return 2;

	/* TODO: others */

	default:
		if (!isprint(c)) {
			return printf("0x%02X", (unsigned char) c);
		}

		putc(c, f);
		return 1;
	}
}

void
fsm_out_csv(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	static const struct bm bm_empty;
	struct bm bm;
	struct fsm_state *s;
	struct state_set *e;
	int i;
	int n;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	(void) options;


	bm = bm_empty;

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = s->edges[i].sl; e != NULL; e = e->next) {
				if (set_contains(s->edges[i].sl, e->state)) {
					bm_set(&bm, i);
				}
			}
		}
	}

	/* header */
	{
		fprintf(f, "\"\"");

		n = -1;

		while (n = bm_next(&bm, n, 1), n != FSM_EDGE_MAX + 1) {
			fprintf(f, ",  ");
			escputc(n, f);
		}

		fprintf(f, "\n");
	}

	/* body */
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "S%u", indexof(fsm, s));

		n = -1;

		while (n = bm_next(&bm, n, 1), n != FSM_EDGE_MAX + 1) {
			fprintf(f, ", ");

			if (s->edges[n].sl == NULL) {
				fprintf(f, "\"\"");
			} else {
				for (e = s->edges[n].sl; e != NULL; e = e->next) {
					fprintf(f, "S%u", indexof(fsm, s->edges[n].sl->state));

					if (e->next != NULL) {
						fprintf(f, " ");
					}
				}
			}
		}

		fprintf(f, "\n");
	}
}

