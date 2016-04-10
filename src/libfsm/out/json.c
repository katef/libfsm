/* $Id$ */

#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "out.h"
#include "libfsm/internal.h"

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

	return putc(c, f);
}

/* XXX: horrible */
static int
hasmore(const struct fsm_state *s, const struct state_set *e, int i)
{
	assert(s != NULL);

	if (e->next != NULL) {
		return 1;
	}

	i++;

	for ( ; i <= FSM_EDGE_MAX; i++) {
		if (s->edges[i].sl != NULL) {
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
	struct state_set *e;
	int end;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	fprintf(f, "{\n");

	{
		fprintf(f, "\t\"states\": [\n");

		for (s = fsm->sl; s != NULL; s = s->next) {
			int i;

			fprintf(f, "\t\t{\n");

			fprintf(f, "\t\t\t\"id\":  \"s%u\",\n",
				indexof(fsm, s));

			fprintf(f, "\t\t\t\"end\": %s,\n",
				fsm_isend(fsm, s) ? "true" : "false");

			fprintf(f, "\t\t\t\"edges\": [\n");

			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				for (e = s->edges[i].sl; e != NULL; e = e->next) {
					assert(e->state != NULL);

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

					fprintf(f, "\"to\": \"s%u\"",
						indexof(fsm, e->state));

					fprintf(f, "}%s\n", hasmore(s, e, i) ? "," : "");
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

		fprintf(f, "\t\"start\": \"s%u\"\n", indexof(fsm, start));
	}

	fprintf(f, "}\n");
}

