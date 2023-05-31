#include "captest.h"

#include <getopt.h>

#define NO_POS FSM_CAPTURE_NO_POS

const struct captest_case_single single_cases[] = {
	{
		.regex = "^",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$^",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$^", .input = "x",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "()*",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "()*",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^$",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^($|($)|(($))|((($))))",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(((($)))|(($))|($)|$)",
		.input = "",
		.count = 5, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^((((a$)))|((b$))|(c$)|d$)",
		.input = "a",
		.count = 5, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^((((a$)))|((b$))|(c$)|d$)",
		.input = "b",
		.count = 7, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {-1, -1}, },
			{ .pos = {-1, -1}, },
			{ .pos = {-1, -1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^((((b$)))|((b$))|(c$)|d$)",
		.input = "b",
		.count = 5, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^((x?))*$",
		.input = "x",
		.count = 3, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "^((x?)*)*$",
		.input = "",
		.count = 3, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^((x?)*)*$",
		.input = "xxxxx",
		.count = 3, .expected = {
			{ .pos = {0, 5}, },
			{ .pos = {5, 5}, },
			{ .pos = {5, 5}, },
		},
	},
	{
		.regex = "xx*x",
		.input = "xx",
		.count = 1, .expected = {
			{ .pos = {0, 2}, },
		},
	},
	{
		.regex = "^(x?)*$",
		.input = "xx",
		.count = 2, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {2, 2}, },
		},
	},
	{
		.regex = "^(x?)*$",
		.input = "xxx",
		.count = 2, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {3, 3}, },
		},
	},
	{
		.regex = "^(x?)+$",
		.input = "xx",
		.count = 2, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {2, 2}, },
		},
	},
	{
		.regex = "^(x?)+$",
		.input = "xxx",
		.count = 2, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {3, 3}, },
		},
	},
	{
		.regex = "^x(z?)*y$",
		.input = "xy",
		.count = 2, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "()|x",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "()|x",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "x|()",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "x|()",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$|",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = ".|$^",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = ".|$^",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "$^|.",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$^|.",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "$$$^|...",
		.input = "xxx",
		.count = 1, .expected = {
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "x?$x?^x?|x?$x?^x?",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "[^x]", .input = "",
		.no_nl = true,
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "[^x]",
		.input = "\n",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = ".$()",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = ".$()", .input = "",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "^.$()",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "^.$()", .input = "",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$(x?)(y?)(z?)",
		.input = "a",
		.count = 4, .expected = {
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = ".$(x?)(y?)(z?)",
		.input = "a",
		.count = 4, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "[^y]",
		.input = "xx",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = ".",
		.input = "xx",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "(x)+",
		.input = "xxx",
		.count = 2, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {2, 3}, },
		},
	},
	{
		.regex = "^(x)*.",
		.input = "xx",
		.count = 2, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^(x)*.",
		.input = "xy",
		.count = 2, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "a.b(c)*",
		.input = "axbc",
		.count = 2, .expected = {
			{ .pos = {0, 4}, },
			{ .pos = {3, 4}, },
		},
	},
	{
		.regex = "^x?^",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^x?^",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$(^)",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "($)",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "($$$)",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$x?^", .input = "x",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$(^)*",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$(^)*",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "$()*",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$()*",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "^$^", .input = "x",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$^$", .input = "x",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$y?^x*", .input = "x",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "x|$^",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "x|$^", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "x|$^$^",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "x|$^$^", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$^|x",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "$^|x", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$^$^|x",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "$^$^|x", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "^$|.",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "x|^$^$", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "^$^$|x", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "$|^|a$",
		.input = "x",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "[^a]x", .input = "x",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "[^a]x",
		.input = "xx",
		.count = 1, .expected = {
			{ .pos = {0, 2}, },
		},
	},
	{
		.regex = "a(b|c$)d", .input = "ac",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "a(^b|c)d", .input = "bd",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "(a|b|)*",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "xx*y$",
		.input = "x_xxy",
		.count = 1, .expected = {
			{ .pos = {2, 5}, },
		},
	},
	{
		.regex = "(|.$)*",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "(.$)*x", .input = "y",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "(.$)*",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^(|.$)*",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(|.$)*$",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "x|y(^)", .input = "",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "(?:x*.|^$).", .input = "",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "(?:x|^$)x", .input = "",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "()+x",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "($$)^",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "$($|$a)",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(?i)abc$",
		.input = "AbC",
		.count = 1, .expected = {
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "^(?i)ab(?-i)c$", .input = "AbC",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "^(?i)ab(?-i)c$",
		.input = "Abc",
		.count = 1, .expected = {
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "^(?i)a[b]c$",
		.input = "ABC",
		.count = 1, .expected = {
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "^(?i)a[^b]c$", .input = "ABC",
		.match = SHOULD_NOT_MATCH,
	},
	{
		.regex = "^(?i)a[bx]c$",
		.input = "ABC",
		.count = 1, .expected = {
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "^(?i)a[b-c]c$",
		.input = "ABC",
		.count = 1, .expected = {
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "(a()b)+a",
		.input = "a!aba",
		.count = 3, .expected = {
			{ .pos = {2, 5}, },
			{ .pos = {2, 4}, },
			{ .pos = {3, 3}, },
		},
	},
	{
		.regex = "^^[^]]",
		.input = "\n",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "x(x()y)*",
		.input = "xxy",
		.count = 3, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {1, 3}, },
			{ .pos = {2, 2}, },
		},
	},
	{
		.regex = "x(()x)*",
		.input = "xx",
		.count = 3, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {1, 2}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "b(x*x*a()*y)*(a)a*",
		.input = "ba",
		.count = 4, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {-1, -1}, },
			{ .pos = {-1, -1}, },
			{ .pos = {1, 2}, },
		},
	},
	{
		.regex = "a(().x)*ab",
		.input = "a.a.aaxab",
		.count = 3, .expected = {
			{ .pos = {4, 9}, },
			{ .pos = {5, 7}, },
			{ .pos = {5, 5}, },
		},
	},
	{
		.regex = "ab(b()*()*)*()*z",
		.input = "a!abz",
		.count = 5, .expected = {
			{ .pos = {2, 5}, },
			{ .pos = {-1, -1}, },
			{ .pos = {-1, -1}, },
			{ .pos = {-1, -1}, },
			{ .pos = {4, 4}, },
		},
	},
	{
		.regex = "^x(y?z*)*$",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "^(y?z*)*$",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(x|$x?)*$",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(^|$x)*$",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "((x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*(x?)*)*y$",
		.input = "xxxxxxxxxxy",
		.count = 15, .expected = {
			{ .pos = {0, 11}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
			{ .pos = {10, 10}, },
		},
	},
	{
		.regex = "^a$",
		.input = "a",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^a(bcd)e$",
		.input = "abcde",
		.count = 2, .expected = {
			{ .pos = {0, 5}, },
			{ .pos = {1, 4}, },
		},
	},
	{
		.regex = "^(a(b((c))(d)))$",
		.input = "abcd",
		.count = 6, .expected = {
			{ .pos = {0, 4}, },
			{ .pos = {0, 4}, },
			{ .pos = {1, 4}, },
			{ .pos = {2, 3}, },
			{ .pos = {2, 3}, },
			{ .pos = {3, 4}, },
		},
	},
	{
		.regex = "^(a(b(c)))$",
		.input = "abc",
		.count = 4, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {0, 3}, },
			{ .pos = {1, 3}, },
			{ .pos = {2, 3}, },
		},
	},
	{
		.regex = "^a(b*)(c)$",
		.input = "ac",
		.count = 3, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 2}, },
		},
	},
	{
		.regex = "^a(b*)(c)$",
		.input = "abc",
		.count = 3, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {1, 2}, },
			{ .pos = {2, 3}, },
		},
	},
	{
		.regex = "^a(b*)(c)$",
		.input = "abbc",
		.count = 3, .expected = {
			{ .pos = {0, 4}, },
			{ .pos = {1, 3}, },
			{ .pos = {3, 4}, },
		},
	},
	{
		.regex = "^(ab*c)$",
		.input = "ac",
		.count = 2, .expected = {
			{ .pos = {0, 2}, },
			{ .pos = {0, 2}, },
		},
	},
	{
		.regex = "^(ab*c)$",
		.input = "abc",
		.count = 2, .expected = {
			{ .pos = {0, 3}, },
			{ .pos = {0, 3}, },
		},
	},
	{
		.regex = "^(ab*c)$",
		.input = "abbc",
		.count = 2, .expected = {
			{ .pos = {0, 4}, },
			{ .pos = {0, 4}, },
		},
	},
	{
		.regex = "^(a*)",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(a*)",
		.input = "x",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(a*)",
		.input = "a",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^(a*)",
		.input = "ax",
		.count = 2, .expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^a*",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^a*",
		.input = "a",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "^a*",
		.input = "ax",
		.count = 1, .expected = {
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = ".|",
		.input = "",
		.count = 1, .expected = {
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "()*^",
		.input = "",
		.count = 2, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "(((())))*^",
		.input = "",
		.count = 5, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},

	{
		.regex = "(x|(x|))^",
		.input = "",
		.count = 3, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = ".*(x|())^",
		.input = "",
		.count = 3, .expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "(()|(()|x)^|x)^",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},

	{
		.regex = "x^()()|()",
		.input = "",
		.count = 4,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "y^()|()^x",
		.input = "x",
		.count = 3,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "()$a|()",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "()$z|(x)$",
		.input = "x",
		.count = 3,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 1}, },
		},
	},

	{
		/* long enough to exercise the USE_COLLAPSED_ZERO_PREFIX optimization */
		.regex = "a*(ba*)c$",
		.input = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac",
		.count = 2,
		.expected = {
			{ .pos = {101, 303}, },
			{ .pos = {201, 302}, },
		},
	},

	/* regression: losing the first character on the transition from
	 * the unanchored start loop to the capture */
	{
		.regex = "aa+b$",
		.input = "aXaXaaab",
		.count = 1,
		.expected = {
			{ .pos = {4, 8}, },
		},
	},
	{
		.regex = "aa*b$",
		.input = "aXaXaaab",
		.count = 1,
		.expected = {
			{ .pos = {4, 8}, },
		},
	},
	{
		.regex = "!!!+$",
		.input = "!\"!\"!\"!!!!",
		.count = 1,
		.expected = {
			{ .pos = {6, 10}, },
		},
	},

	/* new fuzzer regressions */
	{
		/* PCRE does not set the first capture, which is unsatisfiable */
		.regex = "^(.^)*^(a*)",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		/* similar to the previous case, but with different anchoring */
		.regex = "(a)*(^)*^",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(.a)*^(.a)",
		.input = "!a",
		.count = 3,
		.expected = {
			{ .pos = {0, 2}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 2}, },
		},
	},
	{
		.regex = "(A)*^()*^",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},

	{
		.regex = "(a(b*)*|)*bc",
		.input = "b!bc",
		.count = 3,
		.expected = {
			{ .pos = {2, 4}, },
			{ .pos = {2, 2}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		.regex = "^(a(b*)*|)*bc$",
		.input = "bc",
		.count = 3,
		.expected = {
			{ .pos = {0, 2}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		.regex = "(|a((b*)*b*))*",
		.input = "",
		.count = 4,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		/* simplified version of the above */
		.regex = "^(|a(b*)*)*$",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		/* zero repetitions should not set the capture */
		.regex = "^(a)*$",
		.input = "",
		.count = 2,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		.regex = "^(a)*(^)$",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		/* raw fuzzer output */
	        .regex = "()((()(^|$|$^|^|$|$^^|$|$^|^|$|$^^^^|^|(|)($)|)+|^^|^|(|)($)|)+|)($)()+",
		.input = "",
		.count = 12,
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "(^|())+()",
		.input = "",
		.count = 4,
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "(?:(^|^$)+|)+",
		.input = "",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 2,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^((|)($)|)+a$",
		.input = "a",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 4,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		.regex = "^(($)|)+a$",
		.input = "a",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 3,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},
	{
		.regex = "^(|(|x))*$",
		.input = "x",
		.count = 3,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		/* same as the previous but without outer capture */
		.regex = "^(?:|(|x))*$",
		.input = "x",
		.count = 2,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},
	{
		.regex = "(((($)|)+|)a|)+",
		.input = "",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 5,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
			{ .pos = {NO_POS, NO_POS}, },
		},
	},

	{
		.regex = "^(|(|(|x)))*$",
		.input = "x",
		.count = 4,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {0, 1}, },
			{ .pos = {0, 1}, },
		},
	},


	{
		.regex = "^(?:(?:(x?)^)y?)+$",
		.input = "",
		.count = 2,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(?:^())+$",
		.input = "",
		.count = 2,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(?:($|x))+$",
		.input = "x",

		.match = SHOULD_REJECT_AS_UNSUPPORTED,

		.count = 2,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "^(($)|x)+$",
		.input = "x",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 3,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},
	{
		.regex = "^(?:()?^()?)+$",
		.input = "",
		.count = 3,
		.expected = {
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
			{ .pos = {0, 0}, },
		},
	},
	{
		.regex = "^(?:($|x)())+$",
		.input = "x",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 3,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},

	{
		.regex = "()~((|)($)|%)+",
		.input = "~%",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 5,
		.expected = {
			{ .pos = {0, 2}, },
			{ .pos = {0, 0}, },
			{ .pos = {2, 2}, },
			{ .pos = {2, 2}, },
			{ .pos = {2, 2}, },
		},
	},

	{
		/* (slightly) reduced version of the previous */
		.regex = "^(()($)|x)+$",
		.input = "x",
		.match = SHOULD_REJECT_AS_UNSUPPORTED,
		.count = 4,
		.expected = {
			{ .pos = {0, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
			{ .pos = {1, 1}, },
		},
	},

	{
		.regex = "a|_$[^b]",
		.input = "a",
		.count = 1,
		.expected = {
			{ .pos = {0, 1}, },
		},
	},
};

const struct captest_case_multi multi_cases[] = {
	{
		.regex_count = 4,
		.regexes = {
			"^aa$",	  /* exactly two 'a's */
			"^a*",    /* zero or more 'a's followed by anything */
			"^ab?$",  /* 'a' and optionally 'b' */
			"a*$",    /* anything ending in zero or more 'a's */
		},
		.inputs = {
			{
				.input = "",
				.expected = {
					{ .regex = 0, .pos = POS_NONE },
					{ .regex = 1, .pos = { 0, 0 } },
					{ .regex = 2, .pos = POS_NONE },
					{ .regex = 3, .pos = { 0, 0 } },
				},
			},

			{
				.input = "a",
				.expected = {
					{ .regex = 0, .pos = POS_NONE },
					{ .regex = 1, .pos = { 0, 1 } },
					{ .regex = 2, .pos = { 0, 1 } },
					{ .regex = 3, .pos = { 0, 1 } },
				},
			},

			{
				.input = "aa",
				.expected = {
					{ .regex = 0, .pos = { 0, 2 } },
					{ .regex = 1, .pos = { 0, 2 } },
					{ .regex = 2, .pos = POS_NONE },
					{ .regex = 3, .pos = { 0, 2 } },
				},
			},

			{
				.input = "aaa",
				.expected = {
					{ .regex = 0, .pos = POS_NONE },
					{ .regex = 1, .pos = { 0, 3 } },
					{ .regex = 2, .pos = POS_NONE },
					{ .regex = 3, .pos = { 0, 3 } },
				},
			},

			{
				.input = "ba",
				.expected = {
					{ .regex = 0, .pos = POS_NONE },
					{ .regex = 1, .pos = { 0, 0 } },
					{ .regex = 2, .pos = POS_NONE },
					{ .regex = 3, .pos = { 1, 2 } },
				},
			},

			{
				.input = "ab",
				.expected = {
					{ .regex = 0, .pos = POS_NONE },
					{ .regex = 1, .pos = { 0, 1 } },
					{ .regex = 2, .pos = { 0, 2 } },
					{ .regex = 3, .pos = { 2, 2 } },
				},
			},

			{
				.input = NULL,
			},
		},
	},

	{
		.regex_count = 3,
		.regexes = {
			"a(b?)*c",
			"(ab)(c)",
			"ab+(c)",
		},
		.inputs = {
			{
				.input = "",
				.expected = {
					{ .regex = 0, .capture = 0, .pos = POS_NONE },
					{ .regex = 0, .capture = 1, .pos = POS_NONE },
					{ .regex = 1, .capture = 0, .pos = POS_NONE },
					{ .regex = 1, .capture = 1, .pos = POS_NONE },
					{ .regex = 1, .capture = 2, .pos = POS_NONE },
					{ .regex = 2, .capture = 0, .pos = POS_NONE },
					{ .regex = 2, .capture = 1, .pos = POS_NONE },
				},
			},
			{
				.input = "abc",
				.expected = {
					{ .regex = 0, .capture = 0, .pos = {0, 3} },
					{ .regex = 0, .capture = 1, .pos = {2, 2} },
					{ .regex = 1, .capture = 0, .pos = {0, 3} },
					{ .regex = 1, .capture = 1, .pos = {0, 2} },
					{ .regex = 1, .capture = 2, .pos = {2, 3} },
					{ .regex = 2, .capture = 0, .pos = {0, 3} },
					{ .regex = 2, .capture = 1, .pos = {2, 3} },
				},
			},
		},
	},
	{
		/* fuzzer regression: This led to an execution path in fsm_union_array,
		 * fsm_union, fsm_merge, merge that did not init or otherwise set the
		 * `struct fsm_combine_info`, leading to an out of range offset for
		 * the capture base. */
		.regex_count = 3,
		.regexes = {
			".",
			".^",
			"^^_",
		},
		.inputs = {
			{
				.input = "",
				.expected = {
					{ .regex = 0, .pos = POS_NONE },
					{ .regex = 1, .pos = POS_NONE },
					{ .regex = 2, .pos = POS_NONE },
				},
			},
			{
				.input = "_",
				.expected = {
					{ .regex = 0, .pos = { 0, 1 } },
					{ .regex = 1, .pos = { 0, 1 } },
					{ .regex = 2, .pos = { 0, 1 } },
				},
			},
		},
	},
};


static struct captest_case_program program_cases[] = {
	{
		.input = "",
		.char_class = {
			{ .octets = { ~0, ~0, ~0, ~0 }}, /* 0x00 <= x <= 0xff */
		},
		.expected = {
			.count = 4,
			.captures = {
				{ .pos = {0, 0}, },
				{ .pos = {0, 0}, },
				{ .pos = {NO_POS, NO_POS}, },
				{ .pos = {0, 0}, },
			},
		},

		.ops = {
			{ .t = CAPVM_OP_SPLIT, .u.split = { .cont = 3, .new = 1 }},
			{ .t = CAPVM_OP_CHARCLASS, .u.charclass_id = 0 },
			{ .t = CAPVM_OP_JMP, .u.jmp = 0 },
			{ .t = CAPVM_OP_SAVE, .u.save = 0 },
			{ .t = CAPVM_OP_SPLIT, .u.split = { .cont = 5, .new = 7 }},
			{ .t = CAPVM_OP_ANCHOR, .u.anchor = CAPVM_ANCHOR_START },

			{ .t = CAPVM_OP_JMP, .u.jmp = 9 }, /* jump after |() */
			{ .t = CAPVM_OP_SAVE, .u.save = 4 },
			{ .t = CAPVM_OP_SAVE, .u.save = 5 },

			{ .t = CAPVM_OP_SPLIT, .u.split = { .cont = 4, .new = 10 }},

			{ .t = CAPVM_OP_SAVE, .u.save = 2 },
			{ .t = CAPVM_OP_SAVE, .u.save = 3 },
			{ .t = CAPVM_OP_SAVE, .u.save = 6 },
			{ .t = CAPVM_OP_SAVE, .u.save = 7 },
			{ .t = CAPVM_OP_SAVE, .u.save = 1 },
			{ .t = CAPVM_OP_SPLIT, .u.split = { .cont = 18, .new = 16 }},
			{ .t = CAPVM_OP_CHARCLASS, .u.charclass_id = 0 },
			{ .t = CAPVM_OP_JMP, .u.jmp = 15 },
			{ .t = CAPVM_OP_MATCH },
		},
	},


	{
		/* correcting compilation of '^(?:($|x))+$' */
		.input = "x",
		.expected = {
			.count = 2,
			.captures = {
				{ .pos = {0, 1}, },
				{ .pos = {1, 1}, },
			},
		},

		.ops = {
			[0] = { .t = CAPVM_OP_SAVE, .u.save = 0 },
			[1] = { .t = CAPVM_OP_ANCHOR, .u.anchor = CAPVM_ANCHOR_START },
			[2] = { .t = CAPVM_OP_SAVE, .u.save = 2 },
			[3] = { .t = CAPVM_OP_SPLIT, .u.split = { .cont = 4, .new = 6 }},
			[4] = { .t = CAPVM_OP_ANCHOR, .u.anchor = CAPVM_ANCHOR_END },

		        /* [5] = { .t = CAPVM_OP_JMP, .u.jmp = 7 }, */
		        [5] = { .t = CAPVM_OP_SPLIT, .u.split = { .cont = 7, .new = 9 }},

			[6] = { .t = CAPVM_OP_CHAR, .u.chr = 'x' },
			[7] = { .t = CAPVM_OP_SAVE, .u.save = 3 },
			[8] = { .t = CAPVM_OP_SPLIT, .u.split = { .cont = 2, .new = 9 }},
			[9] = { .t = CAPVM_OP_ANCHOR, .u.anchor = CAPVM_ANCHOR_END },
			[10] = { .t = CAPVM_OP_SAVE, .u.save = 1 },
			[11] = { .t = CAPVM_OP_MATCH },
		},
	},
};

#define NO_FILTER ((size_t)-1)
struct options {
	size_t filter;
	int verbosity;
	bool track_timing;
	FILE *prog_output;
	enum groups {
		GROUP_SINGLE = 0x01,
		GROUP_MULTI = 0x02,
		GROUP_PROGRAMS = 0x04,
		GROUP_ALL = 0xff,
	} group;
};

static void
print_usage(FILE *f, const char *progname)
{
	fprintf(f, "%s: [-h] [-v] [-s | -m | -p] [-f <id>] [-t]\n", progname);
	fprintf(f, "    -h: print this usage info\n");
	fprintf(f, "    -v: increase verbosity (can repeat: -vvv)\n");
	fprintf(f, "    -f <id>: just run a specific test, by numeric ID\n");
	fprintf(f, "    -s: only single casse\n");
	fprintf(f, "    -m: only multi cases\n");
	fprintf(f, "    -p: only program cases\n");
	fprintf(f, "    -t: print timing info\n");
}

static void
get_options(struct options *opt, int argc, char **argv)
{
	const char *progname = argv[0];
	int c;
	while (c = getopt(argc, argv, "hf:mpstv"), c != -1) {
		switch (c) {
		case 'h':
			print_usage(stdout, progname);
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			opt->verbosity++;
			break;
		case 'f':
			opt->filter = atol(optarg);
			break;
		case 't':
			opt->track_timing = true;
			break;
		case 'p':
			opt->group = GROUP_PROGRAMS;
			break;
		case 's':
			opt->group = GROUP_SINGLE;
			break;
		case 'm':
			opt->group = GROUP_MULTI;
			break;
		case '?':
		default:
			print_usage(stderr, progname);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv) {
	size_t pass = 0;
	size_t fail = 0;
	size_t skip = 0;
	size_t nth = 0;

	struct options options = {
		.filter = NO_FILTER,
		.verbosity = 0,
		.group = GROUP_ALL,
	};
	get_options(&options, argc, argv);

	if (options.verbosity == DUMP_PROGRAMS_VERBOSITY) {
		options.prog_output = fopen("prog_output", "w");
		assert(options.prog_output != NULL);
	}

	/* avoid an extra layer of indentation here */
	if (!(options.group & GROUP_SINGLE)) { goto after_single; }

	printf("-- single cases without trailing newline\n");
	const size_t single_case_count = sizeof(single_cases)/sizeof(single_cases[0]);
	for (size_t c_i = 0; c_i < single_case_count; c_i++) {
		const size_t cur = nth++;
		if (options.filter != NO_FILTER && options.filter != cur) {
			continue;
		}

		if (options.verbosity > 0) {
			printf("%zu: ", cur);
			if (options.verbosity > 2) {
				fflush(stdout);
			}
		}

		if (options.verbosity == DUMP_PROGRAMS_VERBOSITY) {
			fprintf(options.prog_output, "\n\n==== test_case %zu\n", c_i);
		}

		const struct captest_case_single *t = &single_cases[c_i];

		if (t->match == SHOULD_SKIP) {
			printf("%zd: SKIP (regex \"%s\", input \"%s\")\n",
			    cur, t->regex, t->input);
			skip++;
			continue;
		}

		enum captest_run_case_res res = captest_run_case(t, options.verbosity, false, options.prog_output);

		switch (res) {
		case CAPTEST_RUN_CASE_PASS:
			pass++;
			break;
		case CAPTEST_RUN_CASE_FAIL:
			if (options.verbosity == 0) {
				printf("-- test case %zd (regex \"%s\", input \"%s\")\n", cur, t->regex, t->input);
			}
			fail++;
			break;
		case CAPTEST_RUN_CASE_ERROR:
			assert(!"error");
			return EXIT_FAILURE;
		}
	}

	/* second pass, adding a trailing newline to input */
	printf("-- single cases with trailing newline\n");
	for (size_t c_i = 0; c_i < single_case_count; c_i++) {
		const size_t cur = nth++;
		if (options.filter != NO_FILTER && options.filter != cur) {
			continue;
		}

		const struct captest_case_single *t = &single_cases[c_i];
		if (t->no_nl) { continue; }
		if (t->match == SHOULD_SKIP) {
			printf("%zd: SKIP (regex \"%s\", input \"%s\\n\")\n",
			    cur, t->regex, t->input);
			skip++;
			continue;
		}

		if (options.verbosity > 0) {
			printf("%zu: ", cur);
			if (options.verbosity > 2) {
				fflush(stdout);
			}
		}

		enum captest_run_case_res res = captest_run_case(t, options.verbosity, true, options.prog_output);

		switch (res) {
		case CAPTEST_RUN_CASE_PASS:
			pass++;
			break;
		case CAPTEST_RUN_CASE_FAIL:
			if (options.verbosity == 0) {
				printf("-- test case %zd (regex \"%s\", input \"%s\\n\")\n", cur, t->regex, t->input);
			}
			fail++;
			break;
		case CAPTEST_RUN_CASE_ERROR:
			assert(!"error");
			return EXIT_FAILURE;
		}
	}
after_single:

	/* multi-regex tests */
	if (!(options.group & GROUP_MULTI)) { goto after_multi; }

	printf("-- multi-regex cases\n");
	const size_t multi_case_count = sizeof(multi_cases)/sizeof(multi_cases[0]);
	for (size_t c_i = 0; c_i < multi_case_count; c_i++) {
		const size_t cur = nth++;
		if ((options.filter != NO_FILTER && options.filter != cur)) {
			continue;
		}

		const struct captest_case_multi *t = &multi_cases[c_i];
		if (t->match == SHOULD_SKIP) {
			printf("%zu: SKIP (multi)\n", c_i);
			skip++;
			continue;
		}

		if (options.verbosity > 0) {
			printf("%zu: ", cur);
		}

		struct captest_case_multi_result result;
		enum captest_run_case_res res = captest_run_case_multi(t,
		    options.verbosity, false, options.prog_output, &result);

		pass += result.pass;
		fail += result.fail;

		switch (res) {
		case CAPTEST_RUN_CASE_PASS:
			if (options.verbosity > 0) {
				printf("pass\n");
			}
			break;
		case CAPTEST_RUN_CASE_FAIL:
			if (options.verbosity > 0) {
				printf("FAIL\n");
			} else {
				printf("-- test case %zd\n", cur);
			}
			break;
		case CAPTEST_RUN_CASE_ERROR:
			assert(!"error");
			return EXIT_FAILURE;
		}
	}
after_multi:

	/* hardcoded programs */
	if (!(options.group & GROUP_PROGRAMS)) { goto after_programs; }

	const size_t prog_case_count = sizeof(program_cases)/sizeof(program_cases[0]);
	for (size_t c_i = 0; c_i < prog_case_count; c_i++) {
		const size_t cur = nth++;
		if ((options.filter != NO_FILTER && options.filter != cur)) {
			continue;
		}

		const struct captest_case_program *t = &program_cases[c_i];

		if (options.verbosity > 0) {
			printf("%zu: ", cur);
		}

		enum captest_run_case_res res = captest_run_case_program(t,
		    options.verbosity);

		switch (res) {
		case CAPTEST_RUN_CASE_PASS:
			if (options.verbosity > 0) {
				printf("pass\n");
			}
			pass++;
			break;
		case CAPTEST_RUN_CASE_FAIL:
			fail++;
			if (options.verbosity > 0) {
				printf("FAIL\n");
			} else if (options.verbosity == 0) {
				printf("-- test case %zd\n", cur);
			}
			break;
		case CAPTEST_RUN_CASE_ERROR:
			assert(!"error");
			return EXIT_FAILURE;
		}
	}
after_programs:

	printf("-- pass %zu, fail %zu, skip %zu\n", pass, fail, skip);

	return fail > 0
	    ? EXIT_FAILURE
	    : EXIT_SUCCESS;
}
