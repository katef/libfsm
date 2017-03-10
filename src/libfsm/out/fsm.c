#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/out.h>
#include <fsm/options.h>

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
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

/* TODO: centralise, maybe with callback */
static int
escputs(FILE *f, const char *s)
{
	const char *p;
	int r, n;

	assert(f != NULL);
	assert(s != NULL);

	n = 0;

	for (p = s; *p != '\0'; p++) {
		r = escputc(*p, f);
		if (r == -1) {
			return -1;
		}

		n += r;
	}

	return n;
}

/* TODO: centralise */
static const struct fsm_state *
findany(const struct fsm_state *state)
{
	struct fsm_state *f, *s;
	struct fsm_edge *e;
	struct set_iter it;
	int esym;

	assert(state != NULL);

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

	esym = e->symbol;

	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (e->symbol > UCHAR_MAX) {
			break;
		}

		if (e->symbol != esym || set_empty(e->sl)) {
			return NULL;
		}

		s = set_only(e->sl);
		if (f != s) {
			return NULL;
		}
	}

	assert(f != NULL);

	return f;
}

void
fsm_out_fsm(const struct fsm *fsm, FILE *f)
{
	struct fsm_state *s, *start;
	int end;

	assert(fsm != NULL);
	assert(f != NULL);

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
					escputc(e->symbol, f);
					putc('\"', f);
					break;
				}

				fprintf(f, ";");

				if (fsm->opt->comments) {
					if (st == fsm->start) {
						fprintf(f, " # start");
					} else {
						char buf[50];
						int n;

						n = fsm_example(fsm, st, buf, sizeof buf);
						if (-1 == n) {
							perror("fsm_example");
							return;
						}

						fprintf(f, " # e.g. \"");
						escputs(f, buf);
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

