/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

#include <fsm/fsm.h>

#include "vm.h"

enum dfavm_io_result
dfavm_v1_save(FILE *f, const struct dfavm_v1 *vm)
{
	/* write eight byte header */
	unsigned char header[8];
	unsigned char ilen[4];

	memcpy(&header[0], DFAVM_MAGIC, 6);
	header[6] = DFAVM_VARENC_MAJOR;
	header[7] = DFAVM_VARENC_MINOR;

	if (fwrite(&header, sizeof header, 1, f) != 1) {
		return -1;
	}

	/* write out instructions */
	ilen[0] = (vm->len >>  0) & 0xff;
	ilen[1] = (vm->len >>  8) & 0xff;
	ilen[2] = (vm->len >> 16) & 0xff;
	ilen[3] = (vm->len >> 24) & 0xff;

	if (fwrite(&ilen[0],sizeof ilen, 1, f) != 1) {
		return -1;
	}

	if (fwrite(vm->ops, 1, vm->len, f) != vm->len) {
		return -1;
	}

	return 0;
}

enum dfavm_io_result
dfavm_load_v1(FILE *f, struct dfavm_v1 *vm)
{
	unsigned char ilen[4];
	uint32_t len;
	unsigned char *instr;
	size_t nread;

	/* read number of instructions */
	if (fread(&ilen[0], sizeof ilen, 1, f) != 1) {
		return DFAVM_IO_ERROR_READING;
	}

	len = ((uint32_t)ilen[0] << 0) | ((uint32_t)ilen[1] << 8) |
		((uint32_t)ilen[1] << 16) | ((uint32_t)ilen[1] << 24);

	instr = malloc(len);
	if (instr == NULL) {
		return DFAVM_IO_OUT_OF_MEMORY;
	}

	nread = fread(instr, 1, len, f);
	if (nread != len) {
		free(instr);
		return (nread > 0) ? DFAVM_IO_SHORT_READ : DFAVM_IO_ERROR_READING;
	}

	vm->ops = instr;
	vm->len = len;

	return DFAVM_IO_OK;
}

struct fsm_dfavm *
encode_opasm_v1(const struct dfavm_vm_op *instr, size_t ninstr, size_t total_bytes)
{
	static const struct fsm_dfavm zero;

	struct fsm_dfavm *ret;
	struct dfavm_v1 *vm;
	size_t i;
	size_t off;
	unsigned char *enc;

	ret = malloc(sizeof *ret);
	if (ret == NULL) {
		goto error;
	}

	*ret = zero;

	ret->version_major = DFAVM_VARENC_MAJOR;
	ret->version_minor = DFAVM_VARENC_MINOR;

	vm = &ret->u.v1;

	enc = malloc(total_bytes);
	if (enc == NULL) {
		goto error;
	}

	vm->ops = enc;
	vm->len = total_bytes;

	for (off = 0, i = 0; i < ninstr; i++) {
		unsigned char bytes[6];
		unsigned char cmp_bits, instr_bits, rest_bits;
		const struct dfavm_vm_op *op = &instr[i];
		int nb = 1;

		cmp_bits   = 0;
		instr_bits = 0;
		rest_bits  = 0;

		cmp_bits = op->cmp;
		if (cmp_bits > 7) {
			goto error;
		}

		if (op->cmp != VM_CMP_ALWAYS) {
			bytes[1] = op->cmp_arg;
			nb++;
		}

		switch (op->instr) {
		case VM_OP_STOP:
			instr_bits = 0x0;
			rest_bits  = (op->u.stop.end_bits == VM_END_SUCC) ? 0x1 : 0x0;
			break;

		case VM_OP_FETCH:
			instr_bits = 0x1;
			rest_bits  = (op->u.fetch.end_bits == VM_END_SUCC) ? 0x1 : 0x0;
			break;

		case VM_OP_BRANCH:
			instr_bits = 0x2;
			switch (op->u.br.dest) {
			case VM_DEST_SHORT:
				{
					int8_t dst;

					assert(op->u.br.rel_dest >= INT8_MIN);
					assert(op->u.br.rel_dest <= INT8_MAX);

					rest_bits = 0;

				       	dst = op->u.br.rel_dest;
					memcpy(&bytes[nb], &dst, sizeof dst);
					nb += sizeof dst;
				}
				break;

			case VM_DEST_NEAR:
				{
					int16_t dst;

					assert(op->u.br.rel_dest >= INT16_MIN);
					assert(op->u.br.rel_dest <= INT16_MAX);

					rest_bits = 1;

				       	dst = op->u.br.rel_dest;
					memcpy(&bytes[nb], &dst, sizeof dst);
					nb += sizeof dst;
				}
				break;

			case VM_DEST_FAR:
				{
					int32_t dst;

					assert(op->u.br.rel_dest >= INT32_MIN);
					assert(op->u.br.rel_dest <= INT32_MAX);

					rest_bits = 2;

				       	dst = op->u.br.rel_dest;
					memcpy(&bytes[nb], &dst, sizeof dst);
					nb += sizeof dst;
				}
				break;

			case VM_DEST_NONE:
			default:
				goto error;
			}

			break;

		default:
			goto error;
		}

		bytes[0] = (cmp_bits << 5) | (instr_bits << 3) | (rest_bits);
#if DEBUG_ENCODING
		fprintf(stderr, "enc[%4zu] instr=0x%02x cmp=0x%02x rest=0x%02x b=0x%02x\n",
			off, instr_bits, cmp_bits, rest_bits, bytes[0]);
#endif /* DEBUG_ENCODING */

		memcpy(&enc[off], bytes, nb);
		off += nb;

		assert(off <= total_bytes);
	}

	assert(off == total_bytes);

	return ret;

error:
	if (ret != NULL) {
		free(ret);
	}
	return NULL;
}

uint32_t
running_print_op_v1(const unsigned char *ops, uint32_t pc, const char *sp, const char *buf, size_t n, char ch, FILE *f) {
	int op, cmp, end, dest;
	unsigned char b = ops[pc];
	op = (b >> 3) & 0x03;
	cmp = (b>>5)  & 0x07;
	dest = b & 0x03;
	end  = b & 0x01;

	fprintf(f, "[%4" PRIu32 " sp=%zd n=%zu ch=%3u '%c' end=%d] ",
		pc, sp-buf, n, (unsigned char)ch, isprint(ch) ? ch : ' ', end);

	switch (op) {
	case VM_OP_FETCH:
		fprintf(f, "FETCH%s\n", (end == VM_END_FAIL) ? "F" : "S");
		break;

	case VM_OP_STOP:
		fprintf(f, "STOP%s%s", (end == VM_END_FAIL) ? "F" : "S", cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			int arg = ops[++pc];
			if (isprint(arg)) {
				fprintf(f, " '%c'", arg);
			} else {
				fprintf(f, " %d", arg);
			}
		}
		fprintf(f, "\n");
		break;

	case VM_OP_BRANCH:
		fprintf(f, "BR%s%s", (dest == 0 ? "S" : (dest == 1 ? "N" : "F")), cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			int arg = ops[++pc];
			if (isprint(arg)) {
				fprintf(f, " '%c',", arg);
			} else {
				fprintf(f, " %d,", arg);
			}
		}


		{
			int32_t rel;
			unsigned char bytes[4] = { 0, 0, 0, 0 };

			if (dest == 0) {
				union {
					uint8_t u;
					int8_t  i;
				} packed;

				packed.u = ops[pc+1];
				bytes[0] = packed.u;
				rel = packed.i;
				pc += 1;
			} else if (dest == 1) {
				union {
					uint16_t u;
					int16_t  i;
				} packed;

				packed.u = (ops[pc+1] | (ops[pc+2] << 8));
				bytes[0] = ops[pc+1];
				bytes[1] = ops[pc+2];
				rel = packed.i;
				pc += 2;
			} else {
				union {
					uint32_t u;
					int32_t  i;
				} packed;
				packed.u = ops[pc+1] | (ops[pc+2] << 8) | (ops[pc+3] << 16) | (ops[pc+4] << 24);
				bytes[0] = ops[pc+1];
				bytes[1] = ops[pc+2];
				bytes[2] = ops[pc+3];
				bytes[3] = ops[pc+4];
				rel = packed.i;
				pc += 4;
			}

			fprintf(f, " %ld 0x%02x 0x%02x 0x%02x 0x%02x\n",
				(long)rel, bytes[0], bytes[1], bytes[2], bytes[3]);
		}
		break;

	default:
		fprintf(f, "UNK\n");
	}

	return pc;
}

enum dfavm_state
vm_match_v1(const struct dfavm_v1 *vm, struct vm_state *st, const char *buf, size_t n)
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
		int op;
		unsigned char b;

		/* decode instruction */
		b = vm->ops[st->pc];
		op = (b>>3)&0x03;

		(void)running_print_op_v1;  /* make clang happy */
#if DEBUG_VM_EXECUTION
		running_print_op_v1(vm->ops, st->pc, sp, buf, n, ch, stderr);
#endif /* DEBUG_VM_EXECUTION */

		if (op == VM_OP_FETCH) {
			int end;

			end = b & 0x01;
			if (sp >= last) {
				st->fetch_state = end;
				return VM_MATCHING;
			}

			ch = (unsigned char) *sp++;
			st->pc++;
		} else {
			int cmp, end, dest, dest_nbytes;
			int result;
			uint32_t off;

			/* either STOP or BRANCH */
			off = st->pc+1;
			cmp  = b >> 5;

			result = 0;
			if (cmp == VM_CMP_ALWAYS) {
				result = 1;
			} else {
				int arg;

				arg = vm->ops[off++];

				switch (cmp) {
				case VM_CMP_LT: result = ch <  arg; break;
				case VM_CMP_LE: result = ch <= arg; break;
				case VM_CMP_GE: result = ch >= arg; break;
				case VM_CMP_GT: result = ch >  arg; break;
				case VM_CMP_EQ: result = ch == arg; break;
				case VM_CMP_NE: result = ch != arg; break;
				}
			}

			dest = b & 0x3;
			dest_nbytes = 1 << dest;

			if (result) {
				int32_t rel;

				if (op == VM_OP_STOP) {
					end = b & 0x01;
					st->state = (end == VM_END_FAIL) ? VM_FAIL : VM_SUCCESS;
					return st->state;
				}

				if (dest == 0) {
					rel = (int8_t)(vm->ops[off++]);
				} else if (dest == 1) {
					union {
						uint16_t u;
						int16_t  i;
					} packed;

					packed.u = (vm->ops[off+0] | (vm->ops[off+1] << 8));
					rel = packed.i;
					off += 2;
				} else {
					union {
						uint32_t u;
						int32_t  i;
					} packed;
					packed.u = vm->ops[off+0] | (vm->ops[off+1] << 8) | (vm->ops[off+2] << 16) | (vm->ops[off+3] << 24);
					rel = packed.i;
				}

				if (rel >= 0) {
					st->pc += (uint32_t)rel;
				} else {
					st->pc -= (uint32_t)-rel;
				}
			} else if (op == VM_OP_BRANCH) {
				st->pc = off + dest_nbytes;
			} else {
				st->pc = off;
			}
		}
	}

	return VM_FAIL;
}

void
dfavm_v1_finalize(struct dfavm_v1 *vm)
{
	free(vm->ops);
	vm->ops = NULL;
	vm->len = 0;
}

