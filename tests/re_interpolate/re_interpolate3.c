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
test(enum re_interpolate_flags flags, const char *fmt, size_t groupc, const char *groupv[], const char *expected)
{
	char outs[50];
	bool r;

	assert(fmt != NULL);
	assert(expected != NULL);

	if (!re_interpolate(fmt, '$', flags, "<g0>", groupc, groupv, "<ne>", outs, sizeof outs, NULL, NULL)) {
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

	test(RE_INTERPOLATE_SINGLE_DIGIT, "", 0, g0, "");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "", 4, gn, "");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "x", 0, g0, "x");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "x", 4, gn, "x");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "\001", 0, g0, "\001");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "\001", 4, gn, "\001");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "$01", 0, g0, "<g0>1");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "$01", 4, gn, "<g0>1");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "$001", 0, g0, "<g0>01");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "$001", 4, gn, "<g0>01");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "$0", 0, gn, "<g0>");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "x$000000000000000000000x", 0, gn, "x<g0>00000000000000000000x");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "x$000000000000000000001x", 1, gn, "x<g0>00000000000000000001x");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "x$900000000000000000000x", 1, gn, "x<ne>00000000000000000000x");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "x$100000000000000000000x", 1, gn, "xone00000000000000000000x");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "$$$11$11$22$11$33$44$33$22$11$$$$", 4, gn, "$one1one1two2one1three3four4three3two2one1$$");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "$$$$$$$$$$$$$$$$$$$$", 4, gn, "$$$$$$$$$$");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 4, gn, "xyz_one..<g0>003;three,$.one-four=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 3, gn, "xyz_one..<g0>003;three,$.one-<ne>=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 2, gn, "xyz_one..<g0>003;<ne>,$.one-<ne>=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 1, gn, "xyz_one..<g0>003;<ne>,$.one-<ne>=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 0, g0, "xyz_<ne>..<g0>003;<ne>,$.<ne>-<ne>=<ne>23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 1, ga, "xyz_1..<g0>003;<ne>,$.1-<ne>=123");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$0003;$3,$$.$1-$4=$123", 1, gb, "xyz_..<g0>003;<ne>,$.-<ne>=23");

	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 4, gn, "xyz_one..two003;three,$.one-four=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 3, gn, "xyz_one..two003;three,$.one-<ne>=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 2, gn, "xyz_one..two003;<ne>,$.one-<ne>=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 1, gn, "xyz_one..<ne>003;<ne>,$.one-<ne>=one23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 0, g0, "xyz_<ne>..<ne>003;<ne>,$.<ne>-<ne>=<ne>23");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 1, ga, "xyz_1..<ne>003;<ne>,$.1-<ne>=123");
	test(RE_INTERPOLATE_SINGLE_DIGIT, "xyz_$1..$2003;$3,$$.$1-$4=$123", 1, gb, "xyz_..<ne>003;<ne>,$.-<ne>=23");

	return failed;
}

