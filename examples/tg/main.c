/*
 * Copyright 2008-2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/out.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/options.h>

#include "libfsm/internal.h" /* XXX */

#include <re/re.h>

/* TODO: arbitrary size bitmap */
typedef unsigned bm_t;

struct pattern {
	unsigned i;
	const char *name;
	const char *re;
};

struct match {
	bm_t mask;
};

/*
 * Floor of the binary logarithm of i.
 * (i.e. the index of a bit)
 */
static size_t
ilog2(unsigned i)
{
	size_t k = 0;

	while (i >>= 1) {
		k++;
	}

	return k;
}

/* TODO: centralise with libfsm */
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

static void
endlabel_pattern(FILE *f, const struct fsm *fsm, const void *o,
	const struct pattern a[])
{
	const struct pattern *p = o;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(p != NULL);

	(void) a;

	fprintf(f, "%u:%s", p->i, p->name);
}

static void
endlabel_match(FILE *f, const struct fsm *fsm, const void *o,
	const struct pattern a[])
{
	const struct match *m = o;
	unsigned v;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(m != NULL);

	for (v = m->mask; v != 0; v &= v - 1) {
		fprintf(f, "%s", a[ilog2(v & ~(v - 1))].name);

		if ((v & (v - 1)) != 0) {
			fprintf(f, ", ");
		}
	}
}

static void
tg_print(FILE *f, const struct fsm *fsm, const struct pattern a[],
	void (*endlabel)(FILE *, const struct fsm *, const void *,
		const struct pattern []))
{
	const struct fsm_state *s;
	const struct fsm_options *opt;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(a != NULL);
	assert(endlabel != NULL);

	opt = fsm_getoptions(fsm);
	assert(opt != NULL);

	fprintf(f, "digraph G {\n");
	fprintf(f, "\trankdir = LR;\n");
	fprintf(f, "\tnode [ shape = circle ];\n");
	fprintf(f, "\tedge [ weight = 2 ];\n");
	fprintf(f, "\troot = start;\n");
	fprintf(f, "\n");

	fsm_print(fsm, stderr, FSM_OUT_DOT);

	fprintf(f, "\n");

	for (s = fsm->sl; s != NULL; s = s->next) {
		/* here we're overriding the labels FSM_OUT_DOT produced */

		fprintf(f, "\tS%u [ label = <",
			indexof(fsm, s));

		if (!opt->anonymous_states) {
			fprintf(f, "%u<br/>", indexof(fsm, s));
		}

		if (fsm_isend(fsm, s)) {
			const void *o;

			o = fsm_getopaque(fsm, s);
			assert(o != NULL);

			endlabel(f, fsm, o, a);
		}

		fprintf(f, "> ];\n");
	}

	fprintf(f, "}\n");
}

static void
carryopaque(const struct fsm_state **set, size_t n,
	struct fsm *fsm, struct fsm_state *state)
{
	struct match *m;
	size_t i;

	assert(set != NULL);
	assert(n > 0);
	assert(fsm != NULL);
	assert(state != NULL);

	if (!fsm_isend(fsm, state)) {
		return;
	}

	m = malloc(sizeof *m);
	if (m == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	m->mask = 0;

	for (i = 0; i < n; i++) {
		const struct pattern *p;

		if (!fsm_isend(fsm, set[i])) {
			continue;
		}

		p = fsm_getopaque(fsm, set[i]);
		assert(p != NULL);

		m->mask |= 1U << p->i;
	}

	fsm_setopaque(fsm, state, m);
}

int main(void) {
	struct fsm *fsm;
	size_t i;

	struct fsm_options opt = { 0 };

	opt.tidy = 0;

	/* TODO: input.
	 * e.g. from https://github.com/elastic/logstash/blob/v1.4.2/patterns/grok-patterns */
	/* ordered most specific first */
	struct pattern a[] = {
		{ 0, "oct",    "[0-7]+"     },
		{ 0, "hex",    "[0-9a-f]+"  },
		{ 0, "int",    "[0-9]+"     },
		{ 0, "string", ".+"         }
	};

	assert(sizeof a / sizeof *a <= sizeof ((struct match *) NULL)->mask);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		a[i].i = i;
	}

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		perror("fsm_new");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		struct re_err err;
		struct fsm *new;
		const char *s;

		s = a[i].re;

		new = re_comp(RE_NATIVE, fsm_sgetc, &s, &opt, 0, &err);
		if (new == NULL) {
			re_perror(RE_NATIVE, &err, NULL, a[i].re);
			exit(EXIT_FAILURE);
		}

		if (!fsm_minimise(new)) {
			perror("fsm_minimise");
			exit(EXIT_FAILURE);
		}

		fsm_setendopaque(new, &a[i]);

		fsm = fsm_union(fsm, new);
		if (fsm == NULL) {
			perror("fsm_union");
			exit(EXIT_FAILURE);
		}
	}

	{
		/* TODO: save/restore options */
		opt.anonymous_states = 0;
		opt.consolidate_edges = 1;
		opt.fragment = 1;

		tg_print(stderr, fsm, a, endlabel_pattern);
	}

	{
		opt.carryopaque = carryopaque;

		if (!fsm_determinise(fsm)) {
			perror("fsm_determinise");
			exit(EXIT_FAILURE);
		}

		opt.carryopaque = NULL;
	}

	{
		/* TODO: save/restore options */
		opt.anonymous_states = 0;
		opt.consolidate_edges = 1;
		opt.fragment = 1;

		tg_print(stderr, fsm, a, endlabel_match);
	}

	/* would be generated code below this point*/

	{
		char buf[BUFSIZ];
		bm_t mask;
		size_t v;

		mask = ~0U;

		while (fgets(buf, sizeof buf, stdin) != NULL) {
			struct fsm_state *state;
			const struct match *m;
			const char *s;

			/* TODO: handle overflow */

			buf[strcspn(buf, "\n")] = '\0';

			s = buf;

			state = fsm_exec(fsm, fsm_sgetc, &s);
			if (state == NULL) {
				printf("no match\n");
				exit(2);
			}

			m = fsm_getopaque(fsm, state);
			assert(m != NULL);

			/*
			 * TODO: could trim edges from the running FSM. consider how.
			 * keep mask of running reachable bits
			 */

			mask &= m->mask;

			if (mask == 0) {
				printf("no category\n");
				exit(3);
			}
		}

		for (v = mask; v != 0; v &= v - 1) {
			size_t i;

			i = ilog2(v & ~(v - 1));

			printf("%s: /%s/\n", a[i].name, a[i].re);
		}
	}

	/* TODO: free stuff */

	return 0;
}

