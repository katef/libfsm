#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"abcx+",
		},
		.inputs = {
			{ .input = "abc", .expect_fail = true },
			{ .input = "abcx", .expected_ids = { 1 } },
			{ .input = "abcxx", .expected_ids = { 1 } },
			{ .input = "z", .expect_fail = true },
		},
	};

	return run_test(&test);
}
