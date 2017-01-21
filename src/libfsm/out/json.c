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
	assert(s != NULL);

	i++;

	for ( ; i <= FSM_EDGE_MAX; i++) {
		if (!set_empty(s->edges[i].sl)) {
			return 1;
		}
	}

	return 0;
}

void
fsm_out_json(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	struct set_iter it;
	struct fsm_state *s, *e;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	fprintf(f, "{\n");

	{
		fprintf(f, "\t\"states\": [\n");

		for (s = fsm->sl; s != NULL; s = s->next) {
			int i;

			fprintf(f, "\t\t{\n");

			fprintf(f, "\t\t\t\"end\": %s,\n",
				fsm_isend(fsm, s) ? "true" : "false");

			fprintf(f, "\t\t\t\"edges\": [\n");

			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				for (e = set_first(s->edges[i].sl, &it); e != NULL; e = set_next(&it)) {
					assert(e != NULL);

					fprintf(f, "\t\t\t\t{ ");

					fprintf(f, "\"char\": ");
					switch (i) {
					case FSM_EDGE_EPSILON:
						fputs(" false", f);
						break;

					default:
						fputs(" \"", f);
						escputc(i, f);
						putc('\"', f);
						break;
					}

					fprintf(f, ", ");

					fprintf(f, "\"to\": %u",
						indexof(fsm, e));

					fprintf(f, "}%s\n", set_hasnext(&it) && hasmore(s, i) ? "," : "");
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

