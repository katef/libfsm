/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/vm.h>

#include "internal.h"

#include "vm/vm.h"
#include "vm/retlist.h"
#include "print/ir.h"

// VM state:
//
//   Conceptually, the VM is composed of:
//   - string buffer: holds 8-bit bytes to match
//   - program buffer: holds the VM bytecode
//   - address buffer: holds various tables and data
//   - three additional registers:
//
//     SP - string pointer  - 32-bit unsigned offset into string buffer
//     SB - string byte     -  8-bit unsigned register, usually holds
//                                   string buffer byte at SP
//     PC - program counter - 32-bit unsigned offset into program buffer
//
// Note: currently the address buffer and string buffer are currently not used

enum dfavm_io_result
fsm_dfavm_save(FILE *f, const struct fsm_dfavm *vm)
{
	if (vm->version_major == DFAVM_VARENC_MAJOR && vm->version_minor == DFAVM_VARENC_MINOR) {
		return dfavm_v1_save(f, &vm->u.v1);
	}
	else {
		return DFAVM_IO_UNSUPPORTED_VERSION;
	}
}

enum dfavm_io_result
fsm_dfavm_load(FILE *f, struct fsm_dfavm *vm)
{
	unsigned char header[8];
	/* read and check header */
	if (fread(&header[0], sizeof header, 1, f) != 1) {
		return DFAVM_IO_ERROR_READING;
	}

	if (memcmp(&header[0], DFAVM_MAGIC, 6) != 0) {
		return DFAVM_IO_BAD_HEADER;
	}

	if (header[6] == DFAVM_VARENC_MAJOR && header[7] == DFAVM_VARENC_MINOR) {
		vm->version_major = header[6];
		vm->version_minor = header[7];

		return dfavm_load_v1(f, &vm->u.v1);
	}

	return DFAVM_IO_UNSUPPORTED_VERSION;
}

const char *
cmp_name(int cmp)
{
	switch (cmp) {
	case VM_CMP_ALWAYS: return "";   break;
	case VM_CMP_LT:     return "LT"; break;
	case VM_CMP_LE:     return "LE"; break;
	case VM_CMP_EQ:     return "EQ"; break;
	case VM_CMP_GE:     return "GE"; break;
	case VM_CMP_GT:     return "GT"; break;
	case VM_CMP_NE:     return "NE"; break;
	default:            return "??"; break;
	}
}

struct fsm_dfavm *
fsm_vm_compile_with_options(const struct fsm *fsm,
	const struct fsm_options *opt,
	struct fsm_vm_compile_opts vmopts)
{
	static const struct dfavm_assembler_ir zero;
	struct dfavm_assembler_ir a;
	struct ir *ir;
	struct ret_list retlist;
	struct fsm_dfavm *vm;

	assert(fsm != NULL);
	assert(opt != NULL);

	ir = make_ir(fsm, opt);
	if (ir == NULL) {
		return NULL;
	}

	if (!build_retlist(&retlist, ir)) {
		free_ir(fsm, ir);
		return NULL;
	}

	a = zero;

	if (!dfavm_compile_ir(&a, ir, &retlist, vmopts)) {
		free_retlist(&retlist);
		free_ir(fsm, ir);
		return NULL;
	}

	free_ir(fsm, ir);

	vm = dfavm_compile_vm(&a, vmopts);
	if (vm == NULL) {
		return NULL;
	}

	free_retlist(&retlist);
	dfavm_opasm_finalize_op(&a);

	return vm;
}

struct fsm_dfavm *
fsm_vm_compile(const struct fsm *fsm)
{
	// static const struct fsm_vm_compile_opts defaults = { FSM_VM_COMPILE_DEFAULT_FLAGS | FSM_VM_COMPILE_PRINT_IR_PREOPT | FSM_VM_COMPILE_PRINT_IR, NULL };
	static const struct fsm_vm_compile_opts defaults = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };
	static const struct fsm_options opt_default;

	assert(fsm != NULL);

	/*
	 * The only fsm_option make_ir() uses is .comments,
	 * for whether to generate examples.
	 *
	 * The caller won't use that, because the only user-facing
	 * vm interfaces are to interpret for execution. So we use
	 * default options here.
	 */
	return fsm_vm_compile_with_options(fsm, &opt_default, defaults);
}

void
fsm_vm_free(struct fsm_dfavm *vm)
{
	dfavm_free_vm(vm);
}

static enum dfavm_state
vm_match(const struct fsm_dfavm *vm, struct vm_state *st, const char *buf, size_t n)
{
	if (vm->version_major == DFAVM_VARENC_MAJOR && vm->version_minor == DFAVM_VARENC_MINOR) {
		return vm_match_v1(&vm->u.v1, st, buf, n);
	} else if (vm->version_major == DFAVM_FIXEDENC_MAJOR && vm->version_minor == DFAVM_FIXEDENC_MINOR) {
		return vm_match_v2(&vm->u.v2, st, buf, n);
	}

	return VM_FAIL;
}

static enum dfavm_state 
vm_end(const struct fsm_dfavm *vm, struct vm_state *st)
{
	(void)vm;

#if DEBUG_VM_EXECUTION
	fprintf(stderr, "END: st->state = %d, st->fetch_state = %d\n",
		st->state, st->fetch_state);
#endif /* DEBUG_VM_EXECUTION */

	if (st->state != VM_MATCHING) {
		return st->state;
	}

	return (st->fetch_state == VM_END_SUCC) ? VM_SUCCESS : VM_FAIL;
}

int
fsm_vm_match_file(const struct fsm_dfavm *vm, FILE *f)
{
	static const struct vm_state state_init;
	struct vm_state st = state_init;
	enum dfavm_state result;
	char buf[4096];

	result = VM_FAIL;
	for (;;) {
		size_t nb;

		nb = fread(buf, 1, sizeof buf, f);
		if (nb == 0) {
			break;
		}

		result = vm_match(vm, &st, buf, nb);
		if (result != VM_MATCHING) {
			return result == VM_SUCCESS;
		}
	}

	if (ferror(f)) {
		return 0;
	}

	return vm_end(vm, &st) == VM_SUCCESS;
}

int
fsm_vm_match_buffer(const struct fsm_dfavm *vm, const char *buf, size_t n)
{
	static const struct vm_state state_init;
	struct vm_state st = state_init;
	enum dfavm_state result;

	vm_match(vm, &st, buf, n);
	result = vm_end(vm, &st);

	return result == VM_SUCCESS;
}

