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
test(const char *fmt, bool expected)
{
	bool r;

	assert(fmt != NULL);

	r = re_interpolate_groups(fmt, '$', "<g0>", 0, NULL, "<ne>", NULL, 0, NULL, NULL);

	failed += r != expected;

	printf("%s/%d => %d%s\n", fmt, 0, r,
		r != expected ? " XXX" : "");
}

int main(void) {
	test("", true);
	test("abc", true);
	test("$$", true);
	test("$x", false);

	return failed;
}

