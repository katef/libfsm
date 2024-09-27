/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "libfsm/internal.h" /* XXX: up here for bitmap.h */
#include "libfsm/print.h"

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	fsm_state_t s;

	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque != NULL);

	(void) hook_opaque;

	s = * (fsm_state_t *) lang_opaque;

	fprintf(f, "label = <");

	if (!opt->anonymous_states) {
		fprintf(f, "%u", s);

		if (state_metadata->end_id_count > 0) {
			fprintf(f, "<BR/>");
		}
	}

	for (size_t i = 0; i < state_metadata->end_id_count; i++) {
		fprintf(f, "#%u", state_metadata->end_ids[i]);

		if (i < state_metadata->end_id_count - 1) {
			fprintf(f, ",");
		}
	}

	fprintf(f, ">");

	return 0;
}

static int
default_reject(FILE *f, const struct fsm_options *opt,
	void *lang_opaque, void *hook_opaque)
{
	fsm_state_t s;

	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque != NULL);

	(void) hook_opaque;

	s = * (fsm_state_t *) lang_opaque;

	if (!opt->anonymous_states) {
		fprintf(f, "label = <%u>", s);
	}

	return 0;
}

static int
print_state(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm *fsm,
	const char *prefix, fsm_state_t s)
{
	struct fsm_edge e;
	struct edge_iter it;
	struct edge_ordered_iter eoi;
	struct state_set *unique;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(fsm != NULL);
	assert(prefix != NULL);
	assert(s < fsm->statecount);

	if (!opt->consolidate_edges) {
		{
			struct state_iter jt;
			fsm_state_t st;

			for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
				fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
					prefix, s,
					prefix, st);

				fprintf(f, "&#x3B5;");

				fprintf(f, "> ];\n");
			}
		}

		for (edge_set_ordered_iter_reset(fsm->states[s].edges, &eoi); edge_set_ordered_iter_next(&eoi, &e); ) {
			fprintf(f, "\t%sS%-2u -> %sS%-2u [ label = <",
				prefix, s,
				prefix, e.state);

			dot_escputc_html(f, opt, (char) e.symbol);

			fprintf(f, "> ];\n");
		}

		return 0;
	}

	unique = NULL;

	for (edge_set_reset(fsm->states[s].edges, &it); edge_set_next(&it, &e); ) {
		if (!state_set_add(&unique, fsm->alloc, e.state)) {
			return -1;
		}
	}

	/*
	 * The consolidate_edges option is an aesthetic optimisation.
	 * For a state which has multiple edges all transitioning to the same state,
	 * all these edges are combined into a single edge, labelled with a more
	 * concise form of all their values.
	 *
	 * To implement this, we loop through all unique states, rather than
	 * looping through each edge.
	 */
	for (edge_set_ordered_iter_reset(fsm->states[s].edges, &eoi); edge_set_ordered_iter_next(&eoi, &e); ) {
		struct fsm_edge ne;
		struct edge_iter kt;
		struct bm bm;

		/* unique states only */
		if (!state_set_contains(unique, e.state)) {
			continue;
		}

		state_set_remove(&unique, e.state);

		bm_clear(&bm);

		/* find all edges which go from this state to the same target state */
		for (edge_set_reset(fsm->states[s].edges, &kt); edge_set_next(&kt, &ne); ) {
			if (ne.state == e.state) {
				bm_set(&bm, ne.symbol);
			}
		}

		fprintf(f, "\t%sS%-2u -> %sS%-2u [ ",
			prefix, s,
			prefix, e.state);

		if (bm_count(&bm) > 4) {
			fprintf(f, "weight = 3, ");
		}

		fprintf(f, "label = <");
		bm_print(f, opt, &bm, 0, dot_escputc_html);
		fprintf(f, "> ];\n");
	}

	/*
	 * Special edges are not consolidated above
	 */
	{
		struct state_iter jt;
		fsm_state_t st;

		for (state_set_reset(fsm->states[s].epsilons, &jt); state_set_next(&jt, &st); ) {
			fprintf(f, "\t%sS%-2u -> %sS%-2u [ weight = 1, label = <",
				prefix, s,
				prefix, st);

			fprintf(f, "&#x3B5;");

			fprintf(f, "> ];\n");
		}
	}

	if (unique != NULL) {
		state_set_free(unique);
	}

	return 0;
}

static int
print_dotfrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm *fsm, const char *prefix)
{
	fsm_state_t s;

	assert(f != NULL);
	assert(opt != NULL);
	assert(fsm != NULL);
	assert(prefix != NULL);

	for (s = 0; s < fsm->statecount; s++) {
		if (!fsm_isend(fsm, s)) {
			if (!opt->anonymous_states) {
				fprintf(f, "\t%sS%-2u [ ", prefix, s);

				if (-1 == print_hook_reject(f, opt, hooks, default_reject, &s)) {
					return -1;
				}

				fprintf(f, " ];\n");
			}
		} else {
			fsm_end_id_t *ids;
			size_t count;

			count = fsm_endid_count(fsm, s);
			if (count == 0) {
				ids = NULL;
			} else {
				int res;

				ids = f_malloc(fsm->alloc, count * sizeof *ids);
				if (ids == NULL) {
					return -1;
				}

				res = fsm_endid_get(fsm, s, count, ids);
				assert(res == 1);
			}

			fprintf(f, "\t%sS%-2u [ shape = doublecircle",
				prefix, s);

			assert(f != NULL);

			fprintf(f, ", ");

			const struct fsm_state_metadata state_metadata = {
				.end_ids = ids,
				.end_id_count = count,
			};

			if (-1 == print_hook_accept(f, opt, hooks,
				&state_metadata,
				default_accept, &s))
			{
				return -1;
			}

			if (opt->comments && hooks->comment != NULL) {
				fprintf(f, ",");

				if (-1 == print_hook_comment(f, opt, hooks,
					&state_metadata))
				{
					return -1;
				}
			}

			fprintf(f, " ];\n");

			f_free(fsm->alloc, ids);
		}

		/* TODO: comment example per state */

		if (-1 == print_state(f, opt, hooks, fsm, prefix, s)) {
			return -1;
		}
	}

	return 0;
}

int
fsm_print_dot(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm *fsm)
{
	const char *prefix;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "";
	}

	if (opt->fragment) {
		if (-1 == print_dotfrag(f, opt, hooks, fsm, prefix)) {
			return -1;
		}
	} else {
		fsm_state_t start;

		fprintf(f, "digraph G {\n");
		fprintf(f, "\trankdir = LR;\n");

		fprintf(f, "\tnode [ shape = circle ];\n");
		fprintf(f, "\tedge [ weight = 2 ];\n");

		if (opt->anonymous_states) {
			fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
		}

		fprintf(f, "\troot = start;\n");
		fprintf(f, "\n");

		fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

		if (fsm_getstart(fsm, &start)) {
			fprintf(f, "\tstart -> %sS%u;\n",
				opt->prefix != NULL ? opt->prefix : "",
				start);

			fprintf(f, "\n");
		}

		if (-1 == print_dotfrag(f, opt, hooks, fsm, prefix)) {
			return -1;
		}

		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	return 0;
}

