/*
 * Copyright 2008-2020 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

#include <print/esc.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/vm.h>

#include "libfsm/internal.h"
#include "libfsm/print.h"

#include "libfsm/vm/retlist.h"
#include "libfsm/vm/vm.h"

static const char *
str_operator(int cmp)
{
	switch (cmp) {
	case VM_CMP_LT: return "<";
	case VM_CMP_LE: return "<"; /* special case, <= doesn't exist */
	case VM_CMP_EQ: return "=";
	case VM_CMP_GE: return ">"; /* special case, >= doesn't exist */
	case VM_CMP_GT: return ">";
	case VM_CMP_NE: return "!=";

	case VM_CMP_ALWAYS:
	default:
		assert("unreached");
		return NULL;
	}
}

static const char *
ord_operator(int cmp)
{
	switch (cmp) {
	case VM_CMP_LT: return "-lt";
	case VM_CMP_LE: return "-le";
	case VM_CMP_EQ: return "-eq";
	case VM_CMP_GE: return "-ge";
	case VM_CMP_GT: return "-gt";
	case VM_CMP_NE: return "-ne";

	case VM_CMP_ALWAYS:
	default:
		assert("unreached");
		return NULL;
	}
}

static int
print_ids(FILE *f,
	enum fsm_ambig ambig, const fsm_end_id_t *ids, size_t count)
{
	switch (ambig) {
	case AMBIG_NONE:
		break;

	case AMBIG_ERROR:
// TODO: decide if we deal with this ahead of the call to print or not
		if (count > 1) {
			errno = EINVAL;
			return -1;
		}

		fprintf(f, " %u", ids[0]);
		break;

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, and are sorted.
		 */
		fprintf(f, " %u", ids[0]);
		break;

	case AMBIG_MULTIPLE:
		for (size_t i = 0; i < count; i++) {
			fprintf(f, " %u", ids[i]);
		}
		break;

	default:
		assert(!"unreached");
		abort();
	}

	return 0;
}

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;

	fprintf(f, "matched");

	if (-1 == print_ids(f, opt->ambig, state_metadata->end_ids, state_metadata->end_id_count)) {
		return -1;
	}

	return 0;
}

static int
default_reject(FILE *f, const struct fsm_options *opt,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);

	(void) lang_opaque;
	(void) hook_opaque;

	fprintf(f, "fail");

	return 0;
}

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "l%" PRIu32 ")", op->index);

	/*
	 * The example strings here are just for human-readable comments;
	 * double quoted strings in sh do not support numeric escapes for
	 * arbitrary characters, and nobody can read $'...' style strings.
	 * So I'm just using C-style strings here because it's simple enough.
	 */
	if (op->example != NULL) {
		fprintf(f, " # e.g. \"");
		escputs(f, opt, c_escputc_str, op->example);
		fprintf(f, "\"");
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op)
{
	char c;

	if (op->cmp == VM_CMP_ALWAYS) {
		return;
	}

	fprintf(f, "[[ ");

	c = op->cmp_arg;

	if (c == '\0') {
		fprintf(f, "$c %s ''", str_operator(op->cmp));
		if (op->cmp == VM_CMP_LE || op->cmp == VM_CMP_GE) {
			fprintf(f, " || $c %s ''", str_operator(VM_CMP_EQ));
		}
	} else if (c == '\'') {
		fprintf(f, "$c %s \'", str_operator(op->cmp));
		if (op->cmp == VM_CMP_LE || op->cmp == VM_CMP_GE) {
			fprintf(f, " || $c %s \\'", str_operator(VM_CMP_EQ));
		}
	} else if (isgraph((unsigned char) c) || c == ' ') {
		fprintf(f, "$c %s '%c'", str_operator(op->cmp), c);
		if (op->cmp == VM_CMP_LE || op->cmp == VM_CMP_GE) {
			fprintf(f, " || $c %s '%c'", str_operator(VM_CMP_EQ), c);
		}
	} else {
		fprintf(f, "$(ord \"$c\") %s %3u", ord_operator(op->cmp), (unsigned char) c);
	}
	fprintf(f, " ]] && ");
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	enum dfavm_op_end end_bits)
{
	switch (end_bits) {
	case VM_END_FAIL:
		return print_hook_reject(f, opt, hooks, default_reject, NULL);

	case VM_END_SUCC:;
		const struct fsm_state_metadata state_metadata = {
			.end_ids = op->ret->ids,
			.end_id_count = op->ret->count,
		};
		if (-1 == print_hook_accept(f, opt, hooks,
			&state_metadata,
			default_accept,
			NULL))
		{
			return -1;
		}

		if (-1 == print_hook_comment(f, opt, hooks,
			&state_metadata))
		{
			return -1;
		}

		return 0;

	default:
		assert(!"unreached");
		abort();
	}

	return 0;
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
	fprintf(f, "{ l=l%" PRIu32 "; continue; }", op->u.br.dest_arg->index);
}

static void
print_fetch(FILE *f)
{
	fprintf(f, "read -rn 1 c || ");
}

int
fsm_print_sh(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(retlist != NULL);

	if (opt->io != FSM_IO_STR) {
		errno = ENOTSUP;
		return -1;
	}

	/*
	 * We only output labels for ops which are branched to. This gives
	 * gaps in the sequence for ops which don't need a label.
	 * So here we renumber just the ones we use.
	 */
	{
		uint32_t l;

		l = 0;

		for (op = ops; op != NULL; op = op->next) {
			if (op->num_incoming > 0) {
				op->index = l++;
			}
		}
	}

	fprintf(f, "ord() { printf %%d \"'$1\"; }\n");
	fprintf(f, "fail() { echo fail; exit 1; }\n");
	fprintf(f, "matched() { echo match; exit 0; }\n");
	fprintf(f, "\n");

	fprintf(f, "l=start\n");
	fprintf(f, "LANG=C\n");
	fprintf(f, "IFS=\n");
	fprintf(f, "\n");

	fprintf(f, "# generated\n");
	fprintf(f, "while true; do\n");
	fprintf(f, "\tcase $l in\n");
	fprintf(f, "\tstart)\n");

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			if (op != ops) {
				fprintf(f, "\t\t;&\n");
			}

			fprintf(f, "\t");
			print_label(f, op, opt);
			fprintf(f, "\n");
		}

		fprintf(f, "\t\t");

		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op);
			if (-1 == print_end(f, op, opt, hooks, op->u.stop.end_bits)) {
				return -1;
			}
			break;

		case VM_OP_FETCH:
			print_fetch(f);
			if (-1 == print_end(f, op, opt, hooks, op->u.fetch.end_bits)) {
				return -1;
			}
			break;

		case VM_OP_BRANCH:
			print_cond(f, op);
			print_branch(f, op);
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "\n");
	}

	fprintf(f, "\t\t;;\n");
	fprintf(f, "\tesac\n");
	fprintf(f, "done\n");

	return 0;
}

