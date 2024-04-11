#include "endids_codegen.h"

static const struct endids_codegen_testcase testcase = {
	/* no overlap */
	.patterns = {
		{ .pattern = "ab", },
		{ .pattern = "ac", },
		{ .pattern = "ad", },
		{ .pattern = "ae", },
		{ .pattern = "af", },
	},
};

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;
	return endids_codegen_generate(&testcase);
}
