/*
 * Copyright 2026 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <re/re.h>
#include <re/groups.h>

static unsigned failed;

static void
test(const char *fmt, size_t groupc, const char *groupv[], const char *expected)
{
	char outs[40];
	bool r;

	assert(fmt != NULL);
	assert(expected != NULL);

	if (!re_interpolate_groups(fmt, '$', "<g0>", groupc, groupv, "<ne>", outs, sizeof outs, NULL, NULL)) {
		printf("%s/%zu XXX\n", fmt, groupc);
		failed++;
		return;
	}

	failed += r = 0 != strcmp(outs, expected);

	printf("%s/%zu => %s%s\n", fmt, groupc, outs,
		r ? " XXX" : "");
}

int main(void) {
	const char *gn[] = { "one", "two", "three", "four" };
	const char **g0 = NULL;
	const char *ga[] = { "1" };
	const char *gb[] = { "" };
//	const char *gc[] = { NULL }; // XXX: not permitted

	test("", 0, g0, "");
	test("", 4, gn, "");

	test("x", 0, g0, "x");
	test("x", 4, gn, "x");

	test("\001", 0, g0, "\001");
	test("\001", 4, gn, "\001");

	test("$0", 0, gn, "<g0>");
	test("x$000000000000000000000x", 0, gn, "x<g0>x");
	test("x$000000000000000000001x", 1, gn, "xonex");
	test("x$100000000000000000000x", 1, gn, "x<ne>x");

	test("$$$1$1$2$1$3$4$3$2$1$$$$", 4, gn, "$oneonetwoonethreefourthreetwoone$$");
	test("$$$$$$$$$$$$$$$$$$$$", 4, gn, "$$$$$$$$$$");

	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 4, gn, "xyz_one..three;three,$.one-four=<ne>");
	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 3, gn, "xyz_one..three;three,$.one-<ne>=<ne>");
	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 2, gn, "xyz_one..<ne>;<ne>,$.one-<ne>=<ne>");
	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 1, gn, "xyz_one..<ne>;<ne>,$.one-<ne>=<ne>");
	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 0, g0, "xyz_<ne>..<ne>;<ne>,$.<ne>-<ne>=<ne>");
	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 1, ga, "xyz_1..<ne>;<ne>,$.1-<ne>=<ne>");
	test("xyz_$1..$0003;$3,$$.$1-$4=$123", 1, gb, "xyz_..<ne>;<ne>,$.-<ne>=<ne>");

	return failed;
}

