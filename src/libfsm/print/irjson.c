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

static const char *
strategy_name(enum ir_strategy strategy)
{
	switch (strategy) {
	case IR_NONE: return "none";
	case IR_SAME: return "same";
	case IR_MODE: return "mode";
	case IR_MANY: return "many";
	case IR_JUMP: return "jump";

	default:
		return "?";
	}
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, unsigned char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	fprintf(f, "\"");
	json_escputc(f, opt, c);
	fprintf(f, "\"");
}

static void
print_groups(FILE *f, const struct fsm_options *opt,
	const struct ir *ir,
	const struct ir_group *groups, size_t n)
{
	size_t j, k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(groups != NULL);

	fprintf(f, "[\n");

	for (j = 0; j < n; j++) {
		assert(groups[j].ranges != NULL);

		fprintf(f, "\t\t\t\t{\n");

		fprintf(f, "\t\t\t\t\t\"to\": %u,\n", groups[j].to);
		fprintf(f, "\t\t\t\t\t\"ranges\": [\n");

		for (k = 0; k < groups[j].n; k++) {
			fprintf(f, "\t\t\t\t\t\t{ ");

			fprintf(f, "\"start\": ");
			print_endpoint(f, opt, groups[j].ranges[k].start);
			fprintf(f, ", ");
			fprintf(f, "\"end\": ");
			print_endpoint(f, opt, groups[j].ranges[k].end);

			fprintf(f, " }");
			if (k + 1 < groups[j].n) {
				fprintf(f, ",");
			}
			fprintf(f, "\n");
		}

		fprintf(f, "\t\t\t\t\t]\n");

		fprintf(f, "\t\t\t\t}");
		if (j + 1 < n) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}

	fprintf(f, "\t\t\t]\n");
}

static void
print_cs(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, const struct ir_state *cs)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(cs != NULL);

	fprintf(f, "\t\t{\n");

	fprintf(f, "\t\t\t\"end\": %s,\n", cs->isend ? "true" : "false");
	fprintf(f, "\t\t\t\"strategy\": \"%s\",\n", strategy_name(cs->strategy));

	if (cs->example != NULL) {
		fprintf(f, "\t\t\t\"example\": \"");
		escputs(f, opt, json_escputc, cs->example);
		fprintf(f, "\"");
		if (cs->strategy != IR_NONE) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}

	/* TODO: leaf callback for json output */

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		fprintf(f, "\t\t\t\"to\": %u\n", cs->u.same.to);
		break;

	case IR_MANY:
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, ir, cs->u.many.groups, cs->u.many.n);
		break;

	case IR_MODE:
		fprintf(f, "\t\t\t\"mode\": %u,\n", cs->u.mode.mode);
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, ir, cs->u.mode.groups, cs->u.mode.n);
		break;

	case IR_JUMP:
		/* TODO */
		break;

	default:
		;
	}

	fprintf(f, "\t\t}");
}

void
fsm_print_irjson(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	size_t i;

	assert(f != NULL);
	assert(fsm != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	fprintf(f, "{\n");

	fprintf(f, "\t\"start\": %u,\n", ir->start);
	fprintf(f, "\t\"states\": [\n");

	for (i = 0; i < ir->n; i++) {
		print_cs(f, fsm->opt, ir, &ir->states[i]);

		if (i + 1 < ir->n) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}

	fprintf(f, "\t]\n");

	fprintf(f, "}\n");

	free_ir(ir);
}

