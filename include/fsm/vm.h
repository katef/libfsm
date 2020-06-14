#ifndef FSM_VM_H
#define FSM_VM_H

#include <stdio.h>
#include <stddef.h>

struct fsm;
struct fsm_dfavm;

enum fsm_vm_compile_flags {
	FSM_VM_COMPILE_PRINT_IR        = 0x0001,
	FSM_VM_COMPILE_PRINT_IR_PREOPT = 0x0002,

	FSM_VM_COMPILE_PRINT_ENC       = 0x0004,

	FSM_VM_COMPILE_OPTIM1          = 0X0008,
	FSM_VM_COMPILE_OPTIM2          = 0X0010

	/*
	FSM_VM_COMPILE_VAR_ENC         = 0x0020,
	FSM_VM_COMPILE_FIXED_ENC       = 0x0040,
	*/
};

#define FSM_VM_COMPILE_DEFAULT_FLAGS (FSM_VM_COMPILE_OPTIM1 | FSM_VM_COMPILE_OPTIM2)

struct fsm_vm_compile_opts {
	unsigned int flags;
	FILE *out;
};

struct fsm_dfavm *
fsm_vm_compile(const struct fsm *fsm);

struct fsm_dfavm *
fsm_vm_compile_with_opts(const struct fsm *fsm, struct fsm_vm_compile_opts opts);

struct fsm_dfavm *
fsm_vm_read(FILE *f);

struct fsm_dfavm *
fsm_vm_write(const struct fsm_dfavm *vm, FILE *f);

struct fsm_dfavm *
fsm_vm_print(const struct fsm_dfavm *vm, FILE *f);

int
fsm_vm_match_file(const struct fsm_dfavm *mv, FILE *f);

int
fsm_vm_match_buffer(const struct fsm_dfavm *mv, const char *buf, size_t n);

void fsm_vm_free(struct fsm_dfavm *);

#endif /* FSM_VM_H */

