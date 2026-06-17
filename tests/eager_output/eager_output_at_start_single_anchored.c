#include "utils.h"

int main(void)
{
	const struct eager_output_test test_unanchored_start = {
		.patterns =  { "$" },
		.inputs = {
			{ .input = "", .expected_ids = { 1 } },
		},
	};

	const struct eager_output_test test_anchored_start = {
		.patterns =  { "^$" },
		.inputs = {
			{ .input = "", .expected_ids = { 1 } },
		},
	};

	bool pass = run_test(&test_unanchored_start);
	pass = run_test(&test_anchored_start) && pass;
	return pass;
}
