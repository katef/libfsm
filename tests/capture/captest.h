/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CAPTEST_H
#define CAPTEST_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include <re/re.h>

/* for captest_run_case_program, to evaluate handwritten programs */
#include "../../src/libfsm/capture_vm_program.h"
#include "../../src/libfsm/capture_vm.h"

#define MAX_CAPTEST_SINGLE_CAPTURE_PAIRS 16
#define MAX_CAPTEST_MULTI_CAPTURE_PAIRS 16
#define MAX_CAPTEST_PROGRAM_CAPTURE_PAIRS 16

/* position representing no match */
#define POS_NONE { (size_t)-1, (size_t)-1 }

/* If verbosity is set to this (with -vvvvvvvvv) then dump all the
 * compiled programs to 'prog_output'. */
#define DUMP_PROGRAMS_VERBOSITY 9

enum captest_match {
	SHOULD_MATCH = 0,	/* implied, set by designated initializer */
	SHOULD_NOT_MATCH = 1,
	SHOULD_REJECT_AS_UNSUPPORTED = 2,
	SHOULD_SKIP = 3,
};

struct captest_case_single {
	const char *regex;
	const char *input;
	enum captest_match match;
	bool no_nl;		/* do not retry with trailing newline */

	size_t count;
	struct fsm_capture expected[MAX_CAPTEST_SINGLE_CAPTURE_PAIRS];
};

/* Same as captest_case_single, but these expect multiple (possibly overlapping)
 * regexes to be combined before checking the match/capture behavior. */
#define MAX_REGEXES 4
#define MAX_INPUTS 8
#define MAX_CAPTEST_MULTI_EXPECTED 8
struct captest_case_multi {
	uint8_t regex_count;
	const char *regexes[MAX_REGEXES];
	enum captest_match match;
	bool no_nl;

	struct multi_case_input_info {
		const char *input; /* first NULL input = end of list */
		struct case_multi_expected {
			uint8_t regex;	 /* expected: ascending order */
			uint8_t capture; /* 0 is default */
			size_t pos[2];
		} expected[MAX_CAPTEST_MULTI_EXPECTED];
	} inputs[MAX_INPUTS];
};

struct captest_case_multi_result {
	size_t pass;
	size_t fail;
};

#define MAX_PROGRAM_CHAR_CLASSES 4
#define MAX_PROGRAM_OPS 32
struct captest_case_program {
	const char *input;

	struct capvm_char_class char_class[MAX_PROGRAM_CHAR_CLASSES];

	struct {
		uint32_t count;
		uint32_t base;
		struct fsm_capture captures[MAX_CAPTEST_PROGRAM_CAPTURE_PAIRS];
	} expected;

	/* termined by 0'd record, { .t == CAPVM_OP_CHAR, .u.chr = 0x00 } */
	struct capvm_opcode ops[MAX_PROGRAM_OPS];
};

enum captest_run_case_res {
	CAPTEST_RUN_CASE_PASS,
	CAPTEST_RUN_CASE_FAIL,
	CAPTEST_RUN_CASE_ERROR,
};
enum captest_run_case_res
captest_run_case(const struct captest_case_single *testcase,
    int verbosity, bool trailing_newline, FILE *prog_output);

enum captest_run_case_res
captest_run_case_multi(const struct captest_case_multi *testcase,
    int verbosity, bool trailing_newline, FILE *prog_output,
    struct captest_case_multi_result *result);

/* This should probably only be used for evaluating specific
 * hand-written programs for development, because we only care
 * about supporting the kinds of programs that could be produced
 * by compiling from valid regexes. In other words, this is not
 * a stable public interface. */
enum captest_run_case_res
captest_run_case_program(const struct captest_case_program *testcase,
    int verbosity);


#endif
