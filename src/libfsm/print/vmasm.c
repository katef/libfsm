/*
 * Copyright 2020 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>

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

enum asm_dialect {
	AMD64_ATT,
	AMD64_NASM,
	AMD64_GO,
};

/* What I want:
 *
 * int32_t
 * funcname(struct STATE *st, const char *buf, size_t n);
 *
 * What I'll settle for:
 *
 * int32_t
 * funcname(const char *buf, uint64_t n);
 *
 */
static int
print_asm_amd64(FILE *f, const char *funcname, const struct ir *ir, const struct fsm_options *opt, const struct dfavm_assembler_ir *a, enum asm_dialect dialect)
{
	// const char *sst_reg = NULL;      // state struct: not currently used
	const char *stp_reg = "rdi";  // string pointer
	const char *stn_reg = "rsi";  // string length
	const char *chr_reg = "edx";  // char register

	const char *ret_reg = "eax";

	const char *label_dot = ".";

	const struct ir_state *curr_st = NULL;
	const struct dfavm_op_ir *op;

	char *comment;

	assert(f != NULL);
	assert(funcname != NULL);
	assert(ir != NULL);
	assert(opt != NULL);
	assert(a != NULL);

	assert(ir->n > 0);

	switch (dialect) {
	case AMD64_ATT:  comment = "#"; break;
	case AMD64_NASM: comment = ";"; break;
	case AMD64_GO:  comment = "//";
			stp_reg = "DI";
			stn_reg = "SI";
			chr_reg = "DX";
			ret_reg = "AX";
			label_dot = "";
			break;
	default:
		assert(!"should not be reachable");
	}

	/* print preamble */
	switch (dialect) {
	case AMD64_ATT:
		fprintf(f, ".globl _%s\n", funcname);
		fprintf(f, ".text\n");
		fprintf(f, "_%s:\n", funcname);
		break;

	case AMD64_NASM:
		fprintf(f, "section .text\n");
		fprintf(f, "global _%s\n", funcname);
		fprintf(f, "_%s:\n", funcname);
		break;

	case AMD64_GO:
		/* need to determine if we're doing []byte or str */
		fprintf(f, "#include \"textflag.h\"\n");
		fprintf(f, "\n");
		switch (opt->io) {
		case FSM_IO_STR:
			fprintf(f, "// func Match(data string) int\n");
			fprintf(f, "TEXT    ·Match(SB), NOSPLIT, $0-24\n");
			break;
		case FSM_IO_PAIR:
			fprintf(f, "// func Match(data []byte) int\n");
			fprintf(f, "TEXT    ·Match(SB), NOSPLIT, $0-32\n");
			break;

		default:
			assert(! "unreached");

		}

		/* Go calling convention has everything on the stack */
		fprintf(f, "\tMOVQ data_base+0(FP), DI\n");
		fprintf(f, "\tMOVQ data_len+8(FP), SI\n");

	}

	/* XXX: add trampoline into previous state
	 *
	 * Need state struct for this.
	 */

	/* stn_reg += stp_reg <-- stn_reg should hold the end pointer */
	switch (dialect) {
	case AMD64_ATT:
		fprintf(f, "\taddq    %%%s, %%%s\n\n", stp_reg, stn_reg);
		break;

	case AMD64_NASM:
		/* stn_reg += stp_reg <-- stn_reg should hold the end pointer */
		fprintf(f, "\tADD   %s, %s\n\n", stn_reg, stp_reg);
		break;

	case AMD64_GO:
		/* stn_reg += stp_reg <-- stn_reg should hold the end pointer */
		fprintf(f, "\tADDQ   %s, %s\n\n", stp_reg, stn_reg);
	}

	for (op = a->linked; op != NULL; op = op->next) {
		if (curr_st != op->ir_state) {
			unsigned state = op->ir_state - ir->states;
			if (op->num_incoming > 0) {
				fprintf(f, "%sstate_%u:\n", label_dot, state);
			} else {
				fprintf(f, "%s state %u\n", comment, state);
			}

			curr_st = op->ir_state;
		}

		switch (op->instr) {
		case VM_OP_STOP:
			{
				const char *jmp_op;

				if (op->u.stop.end_bits == VM_END_SUCC) {
					unsigned end_st = op->ir_state - ir->states;
					switch (dialect) {
					case AMD64_ATT:
						fprintf(f, "\tmovl    $0x%02x, %%%s\n", end_st, ret_reg);
						break;
					case AMD64_NASM:
						fprintf(f, "\tMOV   %s, %02xh\n", ret_reg, end_st);
						break;
					case AMD64_GO:
						fprintf(f, "\tMOVQ   $%02x, %s\n", end_st, ret_reg);
						break;

					}
				} else {
					switch (dialect) {
					case AMD64_ATT:
						fprintf(f, "\tmovl    $-1, %%%s\n", ret_reg);
						break;
					case AMD64_NASM:
						fprintf(f, "\tMOV   %s, -1\n", ret_reg);
						break;
					case AMD64_GO:
						fprintf(f, "\tMOVQ    $-1, %s\n", ret_reg);
						break;
					}
				}

				if (op->cmp == VM_CMP_ALWAYS && op->next == NULL) {
					fprintf(f, "\t%s elided jmp to %sfinish\n", comment, label_dot);
				} else {
					if (op->cmp != VM_CMP_ALWAYS) {
						switch (dialect) {
						case AMD64_ATT:
							fprintf(f, "\tcmpl    $0x%02x,%%%s\n", (unsigned)op->cmp_arg, chr_reg);
							break;
						case AMD64_NASM:
							fprintf(f, "\tCMP   %s,%02xh\n", chr_reg, (unsigned)op->cmp_arg);
							break;
						case AMD64_GO:
							fprintf(f, "\tCMPL   %s, $0x%02x\n", chr_reg, (unsigned)op->cmp_arg);
							break;
						}
					}

					switch (op->cmp) {
					case VM_CMP_ALWAYS: jmp_op = (dialect == AMD64_ATT) ? "jmp    " : "JMP  "; break;
					case VM_CMP_LT:     jmp_op = (dialect == AMD64_ATT) ? "jb     " : "JB   "; break;
					case VM_CMP_LE:     jmp_op = (dialect == AMD64_ATT) ? "jbe    " : "JBE  "; break;
					case VM_CMP_GE:     jmp_op = (dialect == AMD64_ATT) ? "jae    " : "JAE  "; break;
					case VM_CMP_GT:     jmp_op = (dialect == AMD64_ATT) ? "ja     " : "JA   "; break;
					case VM_CMP_EQ:     jmp_op = (dialect == AMD64_ATT) ? "je     " : "JE   "; break;
					case VM_CMP_NE:     jmp_op = (dialect == AMD64_ATT) ? "jne    " : "JNE  "; break;
					}

					fprintf(f, "\t%-3s %sfinish\n", jmp_op, label_dot);
				}
			}
			break;

		case VM_OP_FETCH:
			{
				if (op->u.fetch.end_bits == VM_END_SUCC) {
					unsigned end_st = op->ir_state - ir->states;

					switch (dialect) {
					case AMD64_ATT:
						fprintf(f, "\tmovl    $0x%02x,%%%s\n", (unsigned)end_st, ret_reg);
						break;

					case AMD64_NASM:
						fprintf(f, "\tMOV   %s, %02xh\n", ret_reg, end_st);
						break;

					case AMD64_GO:
						fprintf(f, "\tMOVL   $0x%02x, %s\n", end_st, ret_reg);
						break;

					}
				} else {

					switch (dialect) {
					case AMD64_ATT:
						fprintf(f, "\tmovl    $-1, %%%s\n", ret_reg);
						break;

					case AMD64_NASM:
						fprintf(f, "\tMOV   %s, -1\n", ret_reg);
						break;

					case AMD64_GO:
						fprintf(f, "\tMOVQ  $-1, %s\n", ret_reg);
						break;
					}
				}


				switch (dialect) {
				case AMD64_ATT:
					fprintf(f, "\tcmpq    %%%s,%%%s\n", stp_reg, stn_reg);
					fprintf(f, "\tje      .finish\n");
					fprintf(f, "\tmovzbl  (%%%s), %%%s\n", stp_reg, chr_reg);
					fprintf(f, "\taddq    $1, %%%s\n", stp_reg);
					break;

				case AMD64_NASM:
					fprintf(f, "\tCMP   %s, %s\n", stp_reg, stn_reg);
					fprintf(f, "\tJE    .finish\n");
					fprintf(f, "\tMOVZX %s, BYTE [%s]\n", chr_reg, stp_reg);
					fprintf(f, "\tADD   %s, 1\n", stp_reg);
					break;

				case AMD64_GO:
					fprintf(f, "\tCMPQ    %s,%s\n", stp_reg, stn_reg);
					fprintf(f, "\tJE      finish\n");
					fprintf(f, "\tMOVBLZX (%s), %s\n", stp_reg, chr_reg);
					fprintf(f, "\tADDQ    $1, %s\n", stp_reg);
					break;

				}
			}
			break;

		case VM_OP_BRANCH:
			{
				const char *jmp_op;
				char jlbl[64];
				unsigned dest_state = op->u.br.dest_arg->ir_state - ir->states;

				if (op->cmp != VM_CMP_ALWAYS) {
					switch (dialect) {
					case AMD64_ATT:
						fprintf(f, "\tcmpl    $0x%02x, %%%s\n", (unsigned)op->cmp_arg, chr_reg);
						break;

					case AMD64_NASM:
						fprintf(f, "\tCMP   %s, %02xh\n", chr_reg, (unsigned)op->cmp_arg);
						break;

					case AMD64_GO:
						fprintf(f, "\tCMPL    %s, $0x%02x\n", chr_reg, (unsigned)op->cmp_arg);
						break;
					}
				}

				snprintf(jlbl, sizeof jlbl, "%sstate_%u", label_dot, dest_state);

				switch (op->cmp) {
				case VM_CMP_ALWAYS: jmp_op = (dialect == AMD64_ATT) ? "jmp    " : "JMP  "; break;
				case VM_CMP_LT:     jmp_op = (dialect == AMD64_ATT) ? "jb     " : "JB   "; break;
				case VM_CMP_LE:     jmp_op = (dialect == AMD64_ATT) ? "jbe    " : "JBE  "; break;
				case VM_CMP_GE:     jmp_op = (dialect == AMD64_ATT) ? "jae    " : "JAE  "; break;
				case VM_CMP_GT:     jmp_op = (dialect == AMD64_ATT) ? "ja     " : "JA   "; break;
				case VM_CMP_EQ:     jmp_op = (dialect == AMD64_ATT) ? "je     " : "JE   "; break;
				case VM_CMP_NE:     jmp_op = (dialect == AMD64_ATT) ? "jne    " : "JNE  "; break;
				}

				fprintf(f, "\t%-3s %s\n", jmp_op, jlbl);
			}
			break;

		default:
			assert(!"unreached");
			break;
		}

		fprintf(f, "\n");
	}

	fprintf(f, "%sfinish:\n", label_dot);


	switch (dialect) {
	case AMD64_ATT:
		fprintf(f, "\tret\n");
		break;

	case AMD64_NASM:
		fprintf(f, "\tRET\n");
		break;

	case AMD64_GO:
		switch (opt->io) {
		case FSM_IO_STR:
			fprintf(f, "\tMOVQ    %s, ret+16(FP)\n", ret_reg);
			break;
		case FSM_IO_PAIR:
			fprintf(f, "\tMOVQ    %s, ret+24(FP)\n", ret_reg);
			break;
		default: assert(! "unreached");
		}

		fprintf(f, "\tRET\n");
		break;
	}

	return 0;
}

static void
print_vmasm_encoding(FILE *f, const struct fsm *fsm, enum asm_dialect dialect)
{
	struct ir *ir;
	const char *prefix;
	char funcname[256];

	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;

	static const struct fsm_vm_compile_opts vm_opts = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	assert(f != NULL);
	assert(ir != NULL);

	a = zero;

	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		free_ir(fsm, ir);
		return;
	}

	/* henceforth, no function should be passed struct fsm *, only the ir and options */

	if (fsm->opt->prefix != NULL) {
		prefix = fsm->opt->prefix;
	} else {
		prefix = "fsm_";
	}

	if (dialect == AMD64_GO) {
		snprintf(funcname, sizeof funcname, "Match");
	} else {
		snprintf(funcname, sizeof funcname, "%smatch", prefix);
	}

	print_asm_amd64(f, funcname, ir, fsm->opt, &a, dialect);

	dfavm_opasm_finalize_op(&a);
	free_ir(fsm, ir);
}

static const enum asm_dialect default_dialect = AMD64_NASM;

void
fsm_print_vmasm(FILE *f, const struct fsm *fsm)
{
	print_vmasm_encoding(f, fsm, default_dialect);
}

void
fsm_print_vmasm_amd64_att(FILE *f, const struct fsm *fsm)
{
	print_vmasm_encoding(f, fsm, AMD64_ATT);
}

void
fsm_print_vmasm_amd64_nasm(FILE *f, const struct fsm *fsm)
{
	print_vmasm_encoding(f, fsm, AMD64_NASM);
}

void
fsm_print_vmasm_amd64_go(FILE *f, const struct fsm *fsm)
{
	print_vmasm_encoding(f, fsm, AMD64_GO);
}
