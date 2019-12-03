#ifndef FSM_VM_H
#define FSM_VM_H

#include <stdio.h>
#include <stddef.h>

struct fsm;
struct fsm_dfavm;

struct fsm_dfavm *
fsm_vm_compile(const struct fsm *fsm);

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

