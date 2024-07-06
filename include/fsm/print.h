/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_PRINT_H
#define FSM_PRINT_H

#include <stdio.h>

struct fsm;

enum fsm_print_lang {
	FSM_PRINT_NONE,       /* No output */

	FSM_PRINT_AMD64_ATT,  /* Assembly in various dialects */
	FSM_PRINT_AMD64_GO,
	FSM_PRINT_AMD64_NASM,

	FSM_PRINT_API,        /* C code which calls the fsm(3) API */
	FSM_PRINT_AWK,        /* awk code (gawk dialect) */
	FSM_PRINT_C,          /* ISO C90 code */
	FSM_PRINT_DOT,        /* Graphviz Dot format, intended for rendering graphically */
	FSM_PRINT_FSM,        /* fsm(5) .fsm format, suitable for parsing by fsm(1) */
	FSM_PRINT_GO,         /* Go code */
	FSM_PRINT_IR,         /* Codegen IR as Dot */
	FSM_PRINT_IRJSON,     /* Codegen IR as JSON */
	FSM_PRINT_JSON,       /* JavaScript Object Notation */
	FSM_PRINT_LLVM,       /* Live Laugh Virtual Machine */
	FSM_PRINT_RUST,       /* Rust code */
	FSM_PRINT_SH,         /* Shell script (bash dialect) */
	FSM_PRINT_VMC,        /* ISO C90 code, VM style */
	FSM_PRINT_VMDOT,      /* Graphviz Dot format, showing VM opcodes */

	FSM_PRINT_VMOPS_C,    /* VM opcodes as a datastructure */
	FSM_PRINT_VMOPS_H,
	FSM_PRINT_VMOPS_MAIN
};

/*
 * Print an FSM to the given file stream. The output is written in the format
 * specified.
 *
 * The output options may be NULL, indicating to use defaults.
 *
 * Returns 0, or -1 on error and errno will be set. An errno of ENOTSUP means
 * the requested IO API is not implemented for this output language.
 */
int fsm_print(FILE *f, const struct fsm *fsm, enum fsm_print_lang lang);

#endif

