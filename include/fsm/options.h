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

struct fsm_allocator {
	void (*free)(void *opaque, void *p);
	void *(*malloc)(void *opaque, size_t sz);
	void *(*realloc)(void *opaque, void *p, size_t sz);
	void *opaque;
};

struct fsm_options {
	/* boolean: true indicates to go to extra lengths in order to produce
	 * neater-looking FSMs for certian operations. This usually means finding
	 * a state which can be re-used, at the cost of runtime.
	 * Keep this false if you want performant operations for large FSM. */
	unsigned int tidy:1;

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

	/* for generated code, what kind of I/O API to generate */
	enum fsm_io io;

	/* a prefix for namespacing generated identifiers. NULL if not required. */
	const char *prefix;

	/* character pointer, for C code fragment output. NULL for the default. */
	const char *cp;

	/* TODO: explain. for C code fragment output */
	int (*leaf)(FILE *, const void *state_opaque, const void *leaf_opaque);
	void *leaf_opaque;

	/* TODO: explain. for C code fragment output */
	int (*endleaf)(FILE *, const void *state_opaque, const void *endleaf_opaque);
	void *endleaf_opaque;

	/* TODO: explain */
	void (*carryopaque)(const struct fsm_state **, size_t,
		struct fsm *, struct fsm_state *);

	/* closure for custom allocation functions */
	const struct fsm_allocator allocator;
};

#endif

