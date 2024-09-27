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
	const struct ir_range *ranges, size_t n)
{
	size_t k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ranges != NULL);

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
	const struct ir_group *groups, size_t n)
{
	size_t j;

	assert(f != NULL);
	assert(opt != NULL);
	assert(groups != NULL);

	fprintf(f, "[\n");

	for (j = 0; j < n; j++) {
		assert(groups[j].ranges != NULL);

		fprintf(f, "\t\t\t\t{\n");

		fprintf(f, "\t\t\t\t\t\"to\": %u,\n", groups[j].to);
		fprintf(f, "\t\t\t\t\t\"ranges\": [\n");
		print_ranges(f, opt, groups[j].ranges, groups[j].n);
		fprintf(f, "\t\t\t\t\t]\n");

		fprintf(f, "\t\t\t\t}");
		if (j + 1 < n) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}

	fprintf(f, "\t\t\t]\n");
}

static int
print_state(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir_state *cs)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(cs != NULL);

	fprintf(f, "\t\t{\n");

	fprintf(f, "\t\t\t\"end\": %s,\n", cs->isend ? "true" : "false");
	if (cs->isend && cs->endids.count > 0) {
		fprintf(f, "\t\t\t\"end_id\": [");
		for (size_t i = 0; i < cs->endids.count; i++) {
			fprintf(f, "%u", cs->endids.ids[i]);

			if (i < (size_t) cs->endids.count - 1) {
				fprintf(f, ", ");
			}
		}
		fprintf(f, "],\n");
	}

	const struct fsm_state_metadata state_metadata = {
		.end_ids = cs->endids.ids,
		.end_id_count = cs->endids.count,
	};

	/* showing hook in addition to existing content */
	if (cs->isend && hooks->accept != NULL) {
		fprintf(f, "\t\t\t\"endleaf\": ");

		if (-1 == print_hook_accept(f, opt, hooks,
			&state_metadata,
			NULL, NULL))
		{
			return -1;
		}

		fprintf(f, ",\n");
	}

	fprintf(f, "\t\t\t\"strategy\": \"%s\"", strategy_name(cs->strategy));
	if (cs->example != NULL || cs->strategy != IR_NONE) {
		fprintf(f, ",");
	}
	fprintf(f, "\n");

	if (cs->example != NULL) {
		fprintf(f, "\t\t\t\"example\": \"");
		escputs(f, opt, json_escputc, cs->example);
		fprintf(f, "\"");
		if (cs->strategy != IR_NONE) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		fprintf(f, "\t\t\t\"to\": %u\n", cs->u.same.to);
		break;

	case IR_COMPLETE:
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, cs->u.complete.groups, cs->u.complete.n);
		break;

	case IR_PARTIAL:
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, cs->u.partial.groups, cs->u.partial.n);
		break;

	case IR_DOMINANT:
		fprintf(f, "\t\t\t\"mode\": %u,\n", cs->u.dominant.mode);
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, cs->u.dominant.groups, cs->u.dominant.n);
		break;

	case IR_ERROR:
		fprintf(f, "\t\t\t\"mode\": %u,\n", cs->u.error.mode);
		fprintf(f, "\t\t\t\"error\": [\n");
		print_ranges(f, opt, cs->u.error.error.ranges, cs->u.error.error.n);
		fprintf(f, "\t\t\t],\n");
		fprintf(f, "\t\t\t\"groups\": ");
		print_groups(f, opt, cs->u.error.groups, cs->u.error.n);
		break;

	case IR_TABLE:
		assert(!"unreached");
		break;

	default:
		;
	}

	fprintf(f, "\t\t}");

	return 0;
}

int
fsm_print_irjson(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir *ir)
{
	size_t i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(ir != NULL);

	fprintf(f, "{\n");

	fprintf(f, "\t\"start\": %u,\n", ir->start);
	fprintf(f, "\t\"states\": [\n");

	for (i = 0; i < ir->n; i++) {
		if (-1 == print_state(f, opt, hooks, &ir->states[i])) {
			return -1;
		}

		if (i + 1 < ir->n) {
			fprintf(f, ",");
		}
		fprintf(f, "\n");
	}

	fprintf(f, "\t]\n");

	fprintf(f, "}\n");

	return 0;
}

