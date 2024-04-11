#include "endids_codegen.h"

struct scanner {
	const uint8_t *str;
	size_t size;
	size_t offset;
};

static int
scanner_next(void *opaque)
{
	struct scanner *s;
	unsigned char c;

	s = opaque;

	if (s->offset == s->size) {
		return EOF;
	}

	c = s->str[s->offset];
	s->offset++;

	return (int) c;
}

static struct fsm *
compile(const char *pattern, unsigned pattern_id, struct fsm_options *opt)
{
	const size_t length = strlen(pattern);
	struct scanner s = {
		.str    = (const uint8_t *)pattern,
		.size   = length,
		.offset = 0
	};

	struct re_err err;
	struct fsm *fsm = re_comp(RE_PCRE, scanner_next, &s, opt, RE_MULTI, &err);
	if (fsm == NULL) { return NULL; }
	if (!fsm_setendid(fsm, pattern_id)) { return NULL; }
	if (!fsm_determinise(fsm)) { return NULL; }
	if (!fsm_minimise(fsm)) { return NULL; }
	return fsm;
}

static int
matches(const struct fsm *fsm, const char *input, fsm_state_t *endstate)
{
	const size_t length = strlen(input);
	struct scanner s = {
		.str    = (const uint8_t *)input,
		.size   = length,
		.offset = 0
	};
	return fsm_exec(fsm, scanner_next, &s, endstate, NULL);
}

/* Don't just return the end state for matches, also pass along matching endids. */
static int
endleaf_cb(FILE *out, const struct fsm_end_ids *ids,
	    const void *endleaf_opaque)
{
	fprintf(out, "\n\t\t{\n"
	    "\t\t\tconst fsm_end_id_t endids[] = { ");

	for (size_t i = 0; i < ids->count; i++) {
		fprintf(out, "%u, ", ids->ids[i]);
	}
	fprintf(out, "};\n");
	fprintf(out, "\t\t\tsave_endids(endids, %u);\n", ids->count);
	fprintf(out, "\t\t\treturn 1;\n");
	fprintf(out, "\t\t}\n");
	
	(void)endleaf_opaque;
	return 0;
}

static bool
needs_escape(char c, char **escaped)
{
	switch (c) {
	case '"':
		*escaped = "\\\"";
		return true;
	case '\n':
		*escaped = "\\n";
		return true;
	case '\\':
		*escaped = "\\\\";
		return true;
	default:
		return false;
	}
}

static void
fprintf_escaped(FILE *out, const char *str)
{
	const char *c = str;
	char *escaped = NULL;
	while (*c) {
		if (needs_escape(*c, &escaped)) {
			fprintf(out, "%s", escaped);
		} else {
			fprintf(out, "%c", *c);
		}
		c++;
	}
}

int
endids_codegen_generate(const struct endids_codegen_testcase *testcase)
{
	struct fsm *fsms[ENDIDS_CODEGEN_MAX_PATTERNS] = {0};

	struct fsm_options opt = {
		.io = FSM_IO_STR,
		.endleaf = endleaf_cb,
	};
	bool should_match[ENDIDS_CODEGEN_MAX_PATTERNS][ENDIDS_CODEGEN_MAX_PATTERNS] = {0};

	size_t fsm_count = 0;

	/* compile each individually */
	for (size_t i = 0; i < ENDIDS_CODEGEN_MAX_PATTERNS; i++) {
		const char *pattern = testcase->patterns[i].pattern;
		if (pattern == NULL) { break; }
		struct fsm *fsm = compile(pattern, i, &opt);
		assert(fsm != NULL);
		fsms[i] = fsm;
		fsm_count++;
	}

	/* check inputs match */
	for (size_t f_i = 0; f_i < fsm_count; f_i++) {
		const struct fsm *fsm = fsms[f_i];
		for (size_t i_i = 0; i_i < fsm_count; i_i++) {
			fsm_state_t endstate;
			const char *input = testcase->patterns[i_i].input
			    ? testcase->patterns[i_i].input
			    : testcase->patterns[i_i].pattern;
			if (matches(fsm, input, &endstate)) {
				fsm_end_id_t id_buf[ENDIDS_CODEGEN_MAX_PATTERNS];
				size_t written;
				enum fsm_getendids_res endid_res = fsm_getendids(fsm, endstate,
				    ENDIDS_CODEGEN_MAX_PATTERNS, id_buf, &written);
				assert(endid_res == FSM_GETENDIDS_FOUND);
				should_match[f_i][i_i] = true;
			}
		}
	}

	/* combine, det, min */
	struct fsm_combined_base_pair bases[ENDIDS_CODEGEN_MAX_PATTERNS] = {0};
	struct fsm *combined = fsm_union_array(fsm_count, fsms, bases);
	assert(combined != NULL);
	if (!fsm_determinise(combined)) { assert(!"det combined"); }
	if (!fsm_minimise(combined)) { assert(!"min combined"); }

	/* check expected inputs match on combined */
	for (size_t f_i = 0; f_i < fsm_count; f_i++) {
		for (size_t i_i = 0; i_i < fsm_count; i_i++) {
			fsm_state_t endstate;
			const char *input = testcase->patterns[i_i].input
			    ? testcase->patterns[i_i].input
			    : testcase->patterns[i_i].pattern;
			if (matches(combined, input, &endstate)) {
				fsm_end_id_t id_buf[ENDIDS_CODEGEN_MAX_PATTERNS];
				size_t written;
				enum fsm_getendids_res endid_res = fsm_getendids(combined, endstate,
				    ENDIDS_CODEGEN_MAX_PATTERNS, id_buf, &written);
				assert(endid_res == FSM_GETENDIDS_FOUND);

				size_t hits = 0;
				for (size_t e_i = 0; e_i < fsm_count; e_i++) {
					if (should_match[e_i][i_i]) { hits++; }
				}
				assert(hits == written);

				for (size_t w_i = 0; w_i < written; w_i++) {
					fsm_end_id_t end_id = id_buf[w_i];
					assert(should_match[end_id][i_i]);
				}
			}
		}
	}

	/* output headers */
	printf(
		"#include <stdlib.h>\n"
		"#include <stdio.h>\n"
		"#include <stdint.h>\n"
		"#include <stdbool.h>\n"
		"#include <assert.h>\n"
		"#include <fsm/fsm.h>\n"
		"\n");

	/* output the input list */
	printf("static const char *inputs[] = {\n");
	for (size_t i_i = 0; i_i < fsm_count; i_i++) {
		const char *input = testcase->patterns[i_i].input
		    ? testcase->patterns[i_i].input
		    : testcase->patterns[i_i].pattern;
		printf("\t\"");
		fprintf_escaped(stdout, input);
		printf("\",\n");
	}
	printf("};\n\n");

	/* endid buffer */
	printf("static fsm_end_id_t endid_buffer[%zu];\n"
	    "static size_t endid_count = 0;\n"
	    "\n", fsm_count);

	printf("static void\n"
	    "save_endids(const fsm_end_id_t *endids, size_t count)\n"
	    "{\n"
	    "\tfor (size_t i = 0; i < count; i++) {\n"
	    "\t\tendid_buffer[i] = endids[i];\n"
	    "\t}\n"
	    "\tendid_count = count;\n"
	    "}\n"
	    "\n");

	/* output input -> expected endids table */
	printf("struct expected_group_ids {\n"
	    "\tfsm_end_id_t ids[%zu];\n"
	    "\tsize_t count;\n"
	    "} expected_group_ids[%zu] = {\n", fsm_count, fsm_count);
	
	for (size_t i_i = 0; i_i < fsm_count; i_i++) {
		printf("\t{\n");
		size_t hits = 0;
		printf("\t\t.ids = {");
		for (size_t p_i = 0; p_i < fsm_count; p_i++) {
			if (should_match[p_i][i_i]) {
				printf(" %zu,", p_i);
				hits++;
			}
		}
		printf("},\n");
		printf("\t\t.count = %zu,\n", hits);
		printf("\t},\n");
	}	
	printf("};\n\n");

	printf("static void\n"
	    "check_endids(size_t input_id)\n"
	    "{\n"
	    "\tassert(endid_count == expected_group_ids[input_id].count);\n"
	    "\tfor (size_t i = 0; i < endid_count; i++) {\n"
	    "\t\tassert(endid_buffer[i] == expected_group_ids[input_id].ids[i]);\n"
	    "\t}\n"
	    "}\n"
	    "\n");

	/* output fsm */
	fsm_print_c(stdout, combined);

	/* output runner function */
	printf("\n"
	    "int\n"
	    "run_generated_code(void)\n"
	    "{\n"
	    "\t\n"
	    "\tfor (size_t i_i = 0; i_i < %zu; i_i++) {\n"
	    "\t\tint res = fsm_main(inputs[i_i]);\n"
	    "\t\tassert(res == 1);\n"
	    "\t\tcheck_endids(i_i);\n"
	    "\t}\n"
	    "\t\n"
	    "\tprintf(\"pass\\n\");\n"
	    "\treturn EXIT_SUCCESS;\n"
	    "}\n\n", fsm_count);

	fsm_free(combined);
	return EXIT_SUCCESS;
}
