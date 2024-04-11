#include "endids_codegen.h"

static const struct endids_codegen_testcase testcase = {
	/* partial overlap */
	.patterns = {
		{ .pattern = "^ab*c$", .input="abc" },
		{ .pattern = "^ac$", .input="ac" },
		{ .pattern = "^abc$", .input="abc" },
		{ .pattern = "^abbc$", .input="abbc"},
	},
};

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;
	return endids_codegen_generate(&testcase);
}
