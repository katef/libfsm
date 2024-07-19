/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_OPTIONS_H
#define FSM_OPTIONS_H

#include <stdio.h>

struct fsm;
struct fsm_state;

enum fsm_io {
	FSM_IO_GETC,
	FSM_IO_STR,
	FSM_IO_PAIR
};

// TODO: comment
enum fsm_ambig {
	AMBIG_NONE     = 0, /* default */
	AMBIG_ERROR    = 1 << 0,
	AMBIG_EARLIEST = 1 << 1,
	AMBIG_MULTIPLE = 1 << 2,

	AMBIG_SINGLE   = AMBIG_ERROR | AMBIG_EARLIEST
};

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

	/* for generated code, what kind of I/O API to generate */
	enum fsm_io io;

	/* for generated code, how to handle multiple endids on an accepting state */
	enum fsm_ambig ambig;

	/* a prefix for namespacing generated identifiers. NULL if not required. */
	const char *prefix;

	/* the name of the enclosing package; NULL to use `prefix` (default). */
	const char *package_prefix;

	/*
	 * Hooks to override generated code. These give an oportunity to
	 * emit application-specific constructs, especially based on ids
	 * attached to end states.
	 *
	 * These hooks are optional, and default to whatever makes sense
	 * for the language.
	 *
	 * Placement in the output stream depends on the format.
	 * This replaces an entire "return xyz;" statement for C-like formats,
	 * but appends extra information for others.
	 */
	struct fsm_hooks {
		/* character pointer, for C code fragment output. NULL for the default. */
		const char *cp;

		int (*args)(FILE *, const struct fsm_options *opt,
			void *leaf_opaque);

		/*
		 * ids[] is sorted and does not have duplicates.
		 */
		int (*accept)(FILE *, const struct fsm_options *opt,
			const fsm_end_id_t *ids, size_t count,
			void *leaf_opaque);

		int (*reject)(FILE *, const struct fsm_options *opt,
			void *leaf_opaque);

		void *hook_opaque;
	} hooks;
};

#endif

