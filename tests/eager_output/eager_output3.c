#include "utils.h"

/* test that eager endids are correctly propagated through fsm_determinise() and fsm_minimise() */
int main(void)
{
	struct eager_output_test test = {
		.patterns =  { "ab(c|d|e)?" },
		.inputs = {
			{ .input = "ab", .expected_ids = { 1 } },
			{ .input = "abc", .expected_ids = { 1 } },
			{ .input = "abd", .expected_ids = { 1 } },
			{ .input = "abe", .expected_ids = { 1 } },
		},
	};
	return run_test(&test, false, false);
}
