#include "captest.h"

#include <fsm/print.h>
#include <fsm/bool.h>

#include <errno.h>

/* for fsm_capvm_program_exec */
#include "../../src/libfsm/capture_vm.h"

struct captest_input {
	const char *string;
	size_t pos;
};

static int
captest_getc(void *opaque)
{
	struct captest_input *input = opaque;
	int res = input->string[input->pos];
	input->pos++;
	return res == 0 ? EOF : res;
}

static struct fsm_options options = {
	.group_edges = 1,
};

#define MAX_INPUT_WITH_NL_LENGTH 1000
static char
input_with_nl[MAX_INPUT_WITH_NL_LENGTH];

enum captest_run_case_res
captest_run_case(const struct captest_case_single *testcase,
    int verbosity, bool trailing_newline, FILE *prog_output)
{
	bool dump_captures = false;
	enum captest_run_case_res res = CAPTEST_RUN_CASE_ERROR;
	struct re_err err;

	if (verbosity == DUMP_PROGRAMS_VERBOSITY) {
		assert(prog_output != NULL);
	} else {
		assert(prog_output == NULL);
	}

	if (verbosity > 0) {
		printf("/%s/ <- \"%s%s\": ",
		    testcase->regex, testcase->input,
		    trailing_newline ? "\\n" : "");
	}

	/* build regex */
	const enum re_flags flags = 0;
	struct captest_input comp_input = {
		.string = testcase->regex,
	};

	struct fsm *fsm = re_comp(RE_PCRE,
		    captest_getc, &comp_input,
		    &options, flags, &err);

	if (testcase->match == SHOULD_REJECT_AS_UNSUPPORTED) {
		if (fsm != NULL) {
			fsm_free(fsm);
			if (verbosity > 0) {
				printf("FAIL (expected UNSUPPORTED)\n");
			}
			return CAPTEST_RUN_CASE_FAIL;
		}
		if (verbosity > 0) {
			printf("pass\n");
		}
		return CAPTEST_RUN_CASE_PASS;
	}

	assert(fsm != NULL);

	if (!fsm_determinise(fsm)) {
		return CAPTEST_RUN_CASE_ERROR;
	}

	if (!fsm_minimise(fsm)) {
		return CAPTEST_RUN_CASE_ERROR;
	}

	if (verbosity > 3) {
		fsm_print_fsm(stdout, fsm);
	}

	if (trailing_newline) {
		const size_t length = strlen(testcase->input);
		assert(length + 1 < MAX_INPUT_WITH_NL_LENGTH);
		memcpy(input_with_nl, testcase->input,
		    length);
		input_with_nl[length] = '\n';
		input_with_nl[length + 1] = '\0';
	}

	const char *input = trailing_newline
	    ? input_with_nl
	    : testcase->input;
	assert(input != NULL);
	const size_t length = strlen(input);

	fsm_state_t end;	/* unused but required by API */
	struct fsm_capture capture_buf[MAX_CAPTEST_SINGLE_CAPTURE_PAIRS];
	const size_t capture_buf_length = MAX_CAPTEST_SINGLE_CAPTURE_PAIRS;

	/* Initialize with values that are distinct from FSM_CAPTURE_NO_POS
	 * and will stand out visually. Should never see these. */
	for (size_t i = 0; i < MAX_CAPTEST_SINGLE_CAPTURE_PAIRS; i++) {
		capture_buf[i].pos[0] = 88888888;
		capture_buf[i].pos[1] = 99999999;
	}

	/* If verbosity is exactly DUMP_PROGRAMS_VERBOSITY, then print out capture info and pass. */
	if (verbosity == DUMP_PROGRAMS_VERBOSITY) {
		assert(prog_output != NULL);
		if (!trailing_newline) {
			const char *match_str = testcase->match == SHOULD_MATCH ? "SHOULD_MATCH"
			    : testcase->match == SHOULD_NOT_MATCH ? "SHOULD_NOT_MATCH"
			    : testcase->match == SHOULD_REJECT_AS_UNSUPPORTED ? "SHOULD_REJECT_AS_UNSUPPORTED"
			    : "ERROR";
			fprintf(prog_output, "regex \"%s\", input \"%s\", match %s, no_nl %d, count %zu:",
			    testcase->regex, testcase->input, match_str, testcase->no_nl,
			    testcase->count);
			for (size_t i = 0; i < testcase->count; i++) {
				fprintf(prog_output, " %zu:[%zd, %zd]",
				    i, testcase->expected[i].pos[0], testcase->expected[i].pos[1]);
			}
			fprintf(prog_output, "\n");
			fsm_capture_dump(prog_output, "capture_info", fsm);
		}
		fsm_free(fsm);
		return CAPTEST_RUN_CASE_PASS;
	}

	/* first, execute with a capture buffer that is one cell too small and check for an error */
	const size_t capture_ceil = fsm_capture_ceiling(fsm);
	assert(capture_ceil > 0);
	const size_t insufficient_capture_buf_length = capture_ceil - 1;
	errno = 0;
	int exec_res = fsm_exec_with_captures(fsm,
	    (const unsigned char *)input, length, &end, capture_buf, insufficient_capture_buf_length);
	assert(exec_res == -1);
	assert(errno == EINVAL);
	errno = 0;

	/* then, execute and check result & captures */
	exec_res = fsm_exec_with_captures(fsm,
	    (const unsigned char *)input, length, &end, capture_buf, capture_buf_length);
	if (exec_res == -1) {
		perror("fsm_exec_with_captures");
		return CAPTEST_RUN_CASE_ERROR;
	}

	if (testcase->match == SHOULD_NOT_MATCH) { /* expect match failure */
		res = (exec_res == 0
		    ? CAPTEST_RUN_CASE_PASS
		    : CAPTEST_RUN_CASE_FAIL);
	} else if (exec_res == 0) {
		res = CAPTEST_RUN_CASE_FAIL; /* didn't match, should have */
	} else {
		res = CAPTEST_RUN_CASE_PASS;
		if (verbosity > 1) {
			dump_captures = true;
		}

		/* check captures against expected */
		for (size_t i = 0; i < testcase->count; i++) {
			if (testcase->expected[i].pos[0] != capture_buf[i].pos[0] ||
			    testcase->expected[i].pos[1] != capture_buf[i].pos[1]) {
				res = CAPTEST_RUN_CASE_FAIL;
				dump_captures = true;
			}
		}
	}

	switch (res) {
	case CAPTEST_RUN_CASE_PASS:
		if (verbosity > 0) {
			printf("pass\n");
		}
		break;
	case CAPTEST_RUN_CASE_FAIL:
		if (verbosity == 0) {
			printf("/%s/ <- \"%s%s\": FAIL\n",
			    testcase->regex, testcase->input,
			    trailing_newline ? "\\n" : "");
		}
		if (verbosity > 0) {
			printf("FAIL\n");
		}
		break;
	case CAPTEST_RUN_CASE_ERROR:
		printf("ERROR\n");
		break;
	}

	if (dump_captures) {
		for (size_t i = 0; i < testcase->count; i++) {
			printf("exp %zd, %zd, got %zd, %zd%s\n",
			    testcase->expected[i].pos[0], testcase->expected[i].pos[1],
			    capture_buf[i].pos[0], capture_buf[i].pos[1],
			    (testcase->expected[i].pos[0] != capture_buf[i].pos[0] ||
				testcase->expected[i].pos[1] != capture_buf[i].pos[1])
			    ? " *" : "");
		}
	}

	fsm_free(fsm);

	return res;
}

enum captest_run_case_res
captest_run_case_multi(const struct captest_case_multi *testcase,
    int verbosity, bool trailing_newline, FILE *prog_output,
    struct captest_case_multi_result *result)
{
	struct re_err err;
	const enum re_flags flags = 0;

	struct captest_case_multi_result ignored_result;
	if (result == NULL) {
		result = &ignored_result;
	}
	memset(result, 0x00, sizeof(*result));

	if (verbosity == DUMP_PROGRAMS_VERBOSITY) {
		assert(prog_output != NULL);
	} else {
		assert(prog_output == NULL);
	}

	/* build each regex, combining them and keeping track of capture offsets */
	struct fsm *fsms[testcase->regex_count];
	struct fsm_combined_base_pair bases[testcase->regex_count];
	struct fsm *combined_fsm = NULL;

	for (size_t i = 0; i < testcase->regex_count; i++) {
		fsms[i] = NULL;
	}

	/* compile each individually */
	for (size_t i = 0; i < testcase->regex_count; i++) {
		struct captest_input comp_input = {
			.string = testcase->regexes[i],
		};

		if (verbosity > 1) {
			fprintf(stderr, "%s: compiling \"%s\"\n",
			    __func__, comp_input.string);
		}

		struct fsm *fsm = re_comp(RE_PCRE,
		    captest_getc, &comp_input,
		    &options, flags, &err);
		assert(fsm != NULL);

		if (!fsm_determinise(fsm)) {
			goto cleanup;
		}

		if (!fsm_minimise(fsm)) {
			goto cleanup;
		}

		if (verbosity > 3) {
			char tag_buf[16] = { 0 };
			snprintf(tag_buf, sizeof(tag_buf), "fsm[%zu]", i);

			fprintf(stderr, "==== fsm[%zu]\n", i);
			fsm_print_fsm(stderr, fsm);
			fsm_capture_dump(stderr, tag_buf, fsm);
		}

		fsms[i] = fsm;
	}

	combined_fsm = fsm_union_array(testcase->regex_count, fsms, bases);
	assert(combined_fsm != NULL);
	if (verbosity > 0) {
		fprintf(stderr, "%s: combined_fsm: %d states after fsm_union_array\n",
		    __func__, fsm_countstates(combined_fsm));
	}
	if (verbosity > 1) {
		for (size_t i = 0; i < testcase->regex_count; i++) {
			fprintf(stderr, "%s: base[%zu]: state %d, capture %u\n",
			    __func__, i, bases[i].state, bases[i].capture);
		}
	}

	if (!fsm_determinise(combined_fsm)) {
		goto cleanup;
	}
	if (verbosity > 0) {
		fprintf(stderr, "%s: combined_fsm: %d states after determinise\n",
		    __func__, fsm_countstates(combined_fsm));
	}

	if (!fsm_minimise(combined_fsm)) {
		goto cleanup;
	}
	if (verbosity > 0) {
		fprintf(stderr, "%s: combined_fsm: %d states after minimise\n",
		    __func__, fsm_countstates(combined_fsm));
	}

	/* If verbosity is exactly 9, then print out capture info and pass. */
	if (verbosity == DUMP_PROGRAMS_VERBOSITY) {
		fsm_capture_dump(prog_output, "capture_info", combined_fsm);
		fsm_free(combined_fsm);
		return CAPTEST_RUN_CASE_PASS;
	}

	if (verbosity > 3) {
		fprintf(stderr, "==== combined\n");
		fsm_print_fsm(stderr, combined_fsm);
		fsm_capture_dump(stderr, "combined", combined_fsm);
	}

	/* for each input, execute and check result */
	const struct multi_case_input_info *info;
	for (info = &testcase->inputs[0]; info->input != NULL; info++) {
		if (trailing_newline) {
			const size_t length = strlen(info->input);
			assert(length + 1 < MAX_INPUT_WITH_NL_LENGTH);
			memcpy(input_with_nl, info->input,
			    length);
			input_with_nl[length] = '\n';
			input_with_nl[length + 1] = '\0';
		}

		const char *input = trailing_newline
		    ? input_with_nl
		    : info->input;
		assert(input != NULL);
		const size_t length = strlen(input);

		if (verbosity > 1) {
			fprintf(stderr, "%s: input: %s\n", __func__, input);
		}

		fsm_state_t end;	/* unused but required by API */
		struct fsm_capture capture_buf[MAX_CAPTEST_MULTI_CAPTURE_PAIRS];
		const size_t capture_buf_length = MAX_CAPTEST_MULTI_CAPTURE_PAIRS;
		for (size_t i = 0; i < capture_buf_length; i++) {
			capture_buf[i].pos[0] = (size_t)-2;
			capture_buf[i].pos[1] = (size_t)-3;
		}

		/* execute and check result & captures */
		int exec_res = fsm_exec_with_captures(combined_fsm,
		    (const unsigned char *)input, length, &end, capture_buf, capture_buf_length);
		if (exec_res == -1) {
			perror("fsm_exec_with_captures");
			return CAPTEST_RUN_CASE_ERROR;
		}

		/* The .regex field should be in ascending order so we know
		 * when we've reached the all-0 suffix of expected[]. */
		uint8_t prev_regex = 0;
		for (const struct case_multi_expected *exp = &info->expected[0];
		     exp->regex >= prev_regex; exp++) {
			prev_regex = exp->regex;
			bool match = true;
			const unsigned capture_base = bases[exp->regex].capture;
			const unsigned capture_id = capture_base + exp->capture;
			assert(capture_id < MAX_CAPTEST_MULTI_CAPTURE_PAIRS);
			const size_t exp_s = exp->pos[0];
			const size_t exp_e = exp->pos[1];
			const size_t got_s = capture_buf[capture_id].pos[0];
			const size_t got_e = capture_buf[capture_id].pos[1];
			if (exp_s == got_s && exp_e == got_e) {
				result->pass++;
			} else {
				match = false;
				result->fail++;
			}

			if (!match || verbosity > 2) {
				fprintf(stderr, "%s: regex %u, capture %u (%u + base %u), exp (%zd, %zd), got (%zd, %zd)%s\n",
				    __func__, exp->regex,
				    capture_id, exp->capture, capture_base,
				    exp_s, exp_e, got_s, got_e,
				    match ? "" : " *** mismatch ***");
			}
		}
	}

	fsm_free(combined_fsm);

	/* this could populate a result struct so it can pass/fail multiple inputs */

	return result->fail == 0
	    ? CAPTEST_RUN_CASE_PASS
	    : CAPTEST_RUN_CASE_FAIL;

cleanup:
	if (combined_fsm != NULL) {
		fsm_free(combined_fsm);
	} else {
		for (size_t i = 0; i < testcase->regex_count; i++) {
			if (fsms[i] != NULL) {
				fsm_free(fsms[i]);
			}
		}
	}

	return CAPTEST_RUN_CASE_ERROR;
}

static struct capvm_program *
get_program_copy(const struct captest_case_program *testcase)
{
	static struct capvm_program prog;
	static struct capvm_opcode ops[MAX_PROGRAM_OPS + 1] = { 0 };
	static struct capvm_char_class cc_sets[MAX_PROGRAM_CHAR_CLASSES] = { 0 };

	memset(&prog, 0x00, sizeof(prog));

	memcpy(ops, testcase->ops,
	    MAX_PROGRAM_OPS * sizeof(testcase->ops[0]));
	memcpy(cc_sets, testcase->char_class,
	    MAX_PROGRAM_CHAR_CLASSES * sizeof(testcase->char_class[0]));

	assert(testcase->expected.count < MAX_CAPTEST_PROGRAM_CAPTURE_PAIRS);
	prog.capture_count = testcase->expected.count;
	prog.capture_base = testcase->expected.base;

	uint32_t max_cc_used = (uint32_t)-1;

	prog.used = MAX_PROGRAM_OPS;
	for (size_t i = 0; i < MAX_PROGRAM_OPS; i++) {
		const struct capvm_opcode *op = &testcase->ops[i];
		if (op->t == CAPVM_OP_CHAR && op->u.chr == 0x00) {
			prog.used = i;
			break;
		} else if (op->t == CAPVM_OP_CHARCLASS) {
			if (max_cc_used == (uint32_t)-1 || op->u.charclass_id > max_cc_used) {
				assert(op->u.charclass_id < MAX_PROGRAM_CHAR_CLASSES);
				max_cc_used = op->u.charclass_id;
			}
		}
	}

	prog.ceil = MAX_PROGRAM_OPS;
	prog.ops = ops;

	prog.char_classes.sets = cc_sets;
	prog.char_classes.count = max_cc_used == (uint32_t)-1 ? 0 : max_cc_used + 1;
	prog.char_classes.ceil = MAX_PROGRAM_CHAR_CLASSES;

	return &prog;
}

enum captest_run_case_res
captest_run_case_program(const struct captest_case_program *testcase,
    int verbosity)
{
	(void)verbosity;

	/* copy program */
	const size_t input_length = strlen(testcase->input);
	struct fsm_capture capture_buf[MAX_CAPTEST_PROGRAM_CAPTURE_PAIRS];
	const size_t capture_buf_length = MAX_CAPTEST_PROGRAM_CAPTURE_PAIRS;

	/* Initialize with FSM_CAPTURE_NO_POS, as the caller would */
	for (size_t i = 0; i < capture_buf_length; i++) {
		capture_buf[i].pos[0] = FSM_CAPTURE_NO_POS;
		capture_buf[i].pos[1] = FSM_CAPTURE_NO_POS;
	}

	struct capvm_program *program = get_program_copy(testcase);

	if (verbosity > 2) {
		fsm_capvm_program_dump(stderr, program);
	}

	fsm_capvm_program_exec(program, (const uint8_t *)testcase->input, input_length,
	    capture_buf, capture_buf_length);

	bool dump_captures = false;
	enum captest_run_case_res res = CAPTEST_RUN_CASE_PASS;

	/* check captures against expected */
	for (size_t i = 0; i < testcase->expected.count; i++) {
		if (testcase->expected.captures[i].pos[0] != capture_buf[i].pos[0] ||
		    testcase->expected.captures[i].pos[1] != capture_buf[i].pos[1]) {
			res = CAPTEST_RUN_CASE_FAIL;
			dump_captures = true;
		}
	}

	if (dump_captures) {
		for (size_t i = 0; i < testcase->expected.count; i++) {
			printf("exp %zd, %zd, got %zd, %zd%s\n",
			    testcase->expected.captures[i].pos[0],
			    testcase->expected.captures[i].pos[1],
			    capture_buf[i].pos[0], capture_buf[i].pos[1],
			    (testcase->expected.captures[i].pos[0] != capture_buf[i].pos[0] ||
				testcase->expected.captures[i].pos[1] != capture_buf[i].pos[1])
			    ? " *" : "");
		}
	}

	return res;
}
