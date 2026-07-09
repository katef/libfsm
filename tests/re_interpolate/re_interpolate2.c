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
test(const char *fmt, enum re_interpolate_flags flags, bool expected)
{
	bool r;

	assert(fmt != NULL);

	r = re_interpolate(fmt, '$', flags, "<g0>", 0, NULL, "<ne>", NULL, 0, NULL, NULL);

	failed += r != expected;

	printf("%s/%d => %d%s\n", fmt, 0, r,
		r != expected ? " XXX" : "");
}

int main(void) {
	test("", 0, true);
	test("abc", 0, true);
	test("$$", 0, true);
	test("$x", 0, false);
	test("{", 0, true);
	test("}", 0, true);

	test("{", RE_INTERPOLATE_BRACES, true);
	test("}", RE_INTERPOLATE_BRACES, true);

	test("${", RE_INTERPOLATE_BRACES, false);
	test("${}", RE_INTERPOLATE_BRACES, false);
	test("$}{", RE_INTERPOLATE_BRACES, false);
	test("{$}", RE_INTERPOLATE_BRACES, false);

	return failed;
}

