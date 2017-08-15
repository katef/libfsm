#include "test_libfsm.h"

#include <unistd.h>
#include <getopt.h>

#include "type_info_fsm_literal.h"

#define EXEC_NAME "test_libfsm"

enum test_type {
	TEST0,
	TEST1,
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

static void usage(void) {
	fprintf(stderr, "Usage: " EXEC_NAME " [-v] [-f] [-n testname]\n");
	exit(1);
}

void reg_test(const char *name, test_fun *test)
{
	struct test_link *link = calloc(1, sizeof(*link));
	assert(link);
	*link = (struct test_link) {
		.next = state.tests,
		.name = name,
		.type = TEST0,
		.u.test0.fun = test,
	};

	state.tests = link;
}

void reg_test1(const char *name, test_fun1 *test, uintptr_t arg)
{
	struct test_link *link = calloc(1, sizeof(*link));
	assert(link);
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

int test_get_verbosity(void) { return state.verbosity; }

#define CHECK(CALL)						       \
	do {							       \
		tests++;					       \
		if (CALL) {					       \
			pass++;					       \
		} else {					       \
			fail++;					       \
		}					               \
       	} while (0)

static void parse_args(int argc, char **argv, struct state *s) {
     int ch;

     while ((ch = getopt(argc, argv, "hvfln:")) != -1) {
             switch (ch) {
             case '?':
             default:
	     case 'h':
		     usage();
		     break;
             case 'v':
                     s->verbosity++;
                     break;
	     case 'l':
		     s->list = true;
		     break;
	     case 'f':
		     s->first_fail = true;
		     break;
             case 'n':
		     s->filter = optarg;
                     break;
             }
     }
}

static void register_tests(void) {
	register_test_literals();
	register_test_re();
	register_test_adt_priq();
	register_test_adt_set();
	register_test_nfa();
}

int main(int argc, char **argv)
{
	parse_args(argc, argv, &state);

	register_tests();

	for (struct test_link *link = state.tests; link; link = link->next) {
		bool hit = false;
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

		if (hit) {
			if (state.verbosity > 0 || state.list) {
				printf("== TEST %s\n", link->name);
			}

			if (state.list) { continue; }

			bool pass = false;
			switch (link->type) {
			case TEST0:
				pass = link->u.test0.fun();
				break;
			case TEST1:
				pass = link->u.test1.fun(link->u.test1.arg);
				break;
			default:
				assert(false);
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
	}

	printf("\n\n-- pass %zd, fail %zd, tests %zd\n",
	    state.pass, state.fail, state.test_count);
	return (state.fail == 0 ? 0 : 1);
}
