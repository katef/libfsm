#include "utils.h"

/* Fuzzer regresison */
int main(void)
{
	struct eager_output_test test = {
		.patterns =  { "", "" },
		.inputs = {
			{ .input = "", .expected_ids = { 1, 2 } },
		},
	};
	return run_test(&test);
}
