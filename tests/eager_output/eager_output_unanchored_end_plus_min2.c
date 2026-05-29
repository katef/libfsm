#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"z",
		},
		.inputs = {
			{ .input = "abc", .expect_fail = true },
			{ .input = "z", .expected_ids = { 1 } },
			{ .input = "Xz", .expected_ids = { 1 } },
			{ .input = "XzX", .expected_ids = { 1 } },
			{ .input = "zX", .expected_ids = { 1 } },
		},
	};

	return run_test(&test);
}
