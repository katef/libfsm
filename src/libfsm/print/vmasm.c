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
#include "libfsm/print.h"

#include "libfsm/vm/retlist.h"
#include "libfsm/vm/vm.h"

enum asm_dialect {
	AMD64_ATT,
	AMD64_NASM,
	AMD64_GO,
};

static int
print_end(FILE *f, const struct dfavm_op_ir *op,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	enum asm_dialect dialect, const char *ret_reg,
	enum dfavm_op_end end_bits)
{
	bool r;

	switch (end_bits) {
	case VM_END_FAIL:
		if (-1 == print_hook_reject(f, opt, hooks, NULL, NULL)) {
			return -1;
		}
		break;

	case VM_END_SUCC:;
		const struct fsm_state_metadata state_metadata = {
			.end_ids = op->ret->ids,
			.end_id_count = op->ret->count,
		};
		if (-1 == print_hook_accept(f, opt, hooks,
			&state_metadata,
			NULL, NULL))
		{
			return -1;
		}
		if (-1 == print_hook_comment(f, opt, hooks,
			&state_metadata))
		{
			return -1;
		}
		break;

	default:
		assert(!"unreached");
		abort();
	}

	if (opt->ambig != AMBIG_NONE) {
		errno = ENOTSUP;
		return -1;
	}

	r = end_bits == VM_END_SUCC;

	switch (dialect) {
	case AMD64_ATT:
		fprintf(f, "\tmovl	$%d, %%%s\n", r, ret_reg);
		break;
	case AMD64_NASM:
		fprintf(f, "\tMOV   %s, %d\n", ret_reg, r);
		break;
	case AMD64_GO:
		fprintf(f, "\tMOVQ   $%d, %s\n", r, ret_reg);
		break;
	}
	
	return 0;
}

static int
print_asm_amd64(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	struct dfavm_op_ir *ops,
	const char *prefix,
	enum asm_dialect dialect)
{
	// const char *sst_reg = NULL;      // state struct: not currently used
	const char *stp_reg = "rdi";  // string pointer
	const char *stn_reg = "rsi";  // string length
	const char *chr_reg = "edx";  // char register

	const char *ret_reg = "eax";

	const char *label_dot = ".";

	const struct dfavm_op_ir *op;

	char *comment;
	const char *sigil;

#if defined(__MACH__)
	sigil = "_";
#else
	sigil = "";
#endif

	assert(f != NULL);
	assert(opt != NULL);

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
		errno = ENOTSUP;
		return -1;
	}

	/* print preamble */
	switch (dialect) {
	case AMD64_ATT:
		fprintf(f, "%s generated\n", comment);
		fprintf(f, ".globl %s%s%s\n", sigil, prefix, "match");
		fprintf(f, ".text\n");
		fprintf(f, "%s%s%s:\n", sigil, prefix, "match");
		break;

	case AMD64_NASM:
		fprintf(f, "%s generated\n", comment);
		fprintf(f, "section .text\n");
		fprintf(f, "global %s%s%s\n", sigil, prefix, "match");
		fprintf(f, "%s%s%s:\n", sigil, prefix, "match");
		break;

	case AMD64_GO:
		/* need to determine if we're doing []byte or str */
		fprintf(f, "#include \"textflag.h\"\n");
		fprintf(f, "\n");

		fprintf(f, "%s generated\n", comment);
		switch (opt->io) {
		case FSM_IO_STR:
			if (opt->comments) {
				fprintf(f, "// func %s%s(data string) int\n", prefix, "Match");
			}
			fprintf(f, "TEXT    ·%s(SB), NOSPLIT, $0-24\n", "Match");
			break;

		case FSM_IO_PAIR:
			if (opt->comments) {
				fprintf(f, "// func %s%s(data []byte) int\n", prefix, "Match");
			}
			fprintf(f, "TEXT    ·%s%s(SB), NOSPLIT, $0-32\n", prefix, "Match");
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

	for (op = ops; op != NULL; op = op->next) {
		if (op->num_incoming > 0) {
			fprintf(f, "%sl%u:\n", label_dot, op->index);

			// TODO: example
		} else if (opt->comments) {
			fprintf(f, "%s l%u\n", comment, op->index);
		}

		switch (op->instr) {
		case VM_OP_STOP:
			{
				if (-1 == print_end(f, op, opt, hooks,
					dialect, ret_reg,
					op->u.stop.end_bits))
				{
					return -1;
				}

				if (op->cmp == VM_CMP_ALWAYS && op->next == NULL) {
					if (opt->comments) {
						fprintf(f, "\t%s elided jmp to %sfinish\n", comment, label_dot);
					}
				} else {
					const char *jmp_op;

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
				if (-1 == print_end(f, op, opt, hooks,
					dialect, ret_reg,
					op->u.fetch.end_bits))
				{
					return -1;
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

				switch (op->cmp) {
				case VM_CMP_ALWAYS: jmp_op = (dialect == AMD64_ATT) ? "jmp    " : "JMP  "; break;
				case VM_CMP_LT:     jmp_op = (dialect == AMD64_ATT) ? "jb     " : "JB   "; break;
				case VM_CMP_LE:     jmp_op = (dialect == AMD64_ATT) ? "jbe    " : "JBE  "; break;
				case VM_CMP_GE:     jmp_op = (dialect == AMD64_ATT) ? "jae    " : "JAE  "; break;
				case VM_CMP_GT:     jmp_op = (dialect == AMD64_ATT) ? "ja     " : "JA   "; break;
				case VM_CMP_EQ:     jmp_op = (dialect == AMD64_ATT) ? "je     " : "JE   "; break;
				case VM_CMP_NE:     jmp_op = (dialect == AMD64_ATT) ? "jne    " : "JNE  "; break;
				}

				fprintf(f, "\t%-3s %sl%u\n", jmp_op, label_dot, op->u.br.dest_arg->index);
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

		default:
			errno = ENOTSUP;
			return -1;
		}

		fprintf(f, "\tRET\n");
		break;
	}

	return 0;
}

static int
print_vmasm_encoding(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops,
	enum asm_dialect dialect)
{
	const char *prefix;

	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);
	assert(retlist != NULL);

	if (dialect == AMD64_GO) {
		if (opt->io != FSM_IO_STR && opt->io != FSM_IO_PAIR) {
			errno = ENOTSUP;
			return -1;   
		}
	} else {
		if (opt->io != FSM_IO_PAIR) {
			errno = ENOTSUP;
			return -1;   
		}
	}

	if (opt->prefix != NULL) {
		prefix = opt->prefix;
	} else {
		prefix = "fsm_";
	}

	return print_asm_amd64(f, opt, hooks, ops, prefix, dialect);
}

int
fsm_print_amd64_att(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	return print_vmasm_encoding(f, opt, hooks, retlist, ops, AMD64_ATT);
}

int
fsm_print_amd64_nasm(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	return print_vmasm_encoding(f, opt, hooks, retlist, ops, AMD64_NASM);
}

int
fsm_print_amd64_go(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ret_list *retlist,
	struct dfavm_op_ir *ops)
{
	return print_vmasm_encoding(f, opt, hooks, retlist, ops, AMD64_GO);
}

