#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  { "ab(c|d|e)" },
		.inputs = {
			{ .input = "abc", .expected_ids = { 1 } },
			{ .input = "abd", .expected_ids = { 1 } },
			{ .input = "abe", .expected_ids = { 1 } },
			{ .input = "Xabe", .expected_ids = { 1 } },
			{ .input = "abeX", .expected_ids = { 1 } },
			{ .input = "XabeX", .expected_ids = { 1 } },
		},
	};
	return run_test(&test);
}
