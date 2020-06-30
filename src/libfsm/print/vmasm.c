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
print_asm_amd64(FILE *f, const char *funcname, const struct ir *ir, const struct dfavm_assembler_ir *a)
{
	// const char *sst_reg = NULL;      // state struct: not currently used
	const char *stp_reg = "rdi";  // string pointer
	const char *stn_reg = "rsi";  // string length
	const char *chr_reg = "edx";  // char register

	const char *ret_reg = "rax";

	struct dfavm_op_ir *op;

	assert(f != NULL);
	assert(funcname != NULL);
	assert(ir != NULL);
	assert(a != NULL);

	assert(ir->n > 0);

	/* print preamble */
	fprintf(f, "section .text\n");
	fprintf(f, "global _%s\n", funcname);
	fprintf(f, "_%s:\n", funcname);

	/* XXX: add trampoline into previous state
	 *
	 * Need state struct for this.
	 */

	/* stn_reg += stp_reg <-- stn_reg should hold the end pointer */
	fprintf(f, "\tADD %s, %s\n\n", stn_reg, stp_reg);

	for (op = a->linked; op != NULL; op = op->next) {
		switch (op->instr) {
		case VM_OP_STOP:
			{
				const char *jmp_op;

				if (op->u.stop.end_bits == VM_END_SUCC) {
					unsigned end_st = op->ir_state - ir->states;
					fprintf(f, "\tMOV %s, %02xh\n", ret_reg, end_st);
				} else {
					fprintf(f, "\tMOV %s, -1\n", ret_reg);
				}

				if (op->cmp != VM_CMP_ALWAYS) {
					fprintf(f, "\tCMP %s,%02xh\n", chr_reg, (unsigned)op->cmp_arg);
				}

				switch (op->cmp) {
					case VM_CMP_ALWAYS: jmp_op = "JMP"; break;
					case VM_CMP_LT:     jmp_op = "JB";  break;
					case VM_CMP_LE:     jmp_op = "JBE"; break;
					case VM_CMP_GE:     jmp_op = "JAE"; break;
					case VM_CMP_GT:     jmp_op = "JA";  break;
					case VM_CMP_EQ:     jmp_op = "JE";  break;
					case VM_CMP_NE:     jmp_op = "JNE"; break;
				}

				fprintf(f, "\t%-3s .finish\n", jmp_op);
			}
			break;

		case VM_OP_FETCH:
			{
				if (op->num_incoming > 0) {
					fprintf(f, ".state_%u\n", op->u.fetch.state);
				}

				if (op->u.fetch.end_bits == VM_END_SUCC) {
					unsigned end_st = op->ir_state - ir->states;
					fprintf(f, "\tMOV %s, %02xh\n", ret_reg, end_st);
				} else {
					fprintf(f, "\tMOV %s, -1\n", ret_reg);
				}

				fprintf(f, "\tCMP %s,%s\n", stp_reg, stn_reg);
				fprintf(f, "\tJE  .finish\n");
				fprintf(f, "\tMOVZX %s, BYTE PTR[%s]\n", chr_reg, stp_reg);
				fprintf(f, "\tADD %s, 1\n", stp_reg);
			}
			break;

		case VM_OP_BRANCH:
			{
				const char *jmp_op;
				char jlbl[64];

				if (op->cmp != VM_CMP_ALWAYS) {
					fprintf(f, "\tCMP %s,%02xh\n", chr_reg, (unsigned)op->cmp_arg);
				}

				snprintf(jlbl, sizeof jlbl, ".state_%u", op->u.br.dest_state);

				switch (op->cmp) {
					case VM_CMP_ALWAYS: jmp_op = "JMP"; break;
					case VM_CMP_LT:     jmp_op = "JB";  break;
					case VM_CMP_LE:     jmp_op = "JBE"; break;
					case VM_CMP_GE:     jmp_op = "JAE"; break;
					case VM_CMP_GT:     jmp_op = "JA";  break;
					case VM_CMP_EQ:     jmp_op = "JE";  break;
					case VM_CMP_NE:     jmp_op = "JNE"; break;
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

	fprintf(f, ".finish:\n");
	fprintf(f, "\tRET\n");

	return 0;
}

void
fsm_print_vmasm(FILE *f, const struct fsm *fsm)
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

	snprintf(funcname, sizeof funcname, "%smatch", prefix);

	print_asm_amd64(f, funcname, ir, &a);

	dfavm_opasm_finalize_op(&a);
	free_ir(fsm, ir);
}

