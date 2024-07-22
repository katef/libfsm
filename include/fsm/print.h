/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_PRINT_H
#define FSM_PRINT_H

struct fsm;
struct fsm_options;

/* a convenience for debugging */
void fsm_dump(FILE *f, const struct fsm *fsm,
	const char *file, unsigned line, const char *name);
#define fsm_dump(f, fsm) (fsm_dump)((f), (fsm), __FILE__, __LINE__, #fsm);

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
		void *leaf_opaque, void *hook_opaque);

	/*
	 * ids[] is sorted and does not have duplicates.
	 */
	int (*accept)(FILE *, const struct fsm_options *opt,
		const fsm_end_id_t *ids, size_t count,
		void *leaf_opaque, void *hook_opaque);

	int (*reject)(FILE *, const struct fsm_options *opt,
		void *leaf_opaque, void *hook_opaque);

	void *hook_opaque;
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
int fsm_print(FILE *f, const struct fsm *fsm,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	enum fsm_print_lang lang);

#endif

