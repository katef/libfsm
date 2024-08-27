#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  { "^abc$", "^ab*c$" },
		.inputs = {
			{ .input = "ac", .expected_ids = { 2 } },
			{ .input = "abc", .expected_ids = { 1, 2 } },
			{ .input = "abbc", .expected_ids = { 2 } },
		},
	};
	return run_test(&test, false, false);
}
