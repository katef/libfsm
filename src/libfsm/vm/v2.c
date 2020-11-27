/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "vm.h"

enum dfavm_vm_op_v2 {
	// Stop the VM, mark match or failure
	VM_V2_OP_STOP    = 0,

	// Start of each state: fetch next character.  Indicates
	// match / fail if EOS.
	VM_V2_OP_FETCH   = 1,

	// Branch to another state
	VM_V2_OP_BRANCH  = 2,
	VM_V2_OP_IBRANCH = 3,
};

struct fsm_dfavm *
encode_opasm_v2(const struct dfavm_vm_op *instr, size_t ninstr)
{
	static const struct fsm_dfavm zero;

	struct fsm_dfavm *ret;
	struct dfavm_v2 *vm;
	size_t i;
	uint32_t *enc;
	uint32_t *abuf;
	uint32_t alen,acap;

	ret = malloc(sizeof *ret);
	*ret = zero;

	ret->version_major = DFAVM_FIXEDENC_MAJOR;
	ret->version_minor = DFAVM_FIXEDENC_MINOR;

	vm = &ret->u.v2;

	enc = malloc(ninstr * sizeof *enc);

	vm->ops = enc;
	vm->len = ninstr;

	abuf = NULL;
	alen = 0;
	acap = 0;

	for (i = 0; i < ninstr; i++) {
		unsigned char cmp_bits, instr_bits, result_bit, cmp_arg;
		const struct dfavm_vm_op *op = &instr[i];
		uint32_t instr;
		uint16_t dest_arg;

		switch (op->instr) {
		case VM_OP_STOP:
			instr_bits = VM_V2_OP_STOP;
			cmp_bits   = op->cmp;
			cmp_arg    = op->cmp_arg;
			result_bit = (op->u.stop.end_bits == VM_END_SUCC) ? 0x1 : 0x0;
			dest_arg   = 0;
			break;

		case VM_OP_FETCH:
			instr_bits = VM_V2_OP_FETCH;
			cmp_bits   = 0;
			cmp_arg    = 0;
			result_bit = (op->u.fetch.end_bits == VM_END_SUCC) ? 0x1 : 0x0;
			dest_arg   = 0;
			break;

		case VM_OP_BRANCH:
			{
				uint32_t dest_state = op->u.br.dest_index;
				int64_t state_diff = (int64_t)dest_state - (int64_t)i;

				if (state_diff >= INT16_MIN && state_diff <= INT16_MAX) {
					instr_bits = VM_V2_OP_BRANCH;
					dest_arg = (int16_t)state_diff;
				}
				else {
					/* allocate address in address table */
					instr_bits = VM_V2_OP_IBRANCH;

					if (alen >= acap) {
						size_t new_acap;
						uint32_t *new_abuf;

						if (acap < 16) {
							new_acap = 16;
						} else if (acap < 1024) {
							new_acap = acap * 2;
						} else {
							new_acap = acap + acap/2;
						}

						new_abuf = realloc(abuf, new_acap * sizeof abuf[0]);
						if (new_abuf == NULL) {
							goto error;
						}

						abuf = new_abuf;
						acap = new_acap;

						vm->abuf = new_abuf;
					}

					assert(alen < acap);
					assert(abuf != NULL);

					abuf[alen] = dest_state;
					dest_arg = alen++;
					vm->alen = alen;
				}

				cmp_bits   = op->cmp;
				cmp_arg    = op->cmp_arg;
				result_bit = 0;
			}
			break;
		}

		if (cmp_bits > 7) {
			goto error;
		}

		instr = (((uint32_t)instr_bits) << 28) | (((uint32_t)cmp_bits)<<25) | (((uint32_t)result_bit)<<24) |
			(((uint32_t)cmp_arg)<<16) | dest_arg;

#if DEBUG_ENCODING
		fprintf(stderr, "enc[%4zu] instr=0x%02x cmp=0x%02x result=0x%02x cmp_arg=0x%02x dest_arg=0x%04x (%d | %u) instr=0x%08lx\n",
			off, instr_bits, cmp_bits, result_bit, (unsigned)cmp_arg, (unsigned)dest_arg, (int16_t)dest_arg, (unsigned)dest_arg,
			(unsigned long)instr);
#endif /* DEBUG_ENCODING */

		enc[i] = instr;
	}

	return ret;

error:
	/* XXX - cleanup */
	return NULL;
}

#define V2DEC(instr,op,cmp,end,cmp_arg,dest_arg) do {  \
	uint32_t tmp ## __LINE__ = (instr);            \
	(op)       = (tmp ## __LINE__) >> 28;          \
	(cmp)      = ((tmp ## __LINE__) >> 25) & 0x7;  \
	(end)      = ((tmp ## __LINE__) >> 24) & 0x1;  \
	(cmp_arg)  = ((tmp ## __LINE__) >> 16) & 0xff; \
	(dest_arg) = (tmp ## __LINE__) & 0xffff;       \
} while(0)

void
running_print_op_v2(const struct dfavm_v2 *vm, uint32_t pc, const char *sp, const char *buf, size_t n, char ch, FILE *f)
{
	int op, cmp, end, arg, dest;

	V2DEC(vm->ops[pc], op,cmp,end, arg, dest);

	fprintf(f, "[%4lu sp=%zd n=%zu ch=%3u '%c' end=%d] ",
		(unsigned long)pc, sp-buf, n, (unsigned char)ch, isprint(ch) ? ch : ' ', end);

	switch (op) {
	case VM_V2_OP_FETCH:
		fprintf(f, "FETCH%s\n", (end == VM_END_FAIL) ? "F" : "S");
		break;

	case VM_V2_OP_STOP:
		fprintf(f, "STOP%s%s", (end == VM_END_FAIL) ? "F" : "S", cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			if (isprint(arg)) {
				fprintf(f, " '%c'", arg);
			} else {
				fprintf(f, " %d", arg);
			}
		}
		fprintf(f, "\n");
		break;

	case VM_V2_OP_BRANCH:
		fprintf(f, "BR%s", cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			if (isprint(arg)) {
				fprintf(f, " '%c',", arg);
			} else {
				fprintf(f, " %d,", arg);
			}
		}

		{
			union {
				uint16_t u;
				int16_t  rel;
			} packed;

			packed.u = dest;

			fprintf(f, " %d %u\n", (int)packed.rel, dest);
		}
		break;

	case VM_V2_OP_IBRANCH:
		fprintf(f, "IBR%s", cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			if (isprint(arg)) {
				fprintf(f, " '%c',", arg);
			} else {
				fprintf(f, " %d,", arg);
			}
		}

		{
			assert(dest >= 0 && (uint32_t)dest < vm->alen);
			uint32_t dest_addr = vm->abuf[dest];
			fprintf(f, " dest=%lu index=%d\n", (unsigned long)dest_addr, dest);
		}
		break;

	default:
		fprintf(f, "UNK[op=%d cmp=%d end=%d arg=%d dest=%d]\n", op,cmp,end,arg, dest);
		break;
	}
}

enum dfavm_state
vm_match_v2(const struct dfavm_v2 *vm, struct vm_state *st, const char *buf, size_t n)
{
	const char *sp, *last;
	int ch;

	if (st->state != VM_MATCHING) {
		return st->state;
	}

	ch   = 0;
	sp   = buf;
	last = buf + n;

	for (;;) {
		int op, cmp, end, arg, dest;

		assert(st->pc < vm->len);

		/* decode instruction */
		V2DEC(vm->ops[st->pc], op,cmp,end, arg, dest);

		(void)running_print_op_v2;  /* make clang happy */
#if DEBUG_VM_EXECUTION
		running_print_op_v2(vm, st->pc, sp, buf, n, ch, stderr);
#endif /* DEBUG_VM_EXECUTION */

		if (op == VM_V2_OP_FETCH) {
			if (sp >= last) {
				st->fetch_state = end;
				return VM_MATCHING;
			}

			ch = (unsigned char) *sp++;
			st->pc++;
		} else {
			int result;

			/* either STOP or BRANCH */
			result = 1;
			if (cmp != VM_CMP_ALWAYS) {
				switch (cmp) {
				case VM_CMP_LT: result = ch <  arg; break;
				case VM_CMP_LE: result = ch <= arg; break;
				case VM_CMP_GE: result = ch >= arg; break;
				case VM_CMP_GT: result = ch >  arg; break;
				case VM_CMP_EQ: result = ch == arg; break;
				case VM_CMP_NE: result = ch != arg; break;
				}
			}

			if (result) {
				switch (op) {
				case VM_V2_OP_STOP:
					st->state = (end == VM_END_FAIL) ? VM_FAIL : VM_SUCCESS;
					return st->state;

				case VM_V2_OP_BRANCH:
					{
						union {
							uint16_t u;
							int16_t  rel;
						} packed;

						packed.u = dest;
						st->pc += packed.rel;
					}
					break;

				case VM_V2_OP_IBRANCH:
					assert((uint32_t)dest < vm->alen);
					st->pc = vm->abuf[dest];
					break;

				default:
					// should not reach!
					abort();
				}
			} else {
				st->pc++;
			}
		}
	}

	return VM_FAIL;
}

void
dfavm_v2_finalize(struct dfavm_v2 *vm)
{
	free(vm->abuf);
	vm->abuf = NULL;
	vm->alen = 0;

	free(vm->ops);
	vm->ops = NULL;
	vm->len = 0;
}

