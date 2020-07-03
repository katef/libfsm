#ifndef RUNNER_H
#define RUNNER_H

#include <stddef.h>

#include <fsm/vm.h>

struct fsm;
struct fsm_dfavm;

enum error_type {
	ERROR_NONE = 0          ,
	ERROR_BAD_RECORD_TYPE   ,
	ERROR_ESCAPE_SEQUENCE   ,
	ERROR_PARSING_REGEXP    ,
	ERROR_DETERMINISING     ,
	ERROR_COMPILING_BYTECODE,
	ERROR_SHOULD_MATCH      ,
	ERROR_SHOULD_NOT_MATCH  ,
	ERROR_FILE_IO           ,
	ERROR_CLOCK_ERROR       ,
	ERROR_INVALID_PARAMETER ,
};

enum implementation {
	IMPL_C,
	IMPL_VMC,
	IMPL_VMASM,
	IMPL_INTERPRET
};

struct fsm_runner {
	enum implementation impl;

	union {
		struct {
			void *h;
			int (*func)(const char *, const char *);
		} impl_c;

		struct {
			void *h;
			int (*func)(const unsigned char *, size_t);
		} impl_asm;

		struct {
			struct fsm_dfavm *vm;
		} impl_vm;
	} u;
};

enum error_type
fsm_runner_initialize(struct fsm *fsm, struct fsm_runner *r, enum implementation impl, struct fsm_vm_compile_opts vm_opts);

void
fsm_runner_finalize(struct fsm_runner *r);

int
fsm_runner_run(const struct fsm_runner *r, const char *s, size_t n);

#endif /* RUNNER_H */

