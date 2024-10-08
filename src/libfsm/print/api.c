/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */

#include <print/esc.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>

static int
rangeclass(unsigned char x, unsigned char y)
{
	int (*a[])(int c) = { isupper, islower, isdigit };
	size_t i;

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i](x) && a[i](y)) {
			return 1;
		}
	}

	return 0;
}

int
fsm_print_api(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm *fsm_orig)
{
	struct fsm *fsm;
	fsm_state_t start, end;
	struct bm *a; /* indexed by "to" state number */
	unsigned int from;

	assert(f != NULL);
	assert(fsm_orig != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (!fsm_has(fsm_orig, fsm_isend)) {
		errno = EINVAL;
		return -1;
	}

	fsm = fsm_clone(fsm_orig);
	if (fsm == NULL) {
		return -1;
	}

	if (!fsm_getstart(fsm, &start)) {
		errno = EINVAL;
		goto error;
	}

	if (!fsm_collate(fsm, &end, fsm_isend)) {
		goto error;
	}

/* TODO: leaf callback for opaques */

	if (opt->fragment) {
		fprintf(f, "\t");
	}
	fprintf(f, "/* Generated by libfsm */\n");
	fprintf(f, "\n");

	if (!opt->fragment) {
		fprintf(f, "#include LF_HEADER\n");
		fprintf(f, "\n");

		fprintf(f, "#include <assert.h>\n");
		fprintf(f, "#include <stddef.h>\n");
		fprintf(f, "\n");

		fprintf(f, "#include <fsm/fsm.h>\n");
		fprintf(f, "#include <fsm/pred.h>\n");
		fprintf(f, "\n");

		fprintf(f, "int\n");
		fprintf(f, "%sfsm(struct fsm *fsm, fsm_state_t *x, fsm_state_t *y)\n",
			opt->prefix != NULL ? opt->prefix : "");

		fprintf(f, "{\n");
	}

	fprintf(f, "\tfsm_state_t s[%zu];\n", fsm->statecount);
	fprintf(f, "\tsize_t i;\n");
	fprintf(f, "\n");

	fprintf(f, "\tassert(x != NULL);\n");
	fprintf(f, "\tassert(y != NULL);\n");
	fprintf(f, "\n");

	fprintf(f, "\tfor (i = 0; i < %zu; i++) {\n", fsm->statecount);

	fprintf(f, "\t\tif (i == %u) {\n", start);
	fprintf(f, "\t\t\ts[%u] = *x;\n", start);
	fprintf(f, "\t\t\tcontinue;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\n");

	fprintf(f, "\t\tif (i == %u) {\n", end);
	fprintf(f, "\t\t\ts[%u] = *y;\n", end);
	fprintf(f, "\t\t\tcontinue;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\n");

	fprintf(f, "\t\tif (!fsm_addstate(fsm, &s[i]) {\n");
	fprintf(f, "\t\t\treturn 0;\n");
	fprintf(f, "\t\t}\n");

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	/* endleaf() doesn't make sense for this format */

	{
		size_t count;

		count = fsm_endid_count(fsm, end);
		if (count > 0) {
			fsm_end_id_t *ids;
			int res;

			ids = f_malloc(fsm->alloc, count * sizeof *ids);
			if (ids == NULL) {
				/* XXX */
				goto error;
			}

			res = fsm_endid_get(fsm, end, count, ids);
			assert(res == 1);

			fprintf(f, "\tif (fsm_isend(fsm, s[%u])) {\n", end);
			for (size_t id = 0; id < count; id++) {
				fprintf(f, "\t\tif (!fsm_endid_set(fsm, s[%u], %zu)) { return 0; }\n", end, id);
			}
			fprintf(f, "\t}\n");
			fprintf(f, "\n");

			f_free(fsm->alloc, ids);
		}
	}

	a = f_malloc(fsm->alloc, fsm->statecount * sizeof *a);
	if (a == NULL) {
		/* XXX */
		goto error;
	}

	for (from = 0; from < fsm->statecount; from++) {
		struct fsm_edge e;
		struct edge_ordered_iter eoi;
		struct state_iter jt;
		fsm_state_t i, to;

		for (i = 0; i < fsm->statecount; i++) {
			bm_clear(&a[i]);
		}

		for (state_set_reset(fsm->states[from].epsilons, &jt); state_set_next(&jt, &to); ) {
			fprintf(f, "\tif (!fsm_addedge_epsilon(fsm, s[%u], s[%u])) { return 0; }\n",
				from, to);
		}

		for (edge_set_ordered_iter_reset(fsm->states[from].edges, &eoi); edge_set_ordered_iter_next(&eoi, &e); ) {
			bm_set(&a[e.state], e.symbol);
		}

		for (i = 0; i < fsm->statecount; i++) {
			int hi, lo;

			to = i;

			hi = -1;

			for (;;) {
				/* start of range */
				lo = bm_next(&a[to], hi, 1);
				if (lo > UCHAR_MAX) {
						break;
				}

				/* one past the end of range */
				hi = bm_next(&a[to], lo, 0);

				if (lo == 0x00 && hi == UCHAR_MAX + 1) {
					fprintf(f, "\tif (!fsm_addedge_any(fsm, s[%u], s[%u]))",
						from, to);
					fprintf(f, " { return 0; }\n");
				} else if (lo == hi - 1) {
					fprintf(f, "\tif (!fsm_addedge_literal(fsm, s[%u], s[%u], ",
						from, to);
					c_escputcharlit(f, opt, lo);
					fprintf(f, ")) { return 0; }\n");
				} else {
					fprintf(f, "\tfor (i = 0x%02x; i <= 0x%02x; i++) {",
						(unsigned int) lo, (unsigned int) hi - 1);
					if (opt->comments && rangeclass(lo, hi - 1)) {
						fprintf(f, " /* '%c' .. '%c' */", (unsigned char) lo, (unsigned char) hi - 1);
					}
					fprintf(f, "\n");
					fprintf(f, "\t\tif (!fsm_addedge_literal(fsm, s[%u], s[%u], i))",
						from, to);
					fprintf(f, " { return 0; }\n");
					fprintf(f, "\t}\n");
				}
			}
		}
	}

	f_free(fsm->alloc, a);

	fprintf(f, "\n");

	if (!opt->fragment) {
		fprintf(f, "\n");
		fprintf(f, "\treturn 1;\n");

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	fsm_free(fsm);

	return 0;

error:

	fsm_free(fsm);

	return -1;
}

