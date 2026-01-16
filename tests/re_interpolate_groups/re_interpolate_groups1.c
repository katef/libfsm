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
test_err(const char *fmt, size_t groupc, const char *groupv[], const char *ne, unsigned expected_pos)
{
	struct re_pos pos;
	char outs[10];
	bool r;

	assert(fmt != NULL);

	/* for these tests we're expecting to error */
	if (re_interpolate_groups(fmt, '$', "<g0>", groupc, groupv, ne, outs, sizeof outs, &pos)) {
		printf("%s/%zu XXX\n", fmt, groupc);
		failed++;
		return;
	}

	failed += r = expected_pos != pos.byte;

	printf("%s/%zu => :%u :%u%s\n", fmt, groupc,
		pos.byte, expected_pos,
		r ? " XXX" : "");
}

int main(void) {
	const char *ne = "<ne>";

	const char *gn[] = { "one", "two", "three", "four" };
	const char **g0 = NULL;
	const char *ga[] = { "1" };
	const char *gb[] = { "" };
//	const char *gc[] = { NULL }; // XXX: not permitted

	test_err("$", 0, g0, ne, 1);
	test_err("$x", 0, g0, ne, 1);
	test_err("$ ", 4, gn, ne, 1);
	test_err("$\\01", 0, g0, ne, 1);

	test_err("$$$x", 4, gn, ne, 3);

	test_err("xyz$1", 0, gn, NULL, 5);
	test_err("xyz$2", 1, gn, NULL, 5);

	test_err("01234567890", 1, gn, ne, 10);

	return failed;
}

