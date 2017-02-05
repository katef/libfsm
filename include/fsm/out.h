/* $Id$ */

#ifndef FSM_OUT_H
#define FSM_OUT_H

#include <stdio.h>

struct fsm;

enum fsm_out {
	FSM_OUT_C,
	FSM_OUT_CSV,
	FSM_OUT_DOT,
	FSM_OUT_FSM,
	FSM_OUT_JSON
};

enum fsm_io {
	FSM_IO_GETC,
	FSM_IO_STR
};

struct fsm_outoptions {
	/* boolean: true indicates to omit names for states in output */
	unsigned int anonymous_states:1;

	/* boolean: true indicates to optmise aesthetically during output by
	 * consolidating similar edges, and outputting a single edge with a more
	 * concise label. Only character edges are consolidated. */
	unsigned int consolidate_edges:1;

	/* boolean: true indicates to produce a fragment of code as output, rather
	 * than a complete self-contained language. e.g. for FSM_OUT_C, this will
	 * be the guts of a function, rather than a complete .c file. This is
	 * intended to be useful for embedding into other code. */
	unsigned int fragment:1;

	/* boolean: true indicates to output comments in the generated code,
	 * where possible. */
	unsigned int comments:1;

	/* for generated code, what kind of I/O API to generate */
	enum fsm_io io;

	/* a prefix for namespacing generated identifiers. NULL if not required. */
	const char *prefix;
};


/*
 * Output an FSM to the given file stream. The output is written in the format
 * specified. The available formats are:
 *
 *  FSM_OUT_C    - ISO C90 code
 *  FSM_OUT_CSV  - A comma-separated state transition matrix
 *  FSM_OUT_DOT  - Graphviz Dot format, intended for rendering graphically
 *  FSM_OUT_FSM  - fsm(5) .fsm format, suitable for parsing by fsm(1)
 *  FSM_OUT_JSON - JavaScript Object Notation
 *
 * The output options may be NULL, indicating to use defaults.
 *
 * TODO: what to return?
 * TODO: explain constraints (e.g. single-character labels for C output)
 */
void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format,
	const struct fsm_outoptions *options);

#endif

