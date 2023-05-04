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

static int
print_label(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
	fprintf(f, "\t\t/* l%" PRIu32 " */\n", op->index);

	if (op->ir_state->example != NULL) {
		fprintf(f, "\t\t/* e.g. \"");
		/* Go's string escape rules are a superset of C's. */
		if (-1 == escputs(f, opt, c_escputc_str, op->ir_state->example)) {
			return -1;
		}
		fprintf(f, "\" */\n");
	}

	return 0;
}

static int
print_cond(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt, const char *prefix)
{
	fprintf(f, "\t\t{%s%s, ", prefix, cmp_operator(op->cmp));
	if (-1 == c_escputcharlit(f, opt, op->cmp_arg)) {
		return -1;
	}
	fprintf(f, ", ");

	return 0;
}

static int
print_end(FILE *f, const struct dfavm_op_ir *op, const char *prefix,
	enum dfavm_op_end end_bits, const struct ir *ir)
{
	if (end_bits == VM_END_FAIL) {
		fprintf(f, "%sactionRET, -1},\n", prefix);
		return 0;
	}

	fprintf(f, "%sactionRET, %td},\n", prefix, op->ir_state - ir->states);

	return 0;
}

static int
print_branch(FILE *f, const struct dfavm_op_ir *op, const char *prefix)
{
	fprintf(f, "%sactionGOTO, %" PRIu32 "},\n", prefix, op->u.br.dest_arg->index);

	return 0;
}

static int
print_fetch(FILE *f, const struct fsm_options *opt, const char *prefix)
{

	fprintf(f, "\t\t{%sopEOF, 0, ", prefix);
	switch (opt->io) {
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
fsm_print_vmopsfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt, const char *prefix,
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

	a = zero;

	/* TODO: we don't currently have .opaque information attached to struct dfavm_op_ir.
	 * We'll need that in order to be able to use the leaf callback here. */
	(void) leaf;
	(void) leaf_opaque;

	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		return -1;
	}

	for (op = a.linked; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			if (-1 == print_label(f, op, opt)) {
				return -1;
			}
		}
		switch (op->instr) {
		case VM_OP_STOP:
			if (-1 == print_cond(f, op, opt, prefix)) {
				return -1;
			}
			if (-1 == print_end(f, op, prefix, op->u.stop.end_bits, ir)) {
				return -1;
			}
			break;

		case VM_OP_FETCH:
			if (-1 == print_fetch(f, opt, prefix)) {
				return -1;
			}
			if (-1 == print_end(f, op, prefix, op->u.fetch.end_bits, ir)) {
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
	}

	dfavm_opasm_finalize_op(&a);

	return 0;
}

int
fsm_print_vmops(FILE *f, const struct fsm *fsm, enum vmops_dialect dialect)
{
	struct ir *ir;
	const char *prefix;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return -1;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (fsm->opt->fragment) {
		if (dialect == VMOPS_C) {
			if (-1 == fsm_print_vmopsfrag(f, ir, fsm->opt, prefix,
				fsm->opt->leaf != NULL ? fsm->opt->leaf : leaf, fsm->opt->leaf_opaque))
			{
				return -1;
			}
		}
	} else {
		switch (dialect) {
		case VMOPS_C:
			fprintf(f, "#include <stdint.h>\n\n");
			fprintf(f, "#ifndef %sLIBFSM_VMOPS_H\n", prefix);
			fprintf(f, "#include \"%svmops.h\"\n", prefix);
			fprintf(f, "#endif /* %sLIBFSM_VMOPS_H */\n", prefix);
			fprintf(f, "struct %sop %sOps[] = {\n", prefix, prefix);
			if (-1 == fsm_print_vmopsfrag(f, ir, fsm->opt, prefix,
				fsm->opt->leaf != NULL ? fsm->opt->leaf : leaf, fsm->opt->leaf_opaque))
			{
				return -1;
			}
			fprintf(f, "\t};\n");
			break;

		case VMOPS_H:
			fprintf(f, "#ifndef %sLIBFSM_VMOPS_H\n", prefix);
			fprintf(f, "#define %sLIBFSM_VMOPS_H\n", prefix);
			fprintf(f, "#include <stdint.h>\n\n");
			fprintf(f, "enum %svmOp { %sopEOF, %sopLT, %sopLE, %sopEQ, %sopNE, %sopGE, %sopGT, %sopALWAYS};\n",
				prefix, prefix, prefix, prefix, prefix, prefix, prefix, prefix, prefix);
			fprintf(f, "enum %sactionOp { %sactionRET, %sactionGOTO };\n", prefix, prefix, prefix);
			fprintf(f, "struct %sop { enum %svmOp op; unsigned char c; enum %sactionOp action; int32_t arg; };\n\n",
				prefix, prefix, prefix);
			fprintf(f, "#endif /* %sLIBFSM_VMOPS_H */\n", prefix);
			break;

		case VMOPS_MAIN:
			fprintf(f, "#include <stdio.h>\n");
			fprintf(f, "#include <string.h>\n");
			fprintf(f, "#include <stdlib.h>\n\n");
			fprintf(f, "#ifndef %sLIBFSM_VMOPS_H\n", prefix);
			fprintf(f, "#include \"%svmops.h\"\n", prefix);
			fprintf(f, "#endif /* %sLIBFSM_VMOPS_H */\n", prefix);
			fprintf(f, "extern struct %sop %sOps[];\n", prefix, prefix);
			fprintf(f, "\n");

			switch (fsm->opt->io) {
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

			switch (fsm->opt->io) {
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

			switch (fsm->opt->io) {
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
			fprintf(f, "\t\tprintf(\"%%smatch\\n\", (r == -1) ? \"no \" : \"\");\n");
			fprintf(f, "\t}\n");
			fprintf(f, "\treturn 0;\n");
			fprintf(f, "}\n");
			break;
		}
	}

	free_ir(fsm, ir);

	if (ferror(f)) {
		return -1;
	}

	return 0;
}

int
fsm_print_vmops_c(FILE *f, const struct fsm *fsm)
{
	return fsm_print_vmops(f, fsm, VMOPS_C);
}

int
fsm_print_vmops_h(FILE *f, const struct fsm *fsm)
{
	return fsm_print_vmops(f, fsm, VMOPS_H);
}

int
fsm_print_vmops_main(FILE *f, const struct fsm *fsm)
{
	return fsm_print_vmops(f, fsm, VMOPS_MAIN);
}

