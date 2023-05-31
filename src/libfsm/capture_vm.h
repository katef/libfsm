/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CAPTURE_VM_H
#define CAPTURE_VM_H

#include <stdint.h>
#include <stdio.h>

#include <fsm/alloc.h>
#include <fsm/capture.h>

/* Interface the virtual machine used to resolve captures.
 * These interfaces are exposed to libre but should not be
 * used directly. */

/* Opaque struct, details in capture_vm_program.h. */
struct capvm_program;

void
fsm_capvm_program_free(const struct fsm_alloc *alloc,
    struct capvm_program *program);

struct capvm_program *
capvm_program_copy(const struct fsm_alloc *alloc,
    const struct capvm_program *program);

/* Add an offset to the capture ID base for a program.
 * Used when FSMs are merged, one of the source FSMs' capture IDs
 * will be shifted to appear after the others. */
void
capvm_program_rebase(struct capvm_program *program, unsigned capture_offset);

void
fsm_capvm_program_dump(FILE *f,
    const struct capvm_program *program);

enum fsm_capvm_program_exec_res {
	FSM_CAPVM_PROGRAM_EXEC_SOLUTION_WRITTEN,
	FSM_CAPVM_PROGRAM_EXEC_NO_SOLUTION_FOUND,

	FSM_CAPVM_PROGRAM_EXEC_ERROR_ALLOC = -1,
	/* FIXME: what other ways can this fail? */
};

/* Execute a capture program with the given input and populate
 * the capture buffer.
 *
 * This asserts that the capture buffer is at least as large as
 * necessary. This is an internal interface, and the buffer size
 * should have already been checked by the caller. */
enum fsm_capvm_program_exec_res
fsm_capvm_program_exec(const struct capvm_program *program,
    const uint8_t *input, size_t length,
    struct fsm_capture *capture_buf, size_t capture_buf_length);

/* Get the capture count from the program. */
unsigned
fsm_capvm_program_get_capture_count(const struct capvm_program *program);

/* Get the max capture ID from the program.
 * If there are no captures (which is pointless) it will return 0. */
unsigned
fsm_capvm_program_get_max_capture_id(const struct capvm_program *program);

#endif
