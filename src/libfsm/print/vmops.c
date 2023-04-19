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

#include "libfsm/vm/vm.h"

#include "ir.h"

enum vmops_dialect {
	VMOPS_C,
	VMOPS_H,
	VMOPS_MAIN,
};

static int
leaf(FILE *f, const struct fsm_end_ids *ids, const void *leaf_opaque)
{
	assert(f != NULL);
	assert(leaf_opaque == NULL);

	(void) ids;
	(void) leaf_opaque;

	/* XXX: this should be FSM_UNKNOWN or something non-EOF,
	 * maybe user defined */
	fprintf(f, "return TOK_UNKNOWN;");

	return 0;
}

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

static void
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt, enum vmops_dialect dialect)
{
	fprintf(f, "\t\t/* l%" PRIu32 " */\n", op->index);

	if (op->ir_state->example != NULL) {
		fprintf(f, "\t\t/* e.g. \"");
		/* Go's string escape rules are a superset of C's. */
		escputs(f, opt, c_escputc_str, op->ir_state->example);
		fprintf(f, "\" */\n");
	}
}

static void
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt, enum vmops_dialect dialect)
{
	fprintf(f, "\t\t{%s, ", cmp_operator(op->cmp));
	c_escputcharlit(f, opt, op->cmp_arg);
	fprintf(f, ", ");
}

static void
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt, enum vmops_dialect dialect,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "actionRET, -1},\n");
		return;
	}

	fprintf(f, "actionRET, %td},\n", op->ir_state - ir->states);
}

static void
print_branch(FILE *f, const struct dfavm_op_ir *op, enum vmops_dialect dialect)
{
	fprintf(f, "actionGOTO, %" PRIu32 "},\n", op->u.br.dest_arg->index);
}

static void
print_fetch(FILE *f, const struct fsm_options *opt, enum vmops_dialect dialect)
{

	fprintf(f, "\t\t{opEOF, 0, ");
	switch (opt->io) {
	case FSM_IO_STR:
	case FSM_IO_PAIR:
		break;
	default:
		assert(!"unreached");
	}
}

/* TODO: eventually to be non-static */
static int
fsm_print_vmopsfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt, enum vmops_dialect dialect,
	const char *cp,
	int (*leaf)(FILE *, const struct fsm_end_ids *ids, const void *leaf_opaque),
	const void *leaf_opaque)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;
	struct dfavm_op_ir *op;

	static const struct fsm_vm_compile_opts vm_opts = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(cp != NULL);

	a = zero;

	/* TODO: we don't currently have .opaque information attached to struct dfavm_op_ir.
	 * We'll need that in order to be able to use the leaf callback here. */
	(void) leaf;
	(void) leaf_opaque;

	/* TODO: we'll need to heed cp for e.g. lx's codegen */
	(void) cp;

	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		return -1;
	}

	for (op = a.linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			print_label(f, op, opt, dialect);
		}
		switch (op->instr) {
		case VM_OP_STOP:
			print_cond(f, op, opt, dialect);
			print_end(f, op, opt,  dialect, op->u.stop.end_bits, ir);
			break;

		case VM_OP_FETCH:
			print_fetch(f, opt, dialect);
			print_end(f, op, opt, dialect, op->u.fetch.end_bits, ir);
			break;

		case VM_OP_BRANCH:
			print_cond(f, op, opt, dialect);
			print_branch(f, op, dialect);
			break;

		default:
			assert(!"unreached");
			break;
		}
	}

	dfavm_opasm_finalize_op(&a);

	return 0;
}

static void
fsm_print_vmops_complete(FILE *f, const struct ir *ir, const struct fsm_options *opt, enum vmops_dialect dialect)
{
	/* TODO: currently unused, but must be non-NULL */
	const char *cp = "";

	assert(f != NULL);
	assert(ir != NULL);
	assert(opt != NULL);

	switch (dialect) {
	case VMOPS_H:
	case VMOPS_MAIN:
		return;
	default:
		break;
	}

	(void) fsm_print_vmopsfrag(f, ir, opt, dialect, cp,
		opt->leaf != NULL ? opt->leaf : leaf, opt->leaf_opaque);
}

void
fsm_print_vmops(FILE *f, const struct fsm *fsm, enum vmops_dialect dialect)
{
	struct ir *ir;
	const char *prefix;
	const char *package_prefix;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (fsm->opt->fragment) {
		fsm_print_vmops_complete(f, ir, fsm->opt, dialect);
		return;
	}

	if (fsm->opt->package_prefix != NULL) {
		package_prefix = fsm->opt->package_prefix;
	} else {
		package_prefix = prefix;
	}

	switch (dialect) {
	case VMOPS_C:
		fprintf(f, "#include <stdint.h>\n\n");
		fprintf(f, "#ifndef LIBFSM_VMOPS_H\n");
		fprintf(f, "#include \"vmops.h\"\n");
		fprintf(f, "#endif /* LIBFSM_VMOPS_H */\n");
		fprintf(f, "struct op %sOps[] = {\n", prefix);
		fsm_print_vmops_complete(f, ir, fsm->opt, dialect);
		fprintf(f, "\t};\n");
		break;

	case VMOPS_H:
		fprintf(f, "#ifndef LIBFSM_VMOPS_H\n");
		fprintf(f, "#define LIBFSM_VMOPS_H\n");
		fprintf(f, "#include <stdint.h>\n\n");
		fprintf(f, "enum vmOp { opEOF, opLT, opLE, opEQ, opNE, opGE, opGT, opALWAYS};\n");
		fprintf(f, "enum actionOp { actionRET, actionGOTO };\n");
		fprintf(f, "struct op { enum vmOp op; char c; enum actionOp action; int32_t arg; };\n\n");
		fprintf(f, "#endif /* LIBFSM_VMOPS_H */\n");
		break;

	case VMOPS_MAIN:
		fprintf(f, "#include <stdio.h>\n");
		fprintf(f, "#include <stdlib.h>\n\n");
		fprintf(f, "#ifndef LIBFSM_VMOPS_H\n");
		fprintf(f, "#include \"vmops.h\"\n");
		fprintf(f, "#endif /* LIBFSM_VMOPS_H */\n");
		fprintf(f, "extern struct op %sOps[];\n", prefix);
		fprintf(f, "\n");

		switch (fsm->opt->io) {
		case FSM_IO_PAIR:
			fprintf(f, "int %smatch(const char *b, const char *e)\n", prefix);
			break;
		case FSM_IO_STR:
			fprintf(f, "int %smatch(const char *s)\n", prefix);
			break;
		case FSM_IO_GETC:
			fprintf(stderr, "unsupported IO API\n");
			break;
		}
		fprintf(f, "{\n");
		fprintf(f, "\tunsigned int i = 0;\n");
		fprintf(f, "\tchar c;\n");
		fprintf(f, "\tint ok;\n");
		fprintf(f, "\tstruct op *ops = %sOps;\n", prefix);


		switch (fsm->opt->io) {
		case FSM_IO_PAIR:
			fprintf(f, "\tconst char *p = b;\n");
			break;
		case FSM_IO_STR:
			fprintf(f, "\tconst char *p = s;\n");
			break;
		case FSM_IO_GETC:
			fprintf(stderr, "unsupported IO API\n");
			break;
		}

		fprintf(f, "\n");
		fprintf(f, "\tfor (;;) {\n");
		fprintf(f, "\t\tok = 0;\n");
		fprintf(f, "\t\tswitch (ops[i].op) {\n");
		fprintf(f, "\t\tcase opEOF:\n");

		switch (fsm->opt->io) {
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
			fprintf(stderr, "unsupported IO API\n");
			break;
		}

		fprintf(f, "\t\t\tok = 1;\n");
		fprintf(f, "\t\t\tbreak;\n");
		fprintf(f, "\t\tcase opLT: ok = c < ops[i].c; break;\n");
		fprintf(f, "\t\tcase opLE: ok = c <= ops[i].c; break;\n");
		fprintf(f, "\t\tcase opEQ: ok = c == ops[i].c; break;\n");
		fprintf(f, "\t\tcase opNE: ok = c != ops[i].c; break;\n");
		fprintf(f, "\t\tcase opGE: ok = c >= ops[i].c; break;\n");
		fprintf(f, "\t\tcase opGT: ok = c > ops[i].c; break;\n");
		fprintf(f, "\t\tcase opALWAYS: ok = 1; break;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\t\tif (ok) {\n");
		fprintf(f, "\t\t\tif (ops[i].action == actionRET) {\n");
		fprintf(f, "\t\t\t\treturn (int) (ops[i].arg);\n");
		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t\ti = ops[i].arg;\n");
		fprintf(f, "\t\t\tcontinue;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\t\ti++;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
		fprintf(f, "#define BUFFER_SIZE (1024)\n");
		fprintf(f, "\n");
		fprintf(f, "int main(void)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tchar *buf;\n");
		fprintf(f, "\tsize_t linecap = BUFFER_SIZE;\n");
		fprintf(f, "\tssize_t linelen;\n");
		fprintf(f, "\tint r;\n");
		fprintf(f, "\n");
		fprintf(f, "\tbuf = malloc(BUFFER_SIZE);\n");
		fprintf(f, "\tif (!buf) {\n");
		fprintf(f, "\t\tperror(\"malloc\");\n");
		fprintf(f, "\t\texit(1);\n");
		fprintf(f, "\t}\n\n");
		fprintf(f, "\tfor (;;) {\n");
		fprintf(f, "\t\tlinelen = getline(&buf, &linecap, stdin);\n");
		fprintf(f, "\t\tif (linelen <= 0) {\n");
		fprintf(f, "\t\t\tbreak;\n");
		fprintf(f, "\t\t}\n");

		switch (fsm->opt->io) {
		case FSM_IO_PAIR:
			fprintf(f, "\t\tr = %smatch(buf, buf + linelen);\n", prefix);
			break;
		case FSM_IO_STR:
			fprintf(f, "\t\tr = %smatch(buf);\n", prefix);
			break;
		case FSM_IO_GETC:
			fprintf(stderr, "unsupported IO API\n");
			break;
		}
		fprintf(f, "\t\tprintf(\"%%smatch\\n\", (r == -1) ? \"no \" : \"\");\n");
		fprintf(f, "\t}\n");
		fprintf(f, "}\n");
		break;
	}

	free_ir(fsm, ir);
}

void
fsm_print_vmops_c(FILE *f, const struct fsm *fsm) {
	fsm_print_vmops(f, fsm, VMOPS_C);
}

void
fsm_print_vmops_h(FILE *f, const struct fsm *fsm) {
	fsm_print_vmops(f, fsm, VMOPS_H);
}

void
fsm_print_vmops_main(FILE *f, const struct fsm *fsm) {
	fsm_print_vmops(f, fsm, VMOPS_MAIN);
}
