#include "utils.h"

/* Regression: This is a case that requires an anchored_start state set
 * rather than a single optional anchored_start state ID to link
 * correctly. */

int main(void)
{
	struct eager_output_test test = {
		.patterns =  {
			"(^|wax-)((?:banana|^apple))",
			"(^|wax-)(orange)",
		},
		.inputs = {
			{ .input = "banana", .expected_ids = { 1 } },
		},
	};

	return run_test(&test);
}
