#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"(^|[^A-Z])abc",
		},
		.inputs = {
			{ .input = "abc", .expected_ids = { 1 } },
			{ .input = "xabc", .expected_ids = { 1 } },
			{ .input = "xyz abc", .expected_ids = { 1 } },
			{ .input = "Xabc", .expect_fail = true },
		},
	};

	return run_test(&test);
}
