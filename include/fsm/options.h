/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_OPTIONS_H
#define FSM_OPTIONS_H

enum fsm_io {
	FSM_IO_GETC,
	FSM_IO_STR,
	FSM_IO_PAIR
};

/*
 * Ambiguity mode. 
 *
 * The ambiguity mode controls how endids are emitted into generated code,
 * and passed to the hooks.accept() callback for an accepting state.
 *
 * This also causes fsm_print() to resolve endid conflicts in various ways
 * for accepting states that have multiple endids present:
 *
 *   AMBIG_NONE     - Do not emit endids.
 *   AMBIG_ERROR    - Emit a single endid.
 *                    hook.conflict() is called for accepting states
 *                    which have multiple endids, and printing errors.
 *   AMBIG_EARLIEST - Emit the lowest numbered endid only.
 *   AMBIG_MULTIPLE - Emit all endids.
 *
 * What it means to emit multiple or single endids is particular to each
 * output language; not every language supports both.
 */
enum fsm_ambig {
	AMBIG_NONE     = 0, /* default */
	AMBIG_ERROR    = 1 << 0,
	AMBIG_EARLIEST = 1 << 1,
	AMBIG_MULTIPLE = 1 << 2,

	AMBIG_SINGLE   = AMBIG_ERROR | AMBIG_EARLIEST,
	AMBIG_ANY      = ~0
};

/*
 * Print options.
 *
 * This is separate to <fsm/print.h> because we also borrow it for libre.
 */
struct fsm_options {
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

	/* boolean: true indicates to use compiler-specific case ranges in generated
	 * code, when possible.
	 */
	unsigned int case_ranges:1;

	/* boolean: true indicates to always use hex escape sequences, rather than
	 * printable characters. This better suits binary data.
	 */
	unsigned int always_hex:1;

	/* boolean: true indicates to group edges with a common destination in output,
	 * when possible, rather than printing them all individually.
	 */
	unsigned int group_edges:1;

	/* a prefix for namespacing generated identifiers. NULL if not required. */
	const char *prefix;

	/* the name of the enclosing package; NULL to use `prefix` (default). */
	const char *package_prefix;

	/* for generated code, what kind of I/O API to generate */
	enum fsm_io io;

	/* for generated code, how to handle multiple endids on an accepting state */
	enum fsm_ambig ambig;
};

#endif

