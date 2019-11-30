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
	case IR_NONE:     return "none";
	case IR_SAME:     return "same";
	case IR_COMPLETE: return "complete";
	case IR_PARTIAL:  return "partial";
	case IR_DOMINANT: return "dominant";
	case IR_ERROR:    return "error";
	case IR_TABLE:    return "table";

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
print_ranges(FILE *f, const struct fsm_options *opt,
	const struct ir *ir,
	const struct ir_range *ranges, size_t n)
{
	size_t k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(ranges != NULL);

	(void)ir; /* unused */

	for (k = 0; k < n; k++) {
		fprintf(f, "\t\t\t\t\t\t{ ");

		fprintf(f, "\"start\": ");
		print_endpoint(f, opt, ranges[k].start);
		fprintf(f, ", ");
		fprintf(f, "\"end\": ");
		print_endpoint(f, opt, ranges[k].end);

		fprintf(f, " }");
		if (k + 1 < n) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}
}

static void
print_groups(FILE *f, const struct fsm_options *opt,
	const struct ir *ir,
	const struct ir_group *groups, size_t n)
{
	size_t j;

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
		print_ranges(f, opt, ir, groups[j].ranges, groups[j].n);
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

	case IR_COMPLETE:
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, ir, cs->u.complete.groups, cs->u.complete.n);
		break;

	case IR_PARTIAL:
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, ir, cs->u.partial.groups, cs->u.partial.n);
		break;

	case IR_DOMINANT:
		fprintf(f, "\t\t\t\"mode\": %u,\n", cs->u.dominant.mode);
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, ir, cs->u.dominant.groups, cs->u.dominant.n);
		break;

	case IR_ERROR:
		fprintf(f, "\t\t\t\"mode\": %u,\n", cs->u.error.mode);
		fprintf(f, "\t\t\t\"error\": [\n");
		print_ranges(f, opt, ir, cs->u.error.error.ranges, cs->u.error.error.n);
		fprintf(f, "\t\t\t],\n");
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, ir, cs->u.error.groups, cs->u.error.n);
		break;

	case IR_TABLE:
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

	free_ir(fsm, ir);
}

