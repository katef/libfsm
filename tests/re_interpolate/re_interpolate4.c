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

	test(RE_INTERPOLATE_BRACES, "${5}", 4, gn, "<ne>");

	test(RE_INTERPOLATE_BRACES, "${0}", 4, gn, "<g0>");
	test(RE_INTERPOLATE_BRACES, "_${0}", 4, gn, "_<g0>");
	test(RE_INTERPOLATE_BRACES, "_${0}_", 4, gn, "_<g0>_");
	test(RE_INTERPOLATE_BRACES, "${1}", 4, gn, "one");
	test(RE_INTERPOLATE_BRACES, "${10}", 4, gn, "<ne>");
	test(RE_INTERPOLATE_BRACES, "${10}0", 4, gn, "<ne>0");
	test(RE_INTERPOLATE_BRACES, "${10}0x", 4, gn, "<ne>0x");
	test(RE_INTERPOLATE_BRACES, "${10}1", 4, gn, "<ne>1");
	test(RE_INTERPOLATE_BRACES, "${10}1x", 4, gn, "<ne>1x");
	test(RE_INTERPOLATE_BRACES, "${1}0", 4, gn, "one0");
	test(RE_INTERPOLATE_BRACES, "0${1}0", 4, gn, "0one0");
	test(RE_INTERPOLATE_BRACES, "0${0}0", 4, gn, "0<g0>0");
	test(RE_INTERPOLATE_BRACES, "${1}1", 4, gn, "one1");
	test(RE_INTERPOLATE_BRACES, "1${1}", 4, gn, "1one");
	test(RE_INTERPOLATE_BRACES, "1${1}1", 4, gn, "1one1");
	test(RE_INTERPOLATE_BRACES, "_${1}", 4, gn, "_one");
	test(RE_INTERPOLATE_BRACES, "_${1}_", 4, gn, "_one_");
	test(RE_INTERPOLATE_BRACES, "x${1}", 4, gn, "xone");
	test(RE_INTERPOLATE_BRACES, "${1}x", 4, gn, "onex");
	test(RE_INTERPOLATE_BRACES, "${01}", 4, gn, "one");
	test(RE_INTERPOLATE_BRACES, "${002}", 4, gn, "two");
	test(RE_INTERPOLATE_BRACES, "${0003}", 4, gn, "three");
	test(RE_INTERPOLATE_BRACES, "${4000}", 4, gn, "<ne>");

	test(RE_INTERPOLATE_BRACES, "${00000000000000001}", 4, gn, "one");
	test(RE_INTERPOLATE_BRACES, "${00000000000000000000000000000000000000000000000001}", 4, gn, "one");

	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${5}", 4, gn, "<ne>");

	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${0}", 4, gn, "<g0>");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "_${0}", 4, gn, "_<g0>");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "_${0}_", 4, gn, "_<g0>_");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${1}", 4, gn, "one");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${10}", 4, gn, "<ne>");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${10}0", 4, gn, "<ne>0");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${10}0x", 4, gn, "<ne>0x");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${10}1", 4, gn, "<ne>1");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${10}1x", 4, gn, "<ne>1x");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${1}0", 4, gn, "one0");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "0${1}0", 4, gn, "0one0");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "0${0}0", 4, gn, "0<g0>0");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${1}1", 4, gn, "one1");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "1${1}", 4, gn, "1one");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "1${1}1", 4, gn, "1one1");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "_${1}", 4, gn, "_one");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "_${1}_", 4, gn, "_one_");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "x${1}", 4, gn, "xone");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${1}x", 4, gn, "onex");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${01}", 4, gn, "one");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${002}", 4, gn, "two");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${0003}", 4, gn, "three");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${4000}", 4, gn, "<ne>");

	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${00000000000000001}", 4, gn, "one");
	test(RE_INTERPOLATE_BRACES | RE_INTERPOLATE_SINGLE_DIGIT, "${00000000000000000000000000000000000000000000000001}", 4, gn, "one");

	return failed;
}

