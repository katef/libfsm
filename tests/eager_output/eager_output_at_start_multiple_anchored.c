#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns = {
			[0] = "$",
			[1] = "^$",
		},
		.inputs = {
			{ .input = "", .expected_ids = { 1, 2 } },
		},
	};
	return run_test(&test, false, false);
}
