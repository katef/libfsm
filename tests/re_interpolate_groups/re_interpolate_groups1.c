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
test_err(const char *fmt, size_t groupc, const char *groupv[], const char *ne,
	unsigned expected_start, unsigned expected_end)
{
	struct re_pos start, end;
	char outs[10];
	bool rs, re;

	assert(fmt != NULL);

	/* for these tests we're expecting to error */
	if (re_interpolate_groups(fmt, '$', "<g0>", groupc, groupv, ne, outs, sizeof outs, &start, &end)) {
		printf("%s/%zu XXX\n", fmt, groupc);
		failed++;
		return;
	}

	failed += rs = expected_start != start.byte;
	failed += re = expected_end   != end.byte;

	printf("%s/%zu => :%u-%u :%u-%u '%.*s'%s\n", fmt, groupc,
		start.byte, end.byte,
		expected_start, expected_end,
		(int) (end.byte - start.byte), fmt + start.byte,
		(rs || re) ? " XXX" : "");
}

int main(void) {
	const char *ne = "<ne>";

	const char *gn[] = { "one", "two", "three", "four" };
	const char **g0 = NULL;

	test_err("$", 0, g0, ne, 0, 1);
	test_err("$x", 0, g0, ne, 0, 1);
	test_err("$ ", 4, gn, ne, 0, 1);
	test_err("$\\01", 0, g0, ne, 0, 1);

	test_err("$0$", 0, g0, ne, 2, 3);
	test_err("$$$x", 4, gn, ne, 2, 3);

	test_err("xyz$1", 0, gn, NULL, 3, 5);
	test_err("xyz$2", 1, gn, NULL, 3, 5);

	test_err("01234567890", 1, gn, ne, 0, 10);
	test_err("$$$$$$$$$$$$$$$$$$$$", 1, gn, ne, 0, 20);
	test_err("$1$1$1$$", 1, gn, ne, 0, 8);
	test_err("$1$1$1x", 1, gn, ne, 0, 7);
	test_err("xxxyyyzzz$$", 1, gn, ne, 0, 11);

	return failed;
}

