/* $Id$ */

#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "libfsm/internal.h"
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
		{ '\\', "\\\\" },
		{ '\"', "\\\"" },
		{ '/',  "\\/"  },

		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  }
	};

	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\u%04x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

/* XXX: horrible */
static int
hasmore(const struct fsm_state *s, int i)
{
	struct fsm_edge *e, search;
	struct set_iter it;

	assert(s != NULL);

	i++;
	search.symbol = i;
	for (e = set_firstafter(s->edges, &it, &search); e != NULL; e = set_next(&it)) {
		if (!set_empty(e->sl)) {
			return 1;
		}
	}

	return 0;
}

void
fsm_out_json(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	fprintf(f, "{\n");

	{
		fprintf(f, "\t\"states\": [\n");

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_edge *e;
			struct set_iter it;

			fprintf(f, "\t\t{\n");

			fprintf(f, "\t\t\t\"end\": %s,\n",
				fsm_isend(fsm, s) ? "true" : "false");

			fprintf(f, "\t\t\t\"edges\": [\n");

			for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
				struct fsm_state *st;
				struct set_iter jt;

				for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
					assert(st != NULL);

					fprintf(f, "\t\t\t\t{ ");

					fprintf(f, "\"char\": ");
					switch (e->symbol) {
					case FSM_EDGE_EPSILON:
						fputs(" false", f);
						break;

					default:
						fputs(" \"", f);
						escputc(e->symbol, f);
						putc('\"', f);
						break;
					}

					fprintf(f, ", ");

					fprintf(f, "\"to\": %u",
						indexof(fsm, st));

					fprintf(f, "}%s\n", set_hasnext(&it) && hasmore(s, e->symbol) ? "," : "");
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

