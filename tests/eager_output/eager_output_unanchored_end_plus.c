#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"abcx+",
			"z",
		},
		.inputs = {
			{ .input = "abc", .expect_fail = true },
			{ .input = "abcx", .expected_ids = { 1 } },
			{ .input = "abcxx", .expected_ids = { 1 } },
			{ .input = "z", .expected_ids = { 2 } },
		},
	};

	return run_test(&test);
}
