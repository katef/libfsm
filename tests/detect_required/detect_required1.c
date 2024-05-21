#include "testutil.h"

const struct testcase tests[] = {
	{ .regex = "^$", .required = "" },
	{ .regex = "^a$", .required = "a" },
	{ .regex = "^abcde$", .required = "abcde" },
	{ .regex = "^(ab|cd)$", .required = "" },
	{ .regex = "^(ab|cd|ef)$", .required = "" },
	{ .regex = "^(abc|def)$", .required = "" },
	{ .regex = "^(abc|dbf)$", .required = "b" },
	{ .regex = "^abc(def)*ghi$", .required = "abcghi" },
	{ .regex = "^abc(def)+ghi$", .required = "abcdefghi" },
	{ .regex = "^ghi(def)abc$", .required = "abcdefghi" },
};

int main()
{
	const bool first_fail = getenv("FIRST_FAIL") != NULL;
	const size_t testcount = sizeof(tests)/sizeof(tests[0]);

	size_t failures = 0;
	for (size_t i = 0; i < testcount; i++) {
		if (!run_test(&tests[i])) {
			failures++;
			if (first_fail) { break; }
		}
	}

	return failures == 0
	    ? EXIT_SUCCESS
	    : EXIT_FAILURE;
}
