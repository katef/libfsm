/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

// TODO: centralise indicies for locals
// why not use $x for names?
#define LOCAL_STR 0
#define LOCAL_CHAR 1
#define LOCAL_STATE 2

#define ERROR_STATE (uint32_t) -1

static void
transition(FILE *f, unsigned index, unsigned to,
	const char *indent)
{
	if (to == ERROR_STATE) {
		fprintf(f, "      i32.const 0\n");
		fprintf(f, "      return\n");
		fprintf(f, "      end_function\n");
		return;
	}

	// no change
	if (to == index) {
		return;
	}

	// set next state
	fprintf(f, "%si32.const %u\n", indent, to);
	fprintf(f, "%slocal.set %u\n", indent, LOCAL_STATE);
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, unsigned char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	fprintf(f, "    i32.const %u", (unsigned char) c);

	if (opt->comments) {
		fprintf(f, " // \'");
		json_escputc(f, opt, c);
		fprintf(f, "\'");
	}

	fprintf(f, "\n");
}

static void
print_range(FILE *f, const struct fsm_options *opt,
	const struct ir_range *range)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(range != NULL);

	// TODO: could identify the same ranges but upper and lowercase,
	// and take advantage of ascii's single-bit difference.
	// do that in the IR, add a "is case insensitive" bool to the range,
	// and make it one case only

	// leaves a boolean on the stack
	if (range->end == range->start) {
		fprintf(f, "    local.get %u\n", LOCAL_CHAR); // get current input byte
		print_endpoint(f, opt, range->start);
		fprintf(f, "    i32.eq\n");
	} else {
		fprintf(f, "    local.get %u\n", LOCAL_CHAR); // get current input byte
		print_endpoint(f, opt, range->start);
		fprintf(f, "    i32.ge_u\n");

		fprintf(f, "    local.get %u\n", LOCAL_CHAR); // get current input byte
		print_endpoint(f, opt, range->end);
		fprintf(f, "    i32.le_u\n");

		fprintf(f, "    i32.and\n");
	}
}

static void
print_ranges(FILE *f, const struct fsm_options *opt,
	const struct ir_range *ranges, size_t n,
	bool complete)
{
	size_t k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ranges != NULL);
	assert(n > 0);

	/* A single range already leaves its own bool on the stack */
	if (n == 1) {
		print_range(f, opt, &ranges[0]);
		return;
	}

	/*
	 * For multiple ranges, we emit an if/else chain so that we can
	 * produce a single boolean and bail out on the first match,
	 * without evaulating every condition.
	 *
	 * This is a || b || c with short-circuit evaulation.
	 * In any case, we end up with a single bool on the stack.
	 */

	/* TODO: would prefer to emit binary search for ranges,
	 * perhaps with IR support. */

	for (k = 0; k < n; k++) {
		print_range(f, opt, &ranges[k]);
		fprintf(f, "    if i32\n");
		fprintf(f, "      i32.const 1 // match \n");
// XXX: we don't want to return here, just leave a bool on the stack for (result i32)
// fprintf(f, "      return\n");

		if (k + 1 < n) {
			fprintf(f, "    else\n");
		}
	}

	if (!complete) {
		fprintf(f, "    else\n");
		fprintf(f, "      i32.const 0 // no match \n");
		fprintf(f, "      return\n");
	}

	for (k = 0; k < n; k++) {
		fprintf(f, "    end_if\n");
	}
}

static void
print_groups(FILE *f, const struct fsm_options *opt,
	const struct ir_group *groups, size_t n,
	unsigned index, bool complete,
	unsigned default_to)
{
	size_t j;

	assert(f != NULL);
	assert(opt != NULL);
	assert(groups != NULL);
	assert(n > 0);

	/*
	 * Here leave nothing on the stack, but transition() as side effect.
	 *
	 * We prefer this as a side effect rather than accumulating a
	 * destination on the stack, because we can skip effects for where
	 * the destination state is unchanged from the current index.
	 */
// TODO: explain we need another if/else chain for short-circuit evaulation of the groups
// this one doesn't return a value, because each group transition()s as a side effect
// we can't avoid the if/else chain for a single group, beause we still need to convert bool to transition() per group

	for (j = 0; j < n; j++) {
		print_ranges(f, opt, groups[j].ranges, groups[j].n, complete);

		fprintf(f, "    if\n");
		transition(f, index, groups[j].to, "      ");

		if (j + 1 < n) {
			fprintf(f, "    else\n");
		}
	}

	if (!complete && default_to != index) {
		fprintf(f, "    else\n");
		transition(f, index, default_to, "      ");
	}

	for (j = 0; j < n; j++) {
		fprintf(f, "    end_if\n");
	}
}

static int
print_state(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir_state *cs,
	unsigned index,
	unsigned delta)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(cs != NULL);

	/* showing hook in addition to existing content */
	if (cs->isend && hooks->accept != NULL) {
		if (-1 == print_hook_accept(f, opt, hooks,
			cs->ids, cs->count,
			NULL, NULL))
		{
			return -1;
		}
	}

	fprintf(f, "    end_block\n");
	fprintf(f, "\n");

	fprintf(f, "    // S%u", index);
	if (cs->example != NULL) {
		fprintf(f, " \"");
		escputs(f, opt, json_escputc, cs->example);
		fprintf(f, "\"");
	}
	fprintf(f, "\n");

	switch (cs->strategy) {
	case IR_NONE:
		fprintf(f, "    // IR_NONE\n");
		transition(f, index, ERROR_STATE, "    ");
		return 0;

	case IR_SAME:
		fprintf(f, "    // IR_SAME\n");
		transition(f, index, cs->u.same.to, "    ");
		break;

	case IR_COMPLETE:
		fprintf(f, "    // IR_COMPLETE\n");
		print_groups(f, opt, cs->u.complete.groups, cs->u.complete.n, index, true, ERROR_STATE);
		break;

	case IR_PARTIAL:
		fprintf(f, "    // IR_PARTIAL\n");
		print_groups(f, opt, cs->u.partial.groups, cs->u.partial.n, index, false, ERROR_STATE);
		fprintf(f, "\n");
		break;

	case IR_DOMINANT:
		fprintf(f, "    // IR_DOMINANT\n");
		print_groups(f, opt, cs->u.dominant.groups, cs->u.dominant.n, index, false, cs->u.dominant.mode);
		break;

	case IR_ERROR:
		fprintf(f, "    // IR_ERROR\n");
		print_ranges(f, opt, cs->u.error.error.ranges, cs->u.error.error.n, false);
		fprintf(f, "    if\n");
		transition(f, index, ERROR_STATE, "      ");

		if (cs->u.error.n > 0) {
			fprintf(f, "    else\n");
			print_groups(f, opt, cs->u.error.groups, cs->u.error.n, index, true, cs->u.error.mode);
		}

		fprintf(f, "    end_if\n");
		break;

	case IR_TABLE:
		fprintf(f, "    // IR_TABLE\n");
		fprintf(f, "    local.get %u\n", LOCAL_CHAR); // get current input byte
		fprintf(f, "    drop\n"); // TODO: do something with it ...
		// TODO: would emit br_table here
		assert(!"unreached");
		break;

	default:
		;
	}

	fprintf(f, "    br %u // continue the loop, %u block%s up\n", delta - 1, delta - 1, &"s"[delta - 1 == 1]);

	return 0;
}

int
fsm_print_wasm(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	const struct ir *ir)
{
	const char *prefix;
	size_t i;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(ir != NULL);
	assert(retlist != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (!opt->fragment) {
                fprintf(f, ".global  %smatch\n", prefix);
                fprintf(f, ".hidden  %smatch\n", prefix);
                fprintf(f, ".type    %smatch,@function\n", prefix);
                fprintf(f, "%smatch:\n", prefix);
                fprintf(f, ".functype %smatch (i32) -> (i32)\n", prefix);
                fprintf(f, ".local i32, i32\n");
		fprintf(f, "// (memory 1 1) // TODO(dgryski): I guess we don't need this line?\n"); // input
	}

// TODO: export to component model, use opt.package_prefix
// TODO: various IO APIs
// TODO: endids

//	fprintf(f, "  (func (param i32) (result i32) (local i32 i32)\n");
//	fprintf(f, "    // s is in LOCAL_STR (the parameter) and we'll keep p there too\n");
//	fprintf(f, "    // we'll cache *p in LOCAL_CHAR\n");
//	fprintf(f, "\n");

	// the current state will be in LOCAL_STATE
	// locals are implicitly initialized to 0
	if (ir->start != 0) {
		fprintf(f, "    // start S%u\n", ir->start);
		fprintf(f, "    i32.const %u\n", ir->start);
		fprintf(f, "    local.set %u\n", LOCAL_STATE);
		fprintf(f, "\n");
	}

//	fprintf(f, "    // for (p = s; *p != '\0'; p++)\n");
	fprintf(f, "    block\n"); // introduce a branch target for getting out of the loop
	fprintf(f, "    loop\n"); // begin the outer loop
	fprintf(f, "\n");

	fprintf(f, "    // fetch *p\n");
	fprintf(f, "    local.get %u\n", LOCAL_STR); // get address of next byte
	fprintf(f, "    i32.load8_u 0\n"); // load byte at that address
	fprintf(f, "    local.tee %u\n", LOCAL_CHAR); // save the current input byte and keep it on the stack
	fprintf(f, "\n");

	fprintf(f, "    // *p != '\\0'\n");
	fprintf(f, "    i32.eqz\n"); // test if the byte is zero
	fprintf(f, "    br_if 1\n"); // exit the outer block if so
	fprintf(f, "\n");

	fprintf(f, "    // p++\n");
	fprintf(f, "    local.get %u\n", LOCAL_STR);
	fprintf(f, "    i32.const 1\n");
	fprintf(f, "    i32.add\n");
	fprintf(f, "    local.set %u\n", LOCAL_STR);
	fprintf(f, "\n");

//	fprintf(f, "    // switch (state)\n");
//	fprintf(f, "    // we need a block for each state: we'll start with a jump-table that\n");
//	fprintf(f, "    // branches out of the block which ends before the code we want to run\n");
	for (i = 0; i < ir->n; i++) {
		fprintf(f, "    block // S%zu\n", i);
	}
	fprintf(f, "    local.get %u\n", LOCAL_STATE);
	fprintf(f, "    br_table {");
	for (i = 0; i < ir->n; i++) {
		fprintf(f, "%s %zu", i > 0 ? "," : "", i);
	}
	fprintf(f, "}\n");

	for (i = 0; i < ir->n; i++) {
		if (i == ERROR_STATE) {
			errno = EINVAL;
			return -1;
		}

		if (-1 == print_state(f, opt, hooks, &ir->states[i], i, ir->n - i)) {
			return -1;
		}
	}

	fprintf(f, "    end_loop\n"); // end of loop
	fprintf(f, "    end_block\n"); // end of outer block
	fprintf(f, "\n");

// TODO: use retlist
// need to index state -> retlist entry, maybe index into data for that
// seems better than scattering it throughout the code as a local maybe
//
//	fprintf(f, "\t\t\t\"end\": %s,\n", cs->isend ? "true" : "false");
//	if (cs->isend && cs->count > 0) {
//		fprintf(f, "\t\t\t\"end_id\": [");
//		for (size_t i = 0; i < cs->count; i++) {
//			fprintf(f, "%u", cs->ids[i]);
//		}
//		fprintf(f, "],\n");
//	}

/* XXX: wasm2c 1.27 doesn't support multi-memory
	fprintf(f, "    local.get %u\n", LOCAL_STATE);
	fprintf(f, "    i32.load8_u 0\n"); // load from memory indexed by current state
*/

	/*
	 * XXX: wasm2c 1.27 doesn't support multi-memory.
	 * For indexing end states I would like to:
	 *
	 *   (memory 1 1)
	 *   (data 1 (i32.const 0) // better: use a bitmap
	 *     "\00\01\00\00\01\00...\00")
	 *
	 * and:
	 *
	 *   local.get %u, LOCAL_STATE);
	 *   i32.load8_u 0
	 *
	 * So this O(n) sequence of if statements is a workaround until
	 * we're able to use multi-memory.
	 */
// TODO: i could at least consolidate runs and use i32.ge_u/i32.le_u
	for (i = 0; i < ir->n; i++) {
		if (!ir->states[i].isend) {
			continue;
		}

		fprintf(f, "    local.get %u\n", LOCAL_STATE);
		fprintf(f, "    i32.const %zu // S%zu\n", i, i);
		fprintf(f, "    i32.eq\n");
		fprintf(f, "    if\n");
		fprintf(f, "      i32.const %zu\n", i);
		fprintf(f, "      return\n");
		fprintf(f, "    end_if\n");
		fprintf(f, "\n");
	}

	fprintf(f, "    i32.const 0\n");
	fprintf(f, "    return\n");
	fprintf(f, "    end_function\n");

        /*
	if (!opt->fragment) {
		fprintf(f, ")\n");
	}
        */

	return 0;
}

