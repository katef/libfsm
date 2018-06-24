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

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/out.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "libfsm/internal.h"
#include "libfsm/out.h"

#include "ir.h"

static unsigned int
indexof(const struct ir *ir, const struct ir_state *cs)
{
	assert(ir != NULL);
	assert(cs != NULL);

	return cs - &ir->states[0];
}

const char *
strategy_name(enum ir_strategy strategy)
{
	switch (strategy) {
	case IR_NONE: return "NONE";
	case IR_SAME: return "SAME";
	case IR_MODE: return "MODE";
	case IR_MANY: return "MANY";
	case IR_JUMP: return "JUMP";

	default:
		return "?";
	}
}

/* TODO: centralise */
static int
escputc_hex(int c, FILE *f)
{
	assert(f != NULL);

	return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
}

/* TODO: centralise */
static int
escputc(int c, FILE *f)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '&',  "&amp;"    },
		{ '\"', "&quot;"   },
		{ ']',  "&#x5D;"   }, /* yes, even in a HTML label */
		{ '<',  "&#x3C;"   },
		{ '>',  "&#x3E;"   },
		{ ' ',  "&#x2423;" }
	};

	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
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

static void
print_endpoint(const struct fsm_options *opt, unsigned char c, FILE *f)
{
	assert(opt != NULL);
	assert(f != NULL);

	if (opt->always_hex) {
		escputc_hex(c, f);
	} else {
		escputc(c, f);
	}
}

static void
print_state(const struct ir *ir,
	const struct ir_state *to, const struct ir_state *self,
	FILE *f)
{
	assert(ir != NULL);
	assert(self != NULL);
	assert(f != NULL);

	if (to == NULL) {
		fprintf(f, "(none)");
	} else if (to == self) {
		fprintf(f, "(self)");
	} else {
		fprintf(f, "S%u", indexof(ir, to));
	}
}

static void
print_grouprows(const struct fsm_options *opt,
	const struct ir *ir, const struct ir_state *self,
	const struct ir_group *groups, size_t n, FILE *f)
{
	size_t j, k;

	assert(opt != NULL);
	assert(ir != NULL);
	assert(groups != NULL);
	assert(f != NULL);

	for (j = 0; j < n; j++) {
		assert(groups[j].ranges != NULL);

		for (k = 0; k < groups[j].n; k++) {
			fprintf(f, "\t\t  <TR>");

			if (groups[j].ranges[k].start == groups[j].ranges[k].end) {
				fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>");
				print_endpoint(opt, groups[j].ranges[k].start, f);
				fprintf(f, "</TD>");
			} else {
				fprintf(f, "<TD ALIGN='LEFT'>");
				print_endpoint(opt, groups[j].ranges[k].start, f);
				fprintf(f, "</TD>");
				fprintf(f, "<TD ALIGN='LEFT'>");
				print_endpoint(opt, groups[j].ranges[k].end, f);
				fprintf(f, "</TD>");
			}

			if (k + 1 < groups[j].n) {
				fprintf(f, "<TD ALIGN='LEFT'>&#x21B4;</TD>");
			} else {
				fprintf(f, "<TD ALIGN='LEFT' PORT='group%u'>",
					(unsigned) j);
				print_state(ir, groups[j].to, self, f);
				fprintf(f, "</TD>");
			}

			fprintf(f, "</TR>\n");
		}
	}
}

static void
print_grouplinks(const struct ir *ir, const struct ir_state *self,
	const struct ir_group *groups, size_t n, FILE *f)
{
	size_t j;

	assert(ir != NULL);
	assert(groups != NULL);
	assert(f != NULL);

	for (j = 0; j < n; j++) {
		if (groups[j].to == NULL) {
			fprintf(f, "\tcs%u:group%u -> cs%s;\n",
				indexof(ir, self), (unsigned) j,
				"(none)");
		} else if (groups[j].to == self) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:group%u -> cs%u;\n",
				indexof(ir, self), (unsigned) j,
				indexof(ir, groups[j].to));
		}
	}
}

static void
print_cs(const struct fsm_options *opt,
	const struct ir *ir, const struct ir_state *cs, FILE *f)
{
	assert(opt != NULL);
	assert(ir != NULL);
	assert(cs != NULL);
	assert(f != NULL);

	if (cs->isend) {
		fprintf(f, "\tcs%u [ peripheries = 2 ];\n", indexof(ir, cs));
	}

	fprintf(f, "\tcs%u [ label =\n", indexof(ir, cs));
	fprintf(f, "\t\t<<TABLE BORDER='0' CELLPADDING='2' CELLSPACING='0'>\n");
	fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>S%u</TD><TD ALIGN='LEFT'>%s</TD></TR>\n",
		indexof(ir, cs), strategy_name(cs->strategy));

	if (cs->example != NULL) {
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>example</TD><TD ALIGN='LEFT'>");
		escputs(f, cs->example);
		fprintf(f, "</TD></TR>\n");
	}

	/* TODO: leaf callback for dot output */

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>to</TD><TD ALIGN='LEFT'>");
		print_state(ir, cs->u.same.to, cs, f);
		fprintf(f, "</TD></TR>\n");
		break;

	case IR_MANY:
		print_grouprows(opt, ir, cs, cs->u.many.groups, cs->u.many.n, f);
		break;

	case IR_MODE:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>mode</TD><TD ALIGN='LEFT' PORT='mode'>");
		print_state(ir, cs->u.mode.mode, cs, f);
		fprintf(f, "</TD></TR>\n");
		print_grouprows(opt, ir, cs, cs->u.mode.groups, cs->u.mode.n, f);
		break;

	case IR_JUMP:
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
		if (cs->u.same.to == cs) {
			/* no edge drawn */
		} else if (cs->u.same.to != NULL) {
			fprintf(f, "\tcs%u -> cs%u;\n",
				indexof(ir, cs), indexof(ir, cs->u.same.to));
		}
		break;

	case IR_MANY:
		print_grouplinks(ir, cs, cs->u.many.groups, cs->u.many.n, f);
		break;

	case IR_MODE:
		if (cs->u.mode.mode == cs) {
			/* no edge drawn */
		} else if (cs->u.mode.mode != NULL) {
			fprintf(f, "\tcs%u:mode -> cs%u;\n",
				indexof(ir, cs), indexof(ir, cs->u.mode.mode));
		}
		print_grouplinks(ir, cs, cs->u.mode.groups, cs->u.mode.n, f);
		break;

	case IR_JUMP:
		break;

	default:
		;
	}
}

void
fsm_out_ir(const struct fsm *fsm, FILE *f)
{
	struct ir *ir;
	size_t i;

	assert(fsm != NULL);
	assert(f != NULL);

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

	for (i = 0; i < ir->n; i++) {
		if (&ir->states[i] == ir->start) {
			fprintf(f, "\tstart -> cs%u;\n", (unsigned) i);
			fprintf(f, "\n");
		}

		print_cs(fsm->opt, ir, &ir->states[i], f);
	}

	fprintf(f, "}\n");

	free_ir(ir);
}

