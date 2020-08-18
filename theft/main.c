/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include <unistd.h>
#include <getopt.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <adt/xalloc.h>

#include "type_info_fsm_literal.h"

#define EXEC_NAME "fuzz_libfsm"

enum test_type {
	TEST0,
	TEST1,
	TEST2,
	TEST3
};

struct test_link {
	struct test_link *next;
	const char *name;
	enum test_type type;
	union {
		struct {
			test_fun *fun;
		} test0;
		struct {
			test_fun1 *fun;
			uintptr_t arg;
		} test1;
		struct {
			test_fun2 *fun;
			uintptr_t arg0;
			uintptr_t arg1;
		} test2;
		struct {
			test_fun3 *fun;
			uintptr_t arg0;
			uintptr_t arg1;
			uintptr_t arg2;
		} test3;
	} u;
};

struct state {
	size_t test_count;
	size_t pass;
	size_t fail;
	const char *filter;
	int verbosity;
	bool first_fail;
	bool list;

	struct test_link *tests;
};

static struct state state;

static void
usage(const char *exec_name)
{
	fprintf(stderr,
		"Usage: %s [-hvlf] [-n <NAME_FILTER>]\n"
		"  -h:		   help\n"
		"  -v:		   increase verbosity\n"
		"  -l:		   list tests by name\n"
		"  -f:		   halt after first failure\n"
		"  -n <STRING>:    only run tests with STRING in the name\n"
		"  -s <SEED>:	   fuzz from the given seed\n",
		exec_name);
	exit(1);
}

static theft_seed
parse_seed(const char *s)
{
	unsigned long long u;
	char *e;

	errno = 0;

	u = strtoull(s, &e, 16);
	if (u == ULLONG_MAX && errno != 0) {
		perror(optarg);
		exit(1);
	}

	/* XXX: assumes theft_seed is uint64_t */
	if (u > UINT64_MAX) {
		fprintf(stderr, "seed out of range\n");
		exit(1);
	}

	if (s[0] == '\0' || *e != '\0') {
		fprintf(stderr, "invalid seed\n");
		exit(1);
	}

	return u;
}

void
reg_test(const char *name, test_fun *test)
{
	struct test_link *link;

	link = xmalloc(sizeof *link);

	*link = (struct test_link) {
		.next = state.tests,
		.name = name,
		.type = TEST0,
		.u.test0.fun = test,
	};

	state.tests = link;
}

void
reg_test1(const char *name, test_fun1 *test, uintptr_t arg)
{
	struct test_link *link;

	link = xmalloc(sizeof *link);

	*link = (struct test_link) {
		.next = state.tests,
		.name = name,
		.type = TEST1,
		.u.test1 = {
			.fun = test,
			.arg = arg,
		},
	};

	state.tests = link;
}

void
reg_test2(const char *name, test_fun2 *test,
    uintptr_t arg0, uintptr_t arg1)
{
	struct test_link *link;

	link = xmalloc(sizeof *link);

	*link = (struct test_link) {
		.next = state.tests,
		.name = name,
		.type = TEST2,
		.u.test2 = {
			.fun = test,
			.arg0 = arg0,
			.arg1 = arg1,
		},
	};

	state.tests = link;
}

void reg_test3(const char *name, test_fun3 *test,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2)
{
	struct test_link *link;

	link = xmalloc(sizeof *link);

	*link = (struct test_link) {
		.next = state.tests,
		.name = name,
		.type = TEST3,
		.u.test3 = {
			.fun = test,
			.arg0 = arg0,
			.arg1 = arg1,
			.arg2 = arg2,
		},
	};

	state.tests = link;
}

int
test_get_verbosity(void)
{
	return state.verbosity;
}

#define CHECK(CALL)						   \
	do {								   \
		tests++;						   \
		if (CALL) {						   \
			pass++;						   \
		} else {						   \
			fail++;						   \
		}								   \
   	} while (0)

int
main(int argc, char **argv)
{
	struct test_link *link;
	theft_seed seed;

	seed = theft_seed_of_time();

	{
		 int c;

		 while (c = getopt(argc, argv, "hvfln:s:"), c != -1) {
			 switch (c) {
			 case 'v': state.verbosity++;         break;
			 case 'l': state.list       = true;   break;
			 case 'f': state.first_fail = true;   break;
			 case 'n': state.filter     = optarg; break;
			 case 's': seed = parse_seed(optarg); break;

			 case 'h':
			 case '?':
			 default:
				 usage(argv[0]);
				 break;
			 }
		 }
	}

	register_test_adt_edge_set();
	register_test_literals();
	register_test_re();
	register_test_adt_priq();
	register_test_adt_set();
	register_test_nfa();
	register_test_trim();
	register_test_minimise();

	for (link = state.tests; link; link = link->next) {
		bool hit, pass;

		hit = false;
		if (state.filter != NULL) {
			size_t offset = 0;
			size_t filter_len = strlen(state.filter);
			while (link->name[offset] != '\0') {
				if (0 == strncmp(&link->name[offset], state.filter, filter_len)) {
					hit = true;
					break;
				}
				offset++;
			}
		} else {
			hit = true; /* no filter */
		}

		if (!hit) {
			continue;
		}

		if (state.verbosity > 0 || state.list) {
			printf("== TEST %s\n", link->name);
		}

		if (state.list) {
			continue;
		}

		pass = false;
		switch (link->type) {
		case TEST0: pass = link->u.test0.fun(seed);
			break;
		case TEST1: pass = link->u.test1.fun(seed,
		    link->u.test1.arg);
			break;
		case TEST2: pass = link->u.test2.fun(seed,
		    link->u.test2.arg0, link->u.test2.arg1);
			break;
		case TEST3: pass = link->u.test3.fun(seed,
		    link->u.test3.arg0, link->u.test3.arg1, link->u.test3.arg2);
			break;

		default:
			assert(false);
			break;
		}

		state.test_count++;
		if (pass) {
			state.pass++;
			if (state.verbosity > 0) {
				printf("PASS %s\n", link->name);
			} else {
				printf(".");
			}
		} else {
			state.fail++;
			if (state.verbosity > 0) {
				printf("FAIL %s\n", link->name);
			} else {
				printf("F");
			}
			if (state.first_fail) {
				break;
			}
		}
	}

	printf("\n\n-- pass %zd, fail %zd, tests %zd\n",
		state.pass, state.fail, state.test_count);

	return (state.fail == 0 ? 0 : 1);
}

