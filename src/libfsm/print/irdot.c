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
print_state(FILE *f, unsigned to, unsigned self)
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
	const struct ir *ir,
	const struct ir_error *error)
{
	size_t k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
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
	const struct ir *ir, unsigned self,
	const struct ir_group *groups, size_t n)
{
	size_t j, k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
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
				print_state(f, groups[j].to, self);
				fprintf(f, "</TD>");
			}

			fprintf(f, "</TR>\n");
		}
	}
}

static void
print_grouplinks(FILE *f, const struct ir *ir, unsigned self,
	const struct ir_group *groups, size_t n)
{
	unsigned j;

	assert(f != NULL);
	assert(ir != NULL);
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

static void
print_cs(FILE *f, const struct fsm_options *opt,
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

	if (cs->example != NULL) {
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>example</TD><TD ALIGN='LEFT'>");
		escputs(f, opt, dot_escputc_html, cs->example);
		fprintf(f, "</TD></TR>\n");
	}

	/* TODO: leaf callback for dot output */

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>to</TD><TD ALIGN='LEFT'>");
		print_state(f, cs->u.same.to, ir_indexof(ir, cs));
		fprintf(f, "</TD></TR>\n");
		break;

	case IR_COMPLETE:
		print_grouprows(f, opt, ir, ir_indexof(ir, cs), cs->u.complete.groups, cs->u.complete.n);
		break;

	case IR_PARTIAL:
		print_grouprows(f, opt, ir, ir_indexof(ir, cs), cs->u.partial.groups, cs->u.partial.n);
		break;

	case IR_DOMINANT:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>mode</TD><TD ALIGN='LEFT' PORT='mode'>");
		print_state(f, cs->u.dominant.mode, ir_indexof(ir, cs));
		fprintf(f, "</TD></TR>\n");
		print_grouprows(f, opt, ir, ir_indexof(ir, cs), cs->u.dominant.groups, cs->u.dominant.n);
		break;

	case IR_ERROR:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>mode</TD><TD ALIGN='LEFT' PORT='mode'>");
		print_state(f, cs->u.error.mode, ir_indexof(ir, cs));
		fprintf(f, "</TD></TR>\n");
		print_errorrows(f, opt, ir, &cs->u.error.error);
		print_grouprows(f, opt, ir, ir_indexof(ir, cs), cs->u.error.groups, cs->u.error.n);
		break;

	case IR_TABLE:
		/* TODO */
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
		print_grouplinks(f, ir, ir_indexof(ir, cs), cs->u.complete.groups, cs->u.complete.n);
		break;

	case IR_PARTIAL:
		print_grouplinks(f, ir, ir_indexof(ir, cs), cs->u.partial.groups, cs->u.partial.n);
		break;

	case IR_DOMINANT:
		if (cs->u.dominant.mode == ir_indexof(ir, cs)) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:mode -> cs%u;\n",
				ir_indexof(ir, cs), cs->u.dominant.mode);
		}
		print_grouplinks(f, ir, ir_indexof(ir, cs), cs->u.dominant.groups, cs->u.dominant.n);
		break;

	case IR_ERROR:
		if (cs->u.error.mode == ir_indexof(ir, cs)) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:mode -> cs%u;\n",
				ir_indexof(ir, cs), cs->u.error.mode);
		}
		print_grouplinks(f, ir, ir_indexof(ir, cs), cs->u.error.groups, cs->u.error.n);
		break;

	case IR_TABLE:
		break;

	default:
		;
	}
}

void
fsm_print_ir(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	size_t i;

	assert(f != NULL);
	assert(fsm != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	fprintf(f, "digraph G {\n");

	fprintf(f, "\tnode [ shape = box, style = rounded ];\n");
	fprintf(f, "\trankdir = LR;\n");
	fprintf(f, "\troot = start;\n");
	fprintf(f, "\n");
	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	fprintf(f, "\tstart -> cs%u;\n", ir->start);
	fprintf(f, "\n");

	for (i = 0; i < ir->n; i++) {
		print_cs(f, fsm->opt, ir, &ir->states[i]);
	}

	fprintf(f, "}\n");

	free_ir(fsm, ir);
}

