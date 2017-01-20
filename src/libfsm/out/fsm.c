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

		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  },
		{ '\v', "\\v"  }
	};

	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\%03o", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

/* TODO: centralise */
static const struct fsm_state *
findany(const struct fsm_state *state)
{
	struct state_set *e, *f;
	struct set_iter iter;
	int i;

	assert(state != NULL);

	f = set_first(state->edges[0].sl, &i);
	
	for (i = 0; i <= UCHAR_MAX; i++) {
		if (set_empty(state->edges[i].sl)) {
			return NULL;
		}

		for (e = set_first(state->edges[i].sl, &iter); e != NULL; e = set_next(&iter)) {
			if (e->state != f->state) {
				return NULL;
			}
		}
	}

	assert(f != NULL);

	return f;
}

void
fsm_out_fsm(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options)
{
	struct set_iter iter;
	struct fsm_state *s;
	struct state_set *e;
	struct fsm_state *start;
	int end;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(options != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		int i;

		{
			const struct fsm_state *a;

			a = findany(s);
			if (a != NULL) {
				fprintf(f, "%-2u -> %2u ?;\n", indexof(fsm, s), indexof(fsm, a));
				continue;
			}
		}

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = set_first(s->edges[i].sl, &iter); e != NULL; e = set_next(&iter)) {
				assert(e->state != NULL);

				fprintf(f, "%-2u -> %2u", indexof(fsm, s), indexof(fsm, e->state));

				/* TODO: print " ?" if all edges are equal */

				switch (i) {
				case FSM_EDGE_EPSILON:
					break;

				default:
					fputs(" \"", f);
					escputc(i, f);
					putc('\"', f);
					break;
				}

				fprintf(f, ";\n");
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

