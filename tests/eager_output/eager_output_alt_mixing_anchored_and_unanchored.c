#include "utils.h"

/* Test for false positive matches when combining patterns
 * with both anchored and unanchored subtrees. */

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"a(?:x|$)",
			"a(?:y|$)",
		},
		.inputs = {
			{ .input = "a", .expected_ids = { 1, 2 } },
			{ .input = "aZ", .expect_fail = true },
			{ .input = "Za", .expected_ids = { 1, 2 } },
			{ .input = "ax", .expected_ids = { 1 } },
			{ .input = "axZ", .expected_ids = { 1 } },
			{ .input = "ay", .expected_ids = { 2 } },
			{ .input = "ayZ", .expected_ids = { 2 } },
			{ .input = "axa", .expected_ids = { 1, 2 } },
			{ .input = "aya", .expected_ids = { 1, 2 } },
			{ .input = "axay", .expected_ids = { 1, 2 } },
			{ .input = "ayax", .expected_ids = { 1, 2 } },
		},
	};

	return run_test(&test, false, false);
}
