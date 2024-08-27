#include "utils.h"

int main(void)
{
	struct eager_output_test test = {
		.patterns =  { "" },
		.inputs = {
			{ .input = "", .expected_ids = { 1 } },
		},
	};
	return run_test(&test, false, false);
}
