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

	if (!re_interpolate(fmt, '\\', RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "<g0>", groupc, groupv, "<ne>", outs, sizeof outs, NULL, NULL)) {
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

	test("", 0, g0, "");
	test("", 4, gn, "");

	test("x", 0, g0, "x");
	test("x", 4, gn, "x");
	test("{", 0, g0, "{");
	test("{", 4, gn, "{");
	test("}", 0, g0, "}");
	test("}", 4, gn, "}");

	test("\\\\", 4, gn, "\\");
	test("\\{1}", 4, gn, "one");
	test("_\\{0}", 4, gn, "_<g0>");
	test("_\\{1}", 4, gn, "_one");
	test("\\{0}_", 4, gn, "<g0>_");
	test("\\{1}_", 4, gn, "one_");
	test("_\\{0}_", 4, gn, "_<g0>_");
	test("_\\{1}_", 4, gn, "_one_");
	test("0\\{0}", 4, gn, "0<g0>");
	test("0\\{1}", 4, gn, "0one");
	test("\\{0}0", 4, gn, "<g0>0");
	test("\\{1}0", 4, gn, "one0");
	test("0\\{0}0", 4, gn, "0<g0>0");
	test("0\\{1}0", 4, gn, "0one0");
	test("1\\{0}", 4, gn, "1<g0>");
	test("1\\{1}", 4, gn, "1one");
	test("\\{0}1", 4, gn, "<g0>1");
	test("\\{1}1", 4, gn, "one1");
	test("1\\{0}1", 4, gn, "1<g0>1");
	test("1\\{11}1", 4, gn, "1<ne>1");
	test("1\\{0}", 4, gn, "1<g0>");
	test("1\\{11}", 4, gn, "1<ne>");
	test("\\{0}1", 4, gn, "<g0>1");
	test("\\{11}1", 4, gn, "<ne>1");
	test("1\\{0}1", 4, gn, "1<g0>1");
	test("1\\{11}1", 4, gn, "1<ne>1");

	test("\001", 0, g0, "\001");
	test("\001", 4, gn, "\001");

	test("\\0", 0, gn, "<g0>");
	test("_\\0_", 0, gn, "_<g0>_");
	test("x\\000000000000000000000x", 0, gn, "x<g0>00000000000000000000x");
	test("x\\000000000000000000001x", 1, gn, "x<g0>00000000000000000001x");
	test("x\\100000000000000000000x", 1, gn, "xone00000000000000000000x");

	test("\\\\\\1\\1\\2\\1\\3\\4\\3\\2\\1\\\\\\\\", 4, gn, "\\oneonetwoonethreefourthreetwoone\\\\");
	test("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\", 4, gn, "\\\\\\\\\\\\\\\\\\\\");

	return failed;
}

