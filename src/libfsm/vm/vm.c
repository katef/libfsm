/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#include <fsm/fsm.h>
#include <fsm/vm.h>

#include "libfsm/internal.h"

#include "vm.h"

//
// Fixed encoding VM state:
//
//   The VM address buffer holds addresses for indirect branches.
//
// VM bytecodes:
//
//   There are four instructions:
//     BRANCH, IBRANCH, FETCH, and STOP.
//
//   Each instruction is 32-bits, encoded as follows:
//
//                1098 765 4   32109876  5432 1098 7654 3210
//                3322 222 2   22221111  1111 1100 0000 0000
//
//     STOP       0000 CCC R   AAAAAAAA  0000 0000 0000 0000
//     FETCH      0001 000 R   00000000  0000 0000 0000 0000
//     BRANCH     0010 CCC 0   AAAAAAAA  DDDD DDDD DDDD DDDD
//     IBRANCH    0011 CCC 0   AAAAAAAA  IIII IIII IIII IIII
//
//                IIII CCC R
//
//     R  = 0 fail / R = 1 succeed
//
//     C = comparison bit
//     A = argument bit
//     D = relative destination bit
//     I = index bit
//
//   BRANCH and IBRANCH are both ways to implement the BRANCH opcode.  The
//   difference between them is how the address is determined.
//
//     BRANCH  address field is a 16-bit signed relative offset
//     IBRANCH address field is a 16-bit index into the address table,
//             which holds a 32-bit unsigned value stored in the address buffer.
//             The argument is the address buffer entry.
//
// Instructions are encoded into 4 bytes.  The first byte holds the instruction
// type, the condition (if applicable) and the success argument (if applicable).
// The remaining three bytes depend on the instruction:
//
// BRANCH : 8-bit arg, 16-bit signed relative dest
// STOP   : 8-bit arg, rest unused
// FETCH  : no args, rest unused
//
// unused bits should be zeroed to keep bytecode forward-compatible.
//
//
// Op codes:
//
//            bits
// BRANCH     0000CCC0
// IBRANCH    0001CCC0
// STOP       0010CCCR  R  = 0 fail / R = 1 succeed
// FETCH      0011000R  R  = 0 fail / R = 1 succeed
//
//
//            bits
// BRANCH     FR000CCC
// IBRANCH    FR001CCC
// STOP       FR01XCCC  X  = 0 fail / X = 1 succeed
//
//   NB: in the future, 'always' comparisons may use their argument byte
//       for other purposes.
//
// Argument 
//   ARG is an 8-bit unsigned byte
//
// BRANCH - compare and branch
//   
//   If the comparison between ARG and CB is true Sets the PC based on the relative
//
// FETCH - fetch instructions
//
//   Fetch instructions fetch the next character from the stream and have a stop
//   bit that indicates success/failure if the stream has no more characters.
//
//   E = 0 indicates FETCHF, failure if EOS
//   E = 1 indicates FETCHS, success if EOS
//
//   FETCHS/F instructions should currently have comparison bits set to 00
//   (always).  A future version reserves the right to support conditional
//   FETCH.
//
//   (NB: the comparison bits are currently just ignored for FETCH instructions)
//
// STOP - stop instructions
//
// Stop instructions have an end bit: 
//     E = 0 indicates STOPF, stop w/ failure
//     E = 1 indicates STOPS, stop w/ success
//
//    STOPF generally indicates that there is no transition from the current
//         state matching the read character
//
//    STOPS generally indicates that the current state is an end state, is
//         complete, and all edges point back to the current state.
//
// BR - branch instructions
//
//   Branch instructions always have a signed relative destination argument.
//   The D bits specify its length:
//
//   DD
//   00    8-bit destination, 1-byte destination argument
//   01   16-bit destination, 2-byte destination argument
//   10   32-bit destination, 4-byte destination argument, LSB ignored
//
//   DD=11 is reserved for future use
//

struct dfavm_assembler_vm {
	struct dfavm_vm_op *instr;
	size_t ninstr;

	uint32_t nbytes;
};

static void
print_vm_op(FILE *f, struct dfavm_vm_op *op)
{
	const char *cmp;
	int nargs = 0;

	cmp = cmp_name(op->cmp);

	fprintf(f, "[%4" PRIu32 "] %1u\t", op->offset, op->num_encoded_bytes);

	switch (op->instr) {
	case VM_OP_FETCH:
		fprintf(f, "FETCH%c%s",
			(op->u.fetch.end_bits == VM_END_FAIL) ? 'F' : 'S',
			cmp);
		break;

	case VM_OP_STOP:
		fprintf(f, "STOP%c%s",
			(op->u.stop.end_bits == VM_END_FAIL) ? 'F' : 'S',
			cmp);
		break;

	case VM_OP_BRANCH:
		{
			char dst;
			switch (op->u.br.dest) {
			case VM_DEST_NONE:  dst = 'Z'; break;
			case VM_DEST_SHORT: dst = 'S'; break;
			case VM_DEST_NEAR:  dst = 'N'; break;
			case VM_DEST_FAR:   dst = 'F'; break;
			default:            dst = '!'; break;
			}

			fprintf(f, "BR%c%s", dst, cmp);
		}
		break;

	default:
		fprintf(f, "UNK_%d_%s", (int)op->instr, cmp);
	}

	if (op->cmp != VM_CMP_ALWAYS) {
		if (isprint(op->cmp_arg)) {
			fprintf(f, " '%c'", op->cmp_arg);
		} else {
			fprintf(f, " 0x%02x",(unsigned)op->cmp_arg);
		}

		nargs++;
	}

	if (op->instr == VM_OP_BRANCH) {
		fprintf(f, "%s%ld [dst_ind=%" PRIu32 "]", ((nargs>0) ? ", " : " "),
			(long)op->u.br.rel_dest, op->u.br.dest_index);
		nargs++;
	}

	fprintf(f, "\t; %6u bytes",
		op->num_encoded_bytes);
	switch (op->instr) {
	case VM_OP_FETCH:
		fprintf(f, "  [state %u]", op->u.fetch.state);
		break;

	case VM_OP_BRANCH:
		// fprintf(f, "  [dest=%p]", (void *)op->u.br.dest_arg);
		break;

	default:
		break;
	}

	fprintf(f, "\n");
}

static void
print_vm_instr(FILE *f, const struct dfavm_assembler_vm *a)
{
	size_t i;
	for (i=0; i < a->ninstr; i++) {
		fprintf(f, "%6zu | ", i);
		print_vm_op(f, &a->instr[i]);
	}
}

static int
op_encoding_size(struct dfavm_vm_op *op, int max_enc)
{
	int nbytes = 1;

	static const long min_dest_1b = INT8_MIN;
	static const long max_dest_1b = INT8_MAX;

	static const long min_dest_2b = INT16_MIN;
	static const long max_dest_2b = INT16_MAX;

//	static const long min_dest_4b = INT32_MIN;
//	static const long max_dest_4b = INT32_MAX;

	assert(op != NULL);

	if (op->cmp != VM_CMP_ALWAYS) {
		nbytes++;
	}

	if (op->instr == VM_OP_BRANCH) {
		int32_t rel_dest = op->u.br.rel_dest;
		if (!max_enc && rel_dest >= min_dest_1b && rel_dest <= max_dest_1b) {
			nbytes += 1;
			op->u.br.dest = VM_DEST_SHORT;
		}
		else if (!max_enc && rel_dest >= min_dest_2b && rel_dest <= max_dest_2b) {
			nbytes += 2;
			op->u.br.dest = VM_DEST_NEAR;
		}
		else {
			// need 32 bits
			nbytes += 4;
			op->u.br.dest = VM_DEST_FAR;
		}
	}

	return nbytes;
}

static void
build_vm_op(const struct dfavm_op_ir *ir, struct dfavm_vm_op *op)
{
	static const struct dfavm_vm_op zero;

	*op = zero;
	op->ir = ir;
	op->instr   = ir->instr;
	op->cmp     = ir->cmp;
	op->cmp_arg = ir->cmp_arg;

	switch (op->instr) {
	case VM_OP_STOP:
		op->u.stop.end_bits = ir->u.stop.end_bits;
		break;

	case VM_OP_FETCH:
		op->u.fetch.state    = ir->u.fetch.state;
		op->u.fetch.end_bits = ir->u.fetch.end_bits;
		break;

	case VM_OP_BRANCH:
		assert(ir->u.br.dest_arg != NULL);

		op->u.br.dest_index = ir->u.br.dest_arg->index;

		/* zero initialize the other fields */
		op->u.br.dest = VM_DEST_NONE;
		op->u.br.rel_dest = 0;

		break;
	}
}

static struct dfavm_vm_op *
build_vm_op_array(const struct dfavm_op_ir *ops, size_t *np)
{
	const struct dfavm_op_ir *op;

	struct dfavm_vm_op *ilist;
	size_t i,ninstr;

	assert(np != NULL);

	/* count number of instructions */
	for (op = ops, ninstr=0; op != NULL; op = op->next) {
		ninstr++;
	}

	errno = 0;

	if (ninstr == 0) {
		*np = 0;
		return NULL;
	}

	ilist = calloc(ninstr, sizeof *ilist);
	if (ilist == NULL) {
		return NULL;
	}

	for (op = ops, i=0; op != NULL; op = op->next, i++) {
		assert(i < ninstr);

		build_vm_op(op, &ilist[i]);
	}

	*np = ninstr;
	return ilist;
}

static void
assign_rel_dests(struct dfavm_assembler_vm *a)
{
	uint32_t off;
	size_t i;

	assert(a != NULL);
	assert(a->instr != NULL);

	/* start with maximum branch encoding */
	off = 0;
	for (i=0; i < a->ninstr; i++) {
		struct dfavm_vm_op *op;
		int nenc;

		op = &a->instr[i];

		nenc = op_encoding_size(op, 1);

		assert(nenc > 0 && nenc <= 6);

		op->offset = off;
		op->num_encoded_bytes = nenc;
		off += nenc;
	}

	a->nbytes = off;

	/* iterate until we converge */
	for (;;) {
		int nchanged = 0;
		size_t i;

		for (i=0; i < a->ninstr; i++) {
			struct dfavm_vm_op *op;

			op = &a->instr[i];

			if (op->instr == VM_OP_BRANCH) {
				struct dfavm_vm_op *dest;
				int64_t diff;
				int nenc;

				assert(op->u.br.dest_index < a->ninstr);
				dest = &a->instr[op->u.br.dest_index];

				diff = (int64_t)dest->offset - (int64_t)op->offset;

				assert(diff >= INT32_MIN && diff <= INT32_MAX);

				op->u.br.rel_dest = diff;

				nenc = op_encoding_size(op, 0);
				if (nenc != op->num_encoded_bytes) {
					op->num_encoded_bytes = nenc;
					nchanged++;
				}
			}
		}

		if (nchanged == 0) {
			break;
		}

		/* adjust offsets */
		off = 0;
		for (i=0; i < a->ninstr; i++) {
			struct dfavm_vm_op *op;

			op = &a->instr[i];
			op->offset = off;
			off += op->num_encoded_bytes;
		}

		a->nbytes = off;
	}
}

struct fsm_dfavm *
dfavm_compile_vm(const struct dfavm_assembler_ir *a, struct fsm_vm_compile_opts opts)
{
	static const struct dfavm_assembler_vm zero;
	struct dfavm_assembler_vm b;
	struct fsm_dfavm *vm;

	b = zero;

	(void)print_vm_instr; /* make clang happy */

	/* build vm instructions */
	b.instr = build_vm_op_array(a->linked, &b.ninstr);
	if (b.instr == NULL) {
		goto error;
	}

	assign_rel_dests(&b);

#if DEBUG_VM_OPCODES
	dump_states(stdout, a);
	fprintf(f, "%6" PRIu32 " total bytes\n", b.nbytes);
#endif /* DEBUG_VM_OPCODES */

	if (opts.flags & FSM_VM_COMPILE_PRINT_ENC) {
		FILE *f = (opts.log != NULL) ? opts.log : stdout;

		fprintf(f,"---[ vm instructions ]---\n");
		print_vm_instr(f, &b);
		fprintf(f, "\n");
	}

	/* XXX: better error handling */
	vm = NULL;
	errno = EINVAL;

	switch (opts.output) {
	case FSM_VM_COMPILE_VM_V1:
		vm = encode_opasm_v1(b.instr, b.ninstr, b.nbytes);
		break;

	case FSM_VM_COMPILE_VM_V2:
		vm = encode_opasm_v2(b.instr, b.ninstr);
		break;
	}

	if (vm == NULL) {
		goto error;
	}

	free(b.instr);

	return vm;

error:

	free(b.instr);

	return NULL;
}

void
dfavm_free_vm(struct fsm_dfavm *vm)
{
	if (vm == NULL) {
		return;
	}

	if (vm->version_major == DFAVM_VARENC_MAJOR && vm->version_minor == DFAVM_VARENC_MINOR) {
		dfavm_v1_finalize(&vm->u.v1);
	} else if (vm->version_major == DFAVM_FIXEDENC_MAJOR && vm->version_minor == DFAVM_FIXEDENC_MINOR) {
		dfavm_v2_finalize(&vm->u.v2);
	} else {
		/* invalid VM version! */
		assert(false && "unsupported version passed in");
	}

	free(vm);
}

