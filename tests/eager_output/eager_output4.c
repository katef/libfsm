#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  { "abcde$" },
		.inputs = {
			{ .input = "abcde", .expected_ids = { 1 } },
			{ .input = "Xabcde", .expected_ids = { 1 } },
		},
	};
	return run_test(&test);
}
