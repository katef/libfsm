/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <adt/set.h>
#include <adt/bitmap.h>

#include <fsm/fsm.h>

#include "libfsm/out.h"

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
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ FSM_EDGE_EPSILON, "\xCE\xB5" }, /* epsilon, U03B5 UTF-8 */

		{ '\"', "\\\"" }
		/* TODO: others */
	};

	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return printf("0x%02X", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

void
fsm_out_csv(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	static const struct bm bm_empty;
	struct fsm_state *s;
	struct bm bm;
	int n;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	(void) options;


	bm = bm_empty;

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
			escputc(n, f);
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

