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
cmp_operator(int cmp)
{
	switch (cmp) {
	case VM_CMP_LT: return "opLT";
	case VM_CMP_LE: return "opLE";
	case VM_CMP_EQ: return "opEQ";
	case VM_CMP_GE: return "opGE";
	case VM_CMP_GT: return "opGT";
	case VM_CMP_NE: return "opNE";
	case VM_CMP_ALWAYS: return "opALWAYS";
	default:
		assert("unreached");
		return NULL;
	}
}

static int
default_accept(FILE *f, const struct fsm_options *opt,
	const fsm_end_id_t *ids, size_t count,
	void *lang_opaque, void *hook_opaque)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque != NULL);

	(void) hook_opaque;

	prefix = lang_opaque;

	// TODO: print ids
	(void) ids;
	(void) count;

	fprintf(f, "%sactionRET, 1", prefix);

	return 0;
}

static int
default_reject(FILE *f, const struct fsm_options *opt,
	void *lang_opaque, void *hook_opaque)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque != NULL);

	(void) hook_opaque;

	prefix = lang_opaque;

	fprintf(f, "%sactionRET, 0", prefix);

	return 0;
}

static int
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "\t\t/* l%" PRIu32 " */\n", op->index);

	if (op->example != NULL) {
		fprintf(f, "\t\t/* e.g. \"");
		/* Go's string escape rules are a superset of C's. */
		if (-1 == escputs(f, opt, c_escputc_str, op->example)) {
			return -1;
		}
		fprintf(f, "\" */\n");
	}

	return 0;
}

static int
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt, const char *prefix)
{
	fprintf(f, "%s%s, ", prefix, cmp_operator(op->cmp));
	if (-1 == c_escputcharlit(f, opt, op->cmp_arg)) {
		return -1;
	}
	fprintf(f, ", ");

	return 0;
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const char *prefix,
	enum dfavm_op_end end_bits)
{
	switch (end_bits) {
	case VM_END_FAIL:
		if (-1 == print_hook_reject(f, opt, hooks, default_reject,
			(void *) prefix))
		{
			return -1;
		}
		break;

	case VM_END_SUCC:
		if (-1 == print_hook_accept(f, opt, hooks,
			op->ret->ids, op->ret->count,
			default_accept,
			(void *) prefix))
		{
			return -1;
		}
		break;

	default:
		assert(!"unreached");
		abort();
	}

	return 0;
}

static int
print_branch(FILE *f, const struct dfavm_op_ir *op, const char *prefix)
{
	fprintf(f, "%sactionGOTO, %" PRIu32, prefix, op->u.br.dest_arg->index);

	return 0;
}

static int
print_fetch(FILE *f, const struct fsm_options *opt, const char *prefix)
{

	fprintf(f, "%sopEOF, 0, ", prefix);
	switch (opt->io) {
	case FSM_IO_GETC:
	case FSM_IO_STR:
	case FSM_IO_PAIR:
		break;
	default:
		assert(!"unreached");
	}

	return 0;
}

/* TODO: eventually to be non-static */
static int
fsm_print_vmopsfrag(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops,
	const char *prefix)
{
	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(opt != NULL);

	(void) retlist;

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			if (-1 == print_label(f, op, opt)) {
				return -1;
			}
		}

		fprintf(f, "\t\t{");

		switch (op->instr) {
		case VM_OP_STOP:
			if (-1 == print_cond(f, op, opt, prefix)) {
				return -1;
			}
			if (-1 == print_end(f, op, opt, hooks, prefix, op->u.stop.end_bits)) {
				return -1;
			}
			break;

		case VM_OP_FETCH:
			if (-1 == print_fetch(f, opt, prefix)) {
				return -1;
			}
			if (-1 == print_end(f, op, opt, hooks, prefix, op->u.fetch.end_bits)) {
				return -1;
			}
			break;

		case VM_OP_BRANCH:
			if (-1 == print_cond(f, op, opt, prefix)) {
				return -1;
			}
			if (-1 == print_branch(f, op, prefix)) {
				return -1;
			}
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "},\n");
	}

	return 0;
}

int
fsm_print_vmops_c(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (opt->fragment) {
		if (-1 == fsm_print_vmopsfrag(f, opt, hooks, retlist, ops, prefix)) {
			return -1;
		}
	} else {
		fprintf(f, "#include <stdint.h>\n\n");
		fprintf(f, "#ifndef %sLIBFSM_VMOPS_H\n", prefix);
		fprintf(f, "#include \"%svmops.h\"\n", prefix);
		fprintf(f, "#endif /* %sLIBFSM_VMOPS_H */\n", prefix);
		fprintf(f, "struct %sop %sOps[] = {\n", prefix, prefix);
		if (-1 == fsm_print_vmopsfrag(f, opt, hooks, retlist, ops, prefix)) {
			return -1;
		}
		fprintf(f, "\t};\n");
	}

	return 0;
}

int
fsm_print_vmops_h(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	(void) retlist;
	(void) ops;

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	fprintf(f, "#ifndef %sLIBFSM_VMOPS_H\n", prefix);
	fprintf(f, "#define %sLIBFSM_VMOPS_H\n", prefix);
	fprintf(f, "#include <stdint.h>\n\n");
	fprintf(f, "enum %svmOp { %sopEOF, %sopLT, %sopLE, %sopEQ, %sopNE, %sopGE, %sopGT, %sopALWAYS};\n",
		prefix, prefix, prefix, prefix, prefix, prefix, prefix, prefix, prefix);
	fprintf(f, "enum %sactionOp { %sactionRET, %sactionGOTO };\n", prefix, prefix, prefix);
	fprintf(f, "struct %sop { enum %svmOp op; unsigned char c; enum %sactionOp action; int32_t arg; };\n\n",
		prefix, prefix, prefix);
	fprintf(f, "#endif /* %sLIBFSM_VMOPS_H */\n", prefix);

	return 0;
}

int
fsm_print_vmops_main(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	(void) retlist;
	(void) ops;

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "#include <string.h>\n");
	fprintf(f, "#include <stdlib.h>\n\n");
	fprintf(f, "#ifndef %sLIBFSM_VMOPS_H\n", prefix);
	fprintf(f, "#include \"%svmops.h\"\n", prefix);
	fprintf(f, "#endif /* %sLIBFSM_VMOPS_H */\n", prefix);
	fprintf(f, "extern struct %sop %sOps[];\n", prefix, prefix);
	fprintf(f, "\n");

	switch (opt->io) {
	case FSM_IO_PAIR:
		fprintf(f, "int %smatch(const char *b, const char *e)\n", prefix);
		break;

	case FSM_IO_STR:
		fprintf(f, "int %smatch(const char *s)\n", prefix);
		break;

	case FSM_IO_GETC:
		errno = ENOTSUP;
		return -1;
	}
	fprintf(f, "{\n");
	fprintf(f, "\tunsigned int i = 0;\n");
	fprintf(f, "\t/* The compiler doesn't know the op stream will have fetch before the first comparison. */\n");
	fprintf(f, "\t/* Initialize to zero to prevent maybe-uninitialized warning. */\n");
	fprintf(f, "\tunsigned char c = 0;\n");
	fprintf(f, "\tint ok;\n");
	fprintf(f, "\tstruct %sop *ops = %sOps;\n", prefix, prefix);

	switch (opt->io) {
	case FSM_IO_PAIR:
		fprintf(f, "\tconst char *p = b;\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tconst char *p = s;\n");
		break;

	case FSM_IO_GETC:
		errno = ENOTSUP;
		return -1;
	}

	fprintf(f, "\n");
	fprintf(f, "\tfor (;;) {\n");
	fprintf(f, "\t\tok = 0;\n");
	fprintf(f, "\t\tswitch (ops[i].op) {\n");
	fprintf(f, "\t\tcase %sopEOF:\n", prefix);

	switch (opt->io) {
	case FSM_IO_PAIR:
		fprintf(f, "\t\t\tif (p < e) {\n");
		fprintf(f, "\t\t\t\t/* not at EOF */\n");
		fprintf(f, "\t\t\t\tc = *p++;\n");
		fprintf(f, "\t\t\t\ti++;\n");
		fprintf(f, "\t\t\t\tcontinue;\n");
		fprintf(f, "\t\t\t}\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\t\t\tc = *p++;\n");
		fprintf(f, "\t\t\tif (c != '\\0') {\n");
		fprintf(f, "\t\t\t\t/* not at EOF */\n");
		fprintf(f, "\t\t\t\ti++;\n");
		fprintf(f, "\t\t\t\tcontinue;\n");
		fprintf(f, "\t\t\t}\n");
		break;

	case FSM_IO_GETC:
		errno = ENOTSUP;
		return -1;
	}

	fprintf(f, "\t\t\tok = 1;\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\t\tcase %sopLT: ok = c < ops[i].c; break;\n", prefix);
	fprintf(f, "\t\tcase %sopLE: ok = c <= ops[i].c; break;\n", prefix);
	fprintf(f, "\t\tcase %sopEQ: ok = c == ops[i].c; break;\n", prefix);
	fprintf(f, "\t\tcase %sopNE: ok = c != ops[i].c; break;\n", prefix);
	fprintf(f, "\t\tcase %sopGE: ok = c >= ops[i].c; break;\n", prefix);
	fprintf(f, "\t\tcase %sopGT: ok = c > ops[i].c; break;\n", prefix);
	fprintf(f, "\t\tcase %sopALWAYS: ok = 1; break;\n", prefix);
	fprintf(f, "\t\t}\n");
	fprintf(f, "\t\tif (ok) {\n");
	fprintf(f, "\t\t\tif (ops[i].action == %sactionRET) {\n", prefix);
	fprintf(f, "\t\t\t\treturn (int) (ops[i].arg);\n");
	fprintf(f, "\t\t\t}\n");
	fprintf(f, "\t\t\ti = ops[i].arg;\n");
	fprintf(f, "\t\t\tcontinue;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\t\ti++;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");
	fprintf(f, "#define %sBUFFER_SIZE (1024)\n", prefix);
	fprintf(f, "\n");
	fprintf(f, "int main(void)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tchar *buf, *p;\n");
	fprintf(f, "\tint r;\n");
	fprintf(f, "\n");
	fprintf(f, "\tbuf = malloc(%sBUFFER_SIZE);\n", prefix);
	fprintf(f, "\tif (!buf) {\n");
	fprintf(f, "\t\tperror(\"malloc\");\n");
	fprintf(f, "\t\texit(1);\n");
	fprintf(f, "\t}\n\n");
	fprintf(f, "\tfor (;;) {\n");
	fprintf(f, "\t\tp = fgets(buf, %sBUFFER_SIZE, stdin);\n", prefix);
	fprintf(f, "\t\tif (!p) {\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\t\t}\n");

	switch (opt->io) {
	case FSM_IO_PAIR:
		fprintf(f, "\t\tr = %smatch(p, p + strlen(p));\n", prefix);
		break;
	case FSM_IO_STR:
		fprintf(f, "\t\tr = %smatch(p);\n", prefix);
		break;
	case FSM_IO_GETC:
		errno = ENOTSUP;
		return -1;
	}
	fprintf(f, "\t\tprintf(\"%%smatch\\n\", r == 0 ? \"no \" : \"\");\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\treturn 0;\n");
	fprintf(f, "}\n");

	return 0;
}

