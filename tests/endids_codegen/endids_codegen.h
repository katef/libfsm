#ifndef ENDIDS_CODEGEN_H
#define ENDIDS_CODEGEN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/print.h>
#include <fsm/bool.h>

#include <re/re.h>

#define ENDIDS_CODEGEN_MAX_PATTERNS 8

/* A set of (regex, input) pairs that should have the same matching behavior
 * individually, when combined, and when combined using codegen. */
struct endids_codegen_testcase {
	struct {
		const char *pattern;
		const char *input; /* if NULL, use pattern */
	} patterns[ENDIDS_CODEGEN_MAX_PATTERNS];
};

int
endids_codegen_generate(const struct endids_codegen_testcase *testcase);

#endif
