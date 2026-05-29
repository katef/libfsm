#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"abc(xy)+",
			"z",
		},
		.inputs = {
			{ .input = "abc", .expect_fail = true },
			{ .input = "abcxy", .expected_ids = { 1 } },
			{ .input = "abcxyxy", .expected_ids = { 1 } },
			{ .input = "z", .expected_ids = { 2 } },
		},
	};

	return run_test(&test);
}
