/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#if __linux__
/* apparently you need this for Linux, and it breaks macOS */
#  define _POSIX_C_SOURCE 200809L
#endif

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/vm.h>

#include <re/re.h>

#include <adt/xalloc.h>

#include "libfsm/internal.h" /* XXX */
#include "libre/print.h" /* XXX */
#include "libre/class.h" /* XXX */
#include "libre/ast.h" /* XXX */

#include "runner.h"

/* maybe enough for a big regex */
#define BUF_LEN (4096 * 1024)

#define DEBUG_ESCAPES     0
#define DEBUG_VM_FSM      0
#define DEBUG_TEST_REGEXP 0

/* reperf driver file
 *
 * Each line starts with an instruction and is followed by a space and then an
 * argument.
 *
 * # <comment>			comment line.  ignored.
 *
 * - <test_name>		starts test named <name>.  resets all settings.
 *
 * D <dialect>			sets the regexp dialect
 *
 * M <regexp>			sets the regexp
 *
 * S <string>			if the previous line was not an 'S' line, then
 * 				sets matching to against a string and sets the
 *				string buffer to <string>.  if the previous line
 *				was an 'S' line, appends a newline and <string>
 *				to the string buffer.
 *
 * F <file>			sets matching against a file.  sets the file to
 * 				<file>
 *
 * N <count>			match is run <count> times
 *
 * R <count>			expects <count> matches.  <count> can be zero.
 *
 * X [<subtest_name>]		executes current match.  if <subtest_name> is
 * 				given, timing / correctness are recorded as
 * 				<test_name>.<subtest_name>, otherwise
 * 				<test_name>
 *
 * Lines ending with \ are continued to the next line.
 * Both F and S directives may be omitted to measure compilation time only,
 * in which case X should still be given.
 */

static struct fsm_options opt;
static struct fsm_vm_compile_opts vm_opts = { 0, FSM_VM_COMPILE_VM_V1, NULL };

enum match_type {
	MATCH_NONE,
	MATCH_STRING,
	MATCH_FILE
};

enum halt {
	HALT_AFTER_COMPILE,
	HALT_AFTER_GLUSHKOVISE,
	HALT_AFTER_DETERMINISE,
	HALT_AFTER_MINIMISE,
	HALT_AFTER_EXECUTION
};

struct str {
	char *data;
	size_t len;
	size_t cap;
};

struct perf_case {
	struct str test_name;
	struct str regexp;
	struct str match;

	size_t line;
	int count;
	int expected_matches;
	enum re_dialect dialect;
	enum match_type mt;
	enum implementation impl;
};

struct timing {
	double comp_delta;
	double glush_delta;
	double det_delta;
	double min_delta;
	double run_delta;
};

static struct str
str_empty(void)
{
	static const struct str zero;
	return zero;
}

static void
str_init(struct str *s, size_t cap)
{
	s->data = xmalloc(cap);
	s->len = 0;
	s->cap = cap;
}

static void
str_clear(struct str *s)
{
	s->len = 0;
	if (s->cap > 0) {
		s->data[0] = '\0';
	}
}

static void
report_delta(double *delta,
	const struct timespec *t0, const struct timespec *t1)
{
	double dsec = difftime(t1->tv_sec, t0->tv_sec);
	double dns  = t1->tv_nsec - t0->tv_nsec;
	*delta = dsec + dns * 1e-9;
}

static void
str_append(struct str *s, const char *str)
{
	size_t len, newlen;

	len = strlen(str);
	newlen = s->len + len + 1;

	if (newlen > s->cap) {
		s->data = xrealloc(s->data, newlen);
		s->cap = newlen;
	}

	memcpy(&s->data[s->len], str, len+1);
	s->len += len;
}

static void
str_set(struct str *s, const char *str)
{
	size_t len = strlen(str);
	if (len + 1 > s->cap) {
		size_t newcap = len+1;
		s->data = xrealloc(s->data, newcap);
		s->cap = newcap;
	}

	memcpy(s->data, str, len+1);
	s->len = len;
}

static void
str_addch(struct str *s, char ch) {
	if (s->len+1 >= s->cap) {
		size_t newcap;

		if (s->cap < 16) {
			newcap = 32;
		} else if (s->cap < 256) {
			newcap = s->cap * 2;
		} else {
			newcap = s->cap + s->cap/2;
		}

		s->data = xrealloc(s->data, newcap);
		s->cap = newcap;
	}

	s->data[s->len++] = ch;
	s->data[s->len] = '\0';
}

static void
str_free(struct str *s)
{
	free(s->data);
	s->data = NULL;
	s->len = s->cap = 0;
}

static void
rstrip(char *s, size_t *lenp)
{
	char *e = &s[*lenp-1];
	while (e > s && isspace((unsigned char)(*e))) {
		*e = '\0';
		(*lenp)--;
	}
}

static char *
lstrip(char *s)
{
	while (isspace((unsigned char)(*s))) {
		s++;
	}

	return s;
}

static enum re_dialect
dialect_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum re_dialect dialect;
	} a[] = {
		{ "like",    RE_LIKE    },
		{ "literal", RE_LITERAL },
		{ "glob",    RE_GLOB    },
		{ "native",  RE_NATIVE  },
		{ "pcre",    RE_PCRE    },
		{ "sql",     RE_SQL     }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].dialect;
		}
	}

	fprintf(stderr, "unrecognised regexp dialect \"%s\"; valid dialects are: ", name);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static void
perf_case_reset(struct perf_case *c)
{
	str_clear(&c->test_name);
	str_clear(&c->regexp);
	str_clear(&c->match);

	c->count     = 1;
	c->dialect   = RE_NATIVE;
	c->mt        = MATCH_NONE;
	c->expected_matches = 1;
}

static void
perf_case_init(struct perf_case *c, enum implementation impl)
{
	c->test_name = str_empty();
	c->regexp    = str_empty();
	c->match     = str_empty();

	c->count     = 1;
	c->dialect   = RE_NATIVE;
	c->mt        = MATCH_NONE;
	c->impl      = impl;

	c->expected_matches = 1;
}

static enum error_type
perf_case_run(struct perf_case *c, enum halt halt,
	struct timing *t);

static void
perf_case_report_txt(struct perf_case *c, enum halt halt,
	enum error_type err, int quiet,
	const struct timing *t);

static void
perf_case_report_head(int quiet, enum halt halt);

static void
perf_case_report_tsv(struct perf_case *c, enum halt halt,
	enum error_type err, int quiet,
	const struct timing *t);

static void
perf_case_report_error(enum error_type err);

static int
parse_perf_case(FILE *f, enum implementation impl, enum halt halt, int quiet, int tsv)
{
	size_t line;
	char *buf;
	char *s;
	char lastcmd;
	struct perf_case c;
	struct str *scont;
	enum error_type err;
	struct timing t;

	scont = NULL;
	line = 0;
	lastcmd = 0;
	perf_case_init(&c, impl);

	buf = xmalloc(BUF_LEN);

	while (s = fgets(buf, BUF_LEN, f), s != NULL) {
		char last;
		char *b;
		size_t len;

		line++;
		len = strlen(s);

		if (len == BUF_LEN - 1) {
			fprintf(stderr, "line %zu: overflow\n", line);
			exit(EXIT_FAILURE);
		}

		if (scont) {
			struct str *sc = scont;
			scont = NULL;

			if (s[len-1] == '\n') {
				s[--len] = '\0';
			}

			if (len == 0) {
				continue;
			}

			if (s[len-1] == '\\') {
				s[--len] = '\0';
				scont = sc;
			}

			str_append(sc, s);

			/* append line to string buffer */
			continue;
		}

		last = lastcmd;
		lastcmd = s[0];

		if (len == 0) {
			continue;
		}

		/* get rid of newline */
		if (s[len-1] == '\n') {
			s[len-1] = '\0';
			len--;
			if (len == 0) {
				continue;
			}
		}

		if (s[0] == '#') {
			continue;
		}

		switch (s[0]) {
		case '#':
			/* comment, skip */
			continue;

		case '-':
			/* strip trailing whitespace */
			rstrip(s, &len);
			str_set(&c.test_name, lstrip(&s[1]));
			c.line = line;
			break;

		case 'D':
			/* parse dialect */
			rstrip(s, &len);
			c.dialect = dialect_name(lstrip(&s[1]));
			break;

		case 'M':
			b = &s[1];
			if (*b == ' ') {
				b++;
			}
			str_set(&c.regexp, b);
			break;

		case 'S':
			b = &s[1];
			if (*b == ' ') {
				b++;
			}

			if (s[len-1] == '\\') {
				s[len-1] = '\0';
				len--;
				scont = &c.match;
			}

			if (last != 'S') {
				str_set(&c.match, b);
			} else {
				str_addch(&c.match, '\n');
				str_append(&c.match, b);
			}

			c.mt = MATCH_STRING;
			break;

		case 'F':
			rstrip(s, &len);
			str_set(&c.match, lstrip(&s[1]));
			c.mt = MATCH_FILE;
			break;

		case 'N':
		case 'R':
			rstrip(s, &len);
			b = lstrip(&s[1]);

			{
				long n;
				char *ep = NULL;
				n = strtol(b, &ep, 10);
				if (*b == '\0' || *ep != '\0') {
					fprintf(stderr, "line %zu: invalid '%c' argument '%s'\n", line, s[0], b);
					exit(EXIT_FAILURE);
				}

				if (s[0] == 'N') {
					c.count = n;
				} else {
					c.expected_matches = n;
				}
			}
			break;

		case 'X':
			t.comp_delta = t.glush_delta = t.det_delta = t.min_delta = t.run_delta = 0.0;
			err = perf_case_run(&c, halt, &t);
			if (tsv) {
				perf_case_report_tsv(&c, halt, err, quiet, &t);
			} else {
				perf_case_report_txt(&c, halt, err, quiet, &t);
				perf_case_report_error(err);
			}
			perf_case_reset(&c);
			break;

		default:
			fprintf(stderr, "line %zu: unknown command '%c': %s\n", line, s[0], s);
			exit(EXIT_FAILURE);
		}
	}

	free(buf);

	return 0;
}

static int
read_file(const char *path, struct str *contents)
{
	char buf[4096];
	FILE *f;
	char *s;
	size_t len, cap;

	f = fopen(path, "r");
	if (f == NULL) {
		return -1;
	}

	s = contents->data;
	cap = contents->cap;
	len = 0;

	for (;;) {
		size_t n;
		n = fread(buf, 1, sizeof buf, f);
		if (n == 0) {
			break;
		}

		if (len + n >= cap) {
			size_t newcap = 2*cap;
			if (newcap < len+n+1) {
				newcap = len+n+1;
			}

			s = xrealloc(s, newcap);
			cap = newcap;
		}

		memcpy(&s[len], buf, n);
		len += n;
	}

	contents->data = s;
	contents->len = len;
	contents->cap = cap;

	if (ferror(f)) {
		return -1;
	}

	return 0;
}

static void
xclock_gettime(struct timespec *tp)
{
	assert(tp != NULL);

	if (clock_gettime(CLOCK_MONOTONIC, tp) != 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
}

static int
phase(double *delta, int count, struct fsm *fsm, int (*f)(struct fsm *))
{
	struct timespec t0, t1;

	assert(delta != NULL);
	assert(fsm != NULL);
	assert(f != NULL);

	xclock_gettime(&t0);

	if (!f(fsm)) {
		fsm_free(fsm);
		return 0;
	}

	xclock_gettime(&t1);

	if (count == 1) {
		report_delta(delta, &t0, &t1);
	}

	return 1;
}

static enum error_type
perf_case_run(struct perf_case *c, enum halt halt,
	struct timing *t)
{
	struct fsm *fsm;
	struct fsm_runner runner;
	struct str contents;
	enum error_type ret;
	int iter;

	contents = str_empty();

	/* XXX - fix this */
	opt.comments = 0;

	{
		static const struct re_err err_zero;
		struct timespec c0, c1;

		struct re_err comp_err;
		enum re_flags flags;

		comp_err = err_zero;
		flags = 0;

		xclock_gettime(&c0);

		for (iter=0; iter < c->count; iter++) {
			const char *re;

			re = c->regexp.data;

			fsm = re_comp(c->dialect, fsm_sgetc, &re, &opt, flags, &comp_err);
			if (fsm == NULL) {
				return ERROR_PARSING_REGEXP;
			}
		}

		xclock_gettime(&c1);

		report_delta(&t->comp_delta, &c0, &c1);
	}

	if (halt == HALT_AFTER_COMPILE) {
		fsm_free(fsm);
		goto done;
	}

	if (!phase(&t->glush_delta, c->count, fsm, fsm_glushkovise)) return ERROR_GLUSHKOVISING;
	if (halt == HALT_AFTER_GLUSHKOVISE) {
		fsm_free(fsm);
		goto done;
	}

	if (!phase(&t->det_delta,   c->count, fsm, fsm_determinise)) return ERROR_DETERMINISING;
	if (halt == HALT_AFTER_DETERMINISE) {
		fsm_free(fsm);
		goto done;
	}

	if (!phase(&t->min_delta,   c->count, fsm, fsm_minimise))    return ERROR_MINIMISING;
	if (halt == HALT_AFTER_MINIMISE) {
		fsm_free(fsm);
		goto done;
	}

	ret = fsm_runner_initialize(fsm, &runner, c->impl, vm_opts);
	if (ret != ERROR_NONE) {
		fsm_free(fsm);
		return ret;
	}

#if DEBUG_VM_FSM
	fprintf(stderr, "FSM:\n");
	fsm_print_fsm(stderr, fsm);
	fprintf(stderr, "---\n");
	{
		FILE *f = fopen("dump.fsm", "w");
		fsm_print_fsm(f, fsm);
		fclose(f);
	}
#endif /* DEBUG_VM_FSM */

	fsm_free(fsm);

	switch (c->mt) {
	case MATCH_NONE:
		break;

	case MATCH_STRING:
		contents = c->match;
		break;

	case MATCH_FILE:
		contents = str_empty();
		if (read_file(c->match.data, &contents) != 0) {
			fsm_runner_finalize(&runner);
			str_free(&contents);
			return ERROR_FILE_IO;
		}
	}

	if (c->mt != MATCH_NONE) {
		struct timespec t0, t1;

		xclock_gettime(&t0);

		ret = ERROR_NONE;

		for (iter=0; iter < c->count; iter++) {
			int r;

			assert(contents.data != NULL);

			r = fsm_runner_run(&runner, contents.data, contents.len);

			/* XXX - at some point, match more than once! */
			if (!!r != !!c->expected_matches) {
				ret = (c->expected_matches) ? ERROR_SHOULD_MATCH : ERROR_SHOULD_NOT_MATCH;
				break;
			}
		}

		if (c->mt == MATCH_FILE) {
			str_free(&contents);
		}

		if (ret != ERROR_NONE) {
			fsm_runner_finalize(&runner);
			return ret;
		}

		xclock_gettime(&t1);

		report_delta(&t->run_delta, &t0, &t1);
	}

done:

	fsm_runner_finalize(&runner);

	return ERROR_NONE;
}

static void
perf_case_report_txt(struct perf_case *c, enum halt halt,
	enum error_type err, int quiet,
	const struct timing *t)
{
	printf("---[ %s ]---\n", c->test_name.data);
	printf("line %zu\n", c->line);
	if (!quiet) {
		printf("regexp /%s/\n", c->regexp.data);
	}

	switch (c->mt) {
	case MATCH_NONE:
		break;

	case MATCH_STRING:
		printf("matching STRING:\n%s\n", c->match.data);
		break;

	case MATCH_FILE:
		printf("matching FILE %s\n", c->match.data);
		break;

	default:
		assert(!"unreached");
	}

	if (err != ERROR_NONE) {
		return;
	}

	printf("compile %d iterations took %.4f seconds, %.4g seconds/iteration\n",
		c->count, t->comp_delta, t->comp_delta / c->count);
	if (halt == HALT_AFTER_COMPILE) {
		return;
	}
	if (c->count == 1) {
		printf("glushkovise %d iterations took %.4f seconds, %.4g seconds/iteration\n",
			c->count, t->glush_delta, t->glush_delta / c->count);
		if (halt == HALT_AFTER_GLUSHKOVISE) {
			return;
		}
		printf("determinise %d iterations took %.4f seconds, %.4g seconds/iteration\n",
			c->count, t->det_delta, t->det_delta / c->count);
		if (halt == HALT_AFTER_DETERMINISE) {
			return;
		}
		printf("minimise %d iterations took %.4f seconds, %.4g seconds/iteration\n",
			c->count, t->min_delta, t->min_delta / c->count);
		if (halt <= HALT_AFTER_MINIMISE) {
			return;
		}
	}
	if (c->mt != MATCH_NONE) {
		printf("execute %d iterations took %.4f seconds, %.4g seconds/iteration\n",
			c->count, t->run_delta, t->run_delta / c->count);
		if (halt == HALT_AFTER_EXECUTION) {
			return;
		}
	}
}

static void
perf_case_report_head(int quiet, enum halt halt)
{
	printf("line");
	if (!quiet) {
		printf("\tname");
		printf("\tregex");
	}
	printf("\tcompile");
	if (halt == HALT_AFTER_COMPILE) {
		goto done;
	}
	printf("\tglushkovise");
	if (halt == HALT_AFTER_GLUSHKOVISE) {
		goto done;
	}
	printf("\tdeterminise");
	if (halt == HALT_AFTER_DETERMINISE) {
		goto done;
	}
	printf("\tminimise");
	if (halt == HALT_AFTER_MINIMISE) {
		goto done;
	}
	printf("\texecute");
	if (halt == HALT_AFTER_EXECUTION) {
		goto done;
	}

done:

	printf("\terror\n");
}

static void
perf_case_report_tsv(struct perf_case *c, enum halt halt,
	enum error_type err, int quiet,
	const struct timing *t)
{
	printf("%zu", c->line);
	if (!quiet) {
		printf("\t\"%s\"", c->test_name.data);
		printf("\t\"%s\"", c->regexp.data);
	}

	printf("\t%.4g", t->comp_delta / c->count);
	if (halt == HALT_AFTER_COMPILE) {
		goto done;
	}
	if (c->count != 1) {
		printf("\t0");
		if (halt == HALT_AFTER_GLUSHKOVISE) {
			goto done;
		}
		printf("\t0");
		if (halt == HALT_AFTER_DETERMINISE) {
			goto done;
		}
		printf("\t0");
		if (halt == HALT_AFTER_MINIMISE) {
			goto done;
		}
	} else {
		printf("\t%.4g", t->glush_delta / c->count);
		if (halt == HALT_AFTER_GLUSHKOVISE) {
			goto done;
		}
		printf("\t%.4g", t->det_delta / c->count);
		if (halt == HALT_AFTER_DETERMINISE) {
			goto done;
		}
		printf("\t%.4g", t->min_delta / c->count);
		if (halt == HALT_AFTER_MINIMISE) {
			goto done;
		}
	}
	if (c->mt != MATCH_NONE) {
		printf("\t%.4g", t->run_delta / c->count);
		if (halt == HALT_AFTER_EXECUTION) {
			goto done;
		}
	} else {
		printf("\t0");
	}

done:

	printf("\t%d\n", err);
}

static void
perf_case_report_error(enum error_type err)
{
	switch (err) {
	case ERROR_NONE:
		break;

	/*
	case ERROR_BAD_RECORD_TYPE:    fprintf(f, "bad record\n"); break;
	case ERROR_ESCAPE_SEQUENCE:    fprintf(f, "bad escape sequence\n"); break;
	*/

	case ERROR_PARSING_REGEXP:
		printf("ERROR: parsing regexp\n");
		break;

	case ERROR_GLUSHKOVISING:
		printf("ERROR: glushkovising regexp\n");
		break;

	case ERROR_DETERMINISING:
		printf("ERROR: determinising regexp\n");
		break;

	case ERROR_COMPILING_BYTECODE:
		printf("ERROR: compiling regexp\n");
		break;

	case ERROR_SHOULD_MATCH:
		printf("ERROR: regexp should match but doesn't\n");
		break;

	case ERROR_SHOULD_NOT_MATCH:
		printf("ERROR: regexp should not match but does\n");
		break;

	case ERROR_FILE_IO:
		printf("ERROR: reading file: %s\n", strerror(errno));
		break;

	default:
		printf("ERROR: unknown error %d\n", (int)err);
		break;
	}

	printf("\n");
}

static void
usage(void)
{
	fprintf(stderr, "usage: reperf [-O <olevel>] [-L <what>] [-x <encoding>] [-p] <driverfile | ->\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        <driverfile> specifies the path to the driver file, or '-' to read it from stdin\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -q\n");
	fprintf(stderr, "             quiet: elide the regexp from output (useful for long regexps)\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -p\n");
	fprintf(stderr, "             pause before running performance tests\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -t\n");
	fprintf(stderr, "             output in TSV format. The default is human-readable text\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -H <halt>\n");
	fprintf(stderr, "             halt after compile, glushkovise, determinise, minimise or execute\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -O <olevel>\n");
	fprintf(stderr, "             sets VM optimization level:\n");
	fprintf(stderr, "                 0 = disable optimizations\n");
	fprintf(stderr, "                 1 = basic optimizations\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -L <what>\n");
	fprintf(stderr, "             logs intermediate representations:\n");
	fprintf(stderr, "                 ir        logs IR (after optimization)\n");
	fprintf(stderr, "                 enc       logs VM encoding instructions\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -l <implementation>\n");
	fprintf(stderr, "             sets implementation type:\n");
	fprintf(stderr, "                 vm        interpret vm instructions (default)\n");
	fprintf(stderr, "                 asm       generate assembly and assemble\n");
	fprintf(stderr, "                 c         compile as per fsm_print_c()\n");
	fprintf(stderr, "                 vmc       compile as per fsm_print_vmc()\n");
	fprintf(stderr, "                 rust      compile as per fsm_print_rust()\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -x <encoding>\n");
	fprintf(stderr, "             sets encoding type:\n");
	fprintf(stderr, "                 v1        version 0.1 variable length encoding\n");
	fprintf(stderr, "                 v2        version 0.2 fixed length encoding\n");
}

static FILE *
xopen(const char *s)
{
	FILE *f;

	assert(s != NULL);

	if (0 == strcmp(s, "-")) {
		return stdin;
	}

	f = fopen(s, "r");
	if (f == NULL) {
		perror(s);
		exit(EXIT_FAILURE);
	}

	return f;
}

enum parse_escape_result {
	PARSE_OK = 0,
	PARSE_INVALID_ESCAPE,
	PARSE_INCOMPLETE_ESCAPE
};

int
main(int argc, char *argv[])
{
	int i;
	int pause, quiet, tsv;
	enum implementation impl;
	enum halt halt;

	int optlevel = 1;

	(void)str_init;

	/* note these defaults are the opposite than for fsm(1) */
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	opt.comments          = 1;

	/*
	 * The IO API would depend on c->mt == MATCH_STRING ? FSM_IO_PAIR : FSM_IO_GETC;
	 * except we read in the entire file below anyway, so we just
	 * use FSM_IO_PAIR for both. The type of fsm_main depends on the IO API.
	 *
	 * The IO API also affects the trampoline code for FFI.
	 */
	opt.io = FSM_IO_PAIR;

	pause                 = 0;
	quiet                 = 0;
	tsv                   = 0;
	impl                  = IMPL_INTERPRET;
	halt                  = HALT_AFTER_EXECUTION;

	{
		int c;

		while (c = getopt(argc, argv, "h" "O:L:l:x:" "pqtH:" ), c != -1) {
			switch (c) {
			case 'O':
				optlevel = strtoul(optarg, NULL, 10);
				break;

			case 'L':
				if (strcmp(optarg, "ir") == 0) {
					vm_opts.flags |= FSM_VM_COMPILE_PRINT_IR;
				} else if (strcmp(optarg, "enc") == 0) {
					vm_opts.flags |= FSM_VM_COMPILE_PRINT_ENC;
				} else {
					fprintf(stderr, "unknown argument to -L: %s\n", optarg);
					usage();
					exit(1);
				}
				break;

			case 'l':
				if (strcmp(optarg, "vm") == 0) {
					impl = IMPL_INTERPRET;
				} else if (strcmp(optarg, "c") == 0) {
					impl = IMPL_C;
				} else if (strcmp(optarg, "rust") == 0) {
					impl = IMPL_RUST;
				} else if (strcmp(optarg, "asm") == 0) {
					impl = IMPL_VMASM;
				} else if (strcmp(optarg, "vmc") == 0) {
					impl = IMPL_VMC;
				} else {
					fprintf(stderr, "unknown argument to -l: %s\n", optarg);
					usage();
					exit(1);
				}
				break;

			case 'H':
				if (optarg[0] == 'c') {
					halt = HALT_AFTER_COMPILE;
				} else if (optarg[0] == 'g') {
					halt = HALT_AFTER_GLUSHKOVISE;
				} else if (optarg[0] == 'd') {
					halt = HALT_AFTER_DETERMINISE;
				} else if (optarg[0] == 'm') {
					halt = HALT_AFTER_MINIMISE;
				} else {
					fprintf(stderr, "unknown argument to -H: %s\n", optarg);
					usage();
					exit(1);
				}
				break;

			case 'x':
				if (strcmp(optarg, "v1") == 0) {
					vm_opts.output = FSM_VM_COMPILE_VM_V1;
				} else if (strcmp(optarg, "v2") == 0) {
					vm_opts.output = FSM_VM_COMPILE_VM_V2;
				} else {
					fprintf(stderr, "unknown argument to -x: %s\n", optarg);
					usage();
					exit(1);
				}
				break;

			case 'p': pause = 1; break;
			case 'q': quiet = 1; break;
			case 't': tsv   = 1; break;

			case 'h':
				usage();
				return EXIT_SUCCESS;

			case '?':
			default:
				usage();
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc < 1) {
		usage();
		return EXIT_FAILURE;
	}

	if (optlevel > 0) {
		vm_opts.flags |= FSM_VM_COMPILE_OPTIM;
	}

	if (pause) {
		char buf[128];
		printf("paused\n");
		fflush(stdout);
		fgets(buf, sizeof buf, stdin);
	}

	if (tsv) {
		perf_case_report_head(quiet, halt);
	}

	for (i=0; i < argc; i++) {
		FILE *f = xopen(argv[i]);
		parse_perf_case(f, impl, halt, quiet, tsv);
		fclose(f);
	}

	return EXIT_SUCCESS;
}
