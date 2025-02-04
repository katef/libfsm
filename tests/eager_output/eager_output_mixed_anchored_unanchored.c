#include "utils.h"

int main(void)
{
	/* fprintf(stderr, "%s: skipping for now, this doesn't pass yet.\n", __FILE__); */
	/* return EXIT_SUCCESS; */

	struct eager_output_test test = {
		.patterns =  {
			"^abc$",
			"def",
			"^ghi",
			"jkl$",
			"mno",
		},
		.inputs = {
			{ .input = "abc", .expected_ids = { 1 } },
			{ .input = "def", .expected_ids = { 2 } },
			{ .input = "ghi", .expected_ids = { 3 } },
			{ .input = "jkl", .expected_ids = { 4 } },
			{ .input = "mno", .expected_ids = { 5 } },

			{ .input = "defmno", .expected_ids = { 2, 5 } },
			{ .input = " def mno ", .expected_ids = { 2, 5 } },

			/* Matching a start-anchored pattern followed by
			 * unanchored ones should just work. */
			{ .input = "ghi def", .expected_ids = { 2, 3 } },

			/* An unanchored pattern before a start-anchored pattern
			 * should only match the unanchored pattern. */
			{ .input = "def ghi", .expected_ids = { 2 } },

			/* Matching an unanchored pattern before an
			 * end-anchored one is fine. */
			{ .input = "mno jkl", .expected_ids = { 4, 5 } },

			/* This should match "mno" with the "jkl" prefix
			 * ignored by the unanchored start, which does
			 * not count as a match for "jkl$". */
			{ .input = "jkl mno", .expected_ids = { 5 } },
		},
	};

	return run_test(&test);
}
