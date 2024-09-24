/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include <print/esc.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "libfsm/internal.h"
#include "libfsm/print.h"

#include "ir.h"

static unsigned int
ir_indexof(const struct ir *ir, const struct ir_state *cs)
{
	assert(ir != NULL);
	assert(cs != NULL);

	return cs - &ir->states[0];
}

static const char *
strategy_name(enum ir_strategy strategy)
{
	switch (strategy) {
	case IR_NONE:     return "NONE";
	case IR_SAME:     return "SAME";
	case IR_COMPLETE: return "COMPLETE";
	case IR_PARTIAL:  return "PARTIAL";
	case IR_DOMINANT: return "DOMINANT";
	case IR_ERROR:    return "ERROR";
	case IR_TABLE:    return "TABLE";

	default:
		return "?";
	}
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, unsigned char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	dot_escputc_html(f, opt, c);
}

static void
print_name(FILE *f, unsigned to, unsigned self)
{
	assert(f != NULL);

	if (to == self) {
		fprintf(f, "(self)");
	} else {
		fprintf(f, "S%u", to);
	}
}

static void
print_errorrows(FILE *f, const struct fsm_options *opt,
	const struct ir_error *error)
{
	size_t k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(error != NULL);
	assert(error->ranges != NULL);

	for (k = 0; k < error->n; k++) {
		fprintf(f, "\t\t  <TR>");

		if (error->ranges[k].start == error->ranges[k].end) {
			fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>");
			print_endpoint(f, opt, error->ranges[k].start);
			fprintf(f, "</TD>");
		} else {
			fprintf(f, "<TD ALIGN='LEFT'>");
			print_endpoint(f, opt, error->ranges[k].start);
			fprintf(f, "</TD>");
			fprintf(f, "<TD ALIGN='LEFT'>");
			print_endpoint(f, opt, error->ranges[k].end);
			fprintf(f, "</TD>");
		}

		if (k + 1 < error->n) {
			fprintf(f, "<TD ALIGN='LEFT'>&#x21B4;</TD>");
		} else {
			fprintf(f, "<TD ALIGN='LEFT'>(error)</TD>");
		}

		fprintf(f, "</TR>\n");
	}
}

static void
print_grouprows(FILE *f, const struct fsm_options *opt,
	unsigned self,
	const struct ir_group *groups, size_t n)
{
	size_t j, k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(groups != NULL);

	for (j = 0; j < n; j++) {
		assert(groups[j].ranges != NULL);

		for (k = 0; k < groups[j].n; k++) {
			fprintf(f, "\t\t  <TR>");

			if (groups[j].ranges[k].start == groups[j].ranges[k].end) {
				fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>");
				print_endpoint(f, opt, groups[j].ranges[k].start);
				fprintf(f, "</TD>");
			} else {
				fprintf(f, "<TD ALIGN='LEFT'>");
				print_endpoint(f, opt, groups[j].ranges[k].start);
				fprintf(f, "</TD>");
				fprintf(f, "<TD ALIGN='LEFT'>");
				print_endpoint(f, opt, groups[j].ranges[k].end);
				fprintf(f, "</TD>");
			}

			if (k + 1 < groups[j].n) {
				fprintf(f, "<TD ALIGN='LEFT'>&#x21B4;</TD>");
			} else {
				fprintf(f, "<TD ALIGN='LEFT' PORT='group%u'>",
					(unsigned) j);
				print_name(f, groups[j].to, self);
				fprintf(f, "</TD>");
			}

			fprintf(f, "</TR>\n");
		}
	}
}

static void
print_grouplinks(FILE *f, unsigned self,
	const struct ir_group *groups, size_t n)
{
	unsigned j;

	assert(f != NULL);
	assert(groups != NULL);

	for (j = 0; j < n; j++) {
		if (groups[j].to == self) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:group%u -> cs%u;\n",
				self, j,
				groups[j].to);
		}
	}
}

static int
print_state(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir *ir, const struct ir_state *cs)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(cs != NULL);

	if (cs->isend) {
		fprintf(f, "\tcs%u [ peripheries = 2 ];\n", ir_indexof(ir, cs));
	}

	fprintf(f, "\tcs%u [ label =\n", ir_indexof(ir, cs));
	fprintf(f, "\t\t<<TABLE BORDER='0' CELLPADDING='2' CELLSPACING='0'>\n");

	fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>S%u</TD><TD ALIGN='LEFT'>%s</TD></TR>\n",
		ir_indexof(ir, cs), strategy_name(cs->strategy));

	if (cs->isend && cs->endids.count > 0) {
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>end id</TD><TD ALIGN='LEFT'>");

		for (size_t i = 0; i < cs->endids.count; i++) {
			fprintf(f, "#%u", cs->endids.ids[i]);

			if (i < (size_t) cs->endids.count - 1) {
				fprintf(f, " ");
			}
		}

		fprintf(f, "</TD></TR>\n");
	}

	if (cs->example != NULL) {
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>example</TD><TD ALIGN='LEFT'>");
		escputs(f, opt, dot_escputc_html, cs->example);
		fprintf(f, "</TD></TR>\n");
	}

	/* showing hook in addition to existing content */
	if (cs->isend && hooks->accept != NULL) {
		fprintf(f, "\t\t  <TR><TD COLSPAN='3' ALIGN='LEFT'>");

		const struct fsm_state_metadata state_metadata = {
			.end_ids = cs->endids.ids,
			.end_id_count = cs->endids.count,
		};

		if (-1 == print_hook_accept(f, opt, hooks,
			&state_metadata,
			NULL,
			NULL))
		{
			return -1;
		}

		fprintf(f, "</TD></TR>\n");
	}

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>to</TD><TD ALIGN='LEFT'>");
		print_name(f, cs->u.same.to, ir_indexof(ir, cs));
		fprintf(f, "</TD></TR>\n");
		break;

	case IR_COMPLETE:
		print_grouprows(f, opt, ir_indexof(ir, cs), cs->u.complete.groups, cs->u.complete.n);
		break;

	case IR_PARTIAL:
		print_grouprows(f, opt, ir_indexof(ir, cs), cs->u.partial.groups, cs->u.partial.n);
		break;

	case IR_DOMINANT:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>mode</TD><TD ALIGN='LEFT' PORT='mode'>");
		print_name(f, cs->u.dominant.mode, ir_indexof(ir, cs));
		fprintf(f, "</TD></TR>\n");
		print_grouprows(f, opt, ir_indexof(ir, cs), cs->u.dominant.groups, cs->u.dominant.n);
		break;

	case IR_ERROR:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>mode</TD><TD ALIGN='LEFT' PORT='mode'>");
		print_name(f, cs->u.error.mode, ir_indexof(ir, cs));
		fprintf(f, "</TD></TR>\n");
		print_errorrows(f, opt, &cs->u.error.error);
		print_grouprows(f, opt, ir_indexof(ir, cs), cs->u.error.groups, cs->u.error.n);
		break;

	case IR_TABLE:
		assert(!"unreached");
		break;

	default:
		;
	}

	fprintf(f, "\t\t</TABLE>> ];\n");

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		if (cs->u.same.to == ir_indexof(ir, cs)) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u -> cs%u;\n",
				ir_indexof(ir, cs), cs->u.same.to);
		}
		break;

	case IR_COMPLETE:
		print_grouplinks(f, ir_indexof(ir, cs), cs->u.complete.groups, cs->u.complete.n);
		break;

	case IR_PARTIAL:
		print_grouplinks(f, ir_indexof(ir, cs), cs->u.partial.groups, cs->u.partial.n);
		break;

	case IR_DOMINANT:
		if (cs->u.dominant.mode == ir_indexof(ir, cs)) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:mode -> cs%u;\n",
				ir_indexof(ir, cs), cs->u.dominant.mode);
		}
		print_grouplinks(f, ir_indexof(ir, cs), cs->u.dominant.groups, cs->u.dominant.n);
		break;

	case IR_ERROR:
		if (cs->u.error.mode == ir_indexof(ir, cs)) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:mode -> cs%u;\n",
				ir_indexof(ir, cs), cs->u.error.mode);
		}
		print_grouplinks(f, ir_indexof(ir, cs), cs->u.error.groups, cs->u.error.n);
		break;

	case IR_TABLE:
		assert(!"unreached");
		break;

	default:
		;
	}

	return 0;
}

int
fsm_print_ir(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir *ir)
{
	size_t i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(ir != NULL);

	fprintf(f, "# generated\n");
	fprintf(f, "digraph G {\n");

	fprintf(f, "\tnode [ shape = box, style = rounded ];\n");
	fprintf(f, "\trankdir = LR;\n");
	fprintf(f, "\troot = start;\n");
	fprintf(f, "\n");
	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	if (ir->n > 0) {
		fprintf(f, "\tstart -> cs%u;\n", ir->start);
		fprintf(f, "\n");
	}

	for (i = 0; i < ir->n; i++) {
		if (-1 == print_state(f, opt, hooks, ir, &ir->states[i])) {
			return -1;
		}
	}

	fprintf(f, "}\n");

	return 0;
}

