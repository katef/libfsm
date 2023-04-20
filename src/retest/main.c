/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <time.h>
#include <signal.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/alloc.h>
#include <fsm/vm.h>

#include <re/re.h>

#include "libfsm/internal.h" /* XXX */
#include "libre/print.h" /* XXX */
#include "libre/class.h" /* XXX */
#include "libre/ast.h" /* XXX */

#include "runner.h"

#define DEBUG_ESCAPES     0
#define DEBUG_VM_FSM      0
#define DEBUG_TEST_REGEXP 0

/*
 * TODO: accepting a delimiter would be useful: /abc/. perhaps provide that as
 * a convenience function, especially wrt. escaping for lexing. Also convenient
 * for specifying flags: /abc/g
 * TODO: flags; -r for RE_REVERSE, etc
 */

struct match {
	int i;
	const char *s;
	struct match *next;
};

static int tty_output = 0;
static int do_timing  = 0;

static int do_watchdog   = 0;
static int watchdog_secs = 5;

/* these are set/queried both inside and outside of signal handlers, so they have to volatile */
static volatile int watchdog_on = 0;
static volatile int watchdog_tripped = 0;

/* watchdog timer signal handler */
static void watchdog_handler(int arg) {
	(void)arg;

	if (watchdog_on) {
		watchdog_tripped = 1;
	}
}

static struct sigaction watchdog_action_enabled = {
	.sa_handler = &watchdog_handler,
#if defined(SA_RESETHAND)
	.sa_flags = SA_RESETHAND,
#endif
};
static struct sigaction watchdog_action_disabled = {
	.sa_handler = SIG_DFL
};

/* custom allocators used if the watchdog timer is enabled.  If the
 * watchdog is tripped, these will cause fsm_determinise() to abort
 * on the next memory allocation.
 */
static void
watchdog_free(void *opaque, void *p)
{
	(void)opaque;

	free(p);
}

static void *
watchdog_calloc(void *opaque, size_t n, size_t sz)
{
	(void)opaque;

	if (watchdog_tripped) {
		return NULL;
	}

	return calloc(n,sz);
}

static void *
watchdog_malloc(void *opaque, size_t sz)
{
	(void)opaque;

	if (watchdog_tripped) {
		return NULL;
	}

	return malloc(sz);
}

static void *
watchdog_realloc(void *opaque, void *p, size_t sz)
{
	(void)opaque;

	if (watchdog_tripped) {
		return NULL;
	}

	return realloc(p,sz);
}

static struct fsm_alloc watchdog_alloc = {
	.free = &watchdog_free,
	.calloc = &watchdog_calloc,
	.malloc = &watchdog_malloc,
	.realloc = &watchdog_realloc,
	.opaque = NULL,
};

static struct fsm_options opt;
static struct fsm_vm_compile_opts vm_opts = { 0, FSM_VM_COMPILE_VM_V1, NULL };

enum retest_options {
	RETEST_OPT_NONE = 0,
	RETEST_OPT_ESCAPE_REGEXP = 0x0001,
};

static void
usage(void)
{
	fprintf(stderr, "usage: retest [-O <olevel>] [-L <what>] [-x <encoding>] [-r <dialect>] [-e <prefix>] [-E <maxerrs>] testfile [testfile...]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "        -O <olevel>\n");
	fprintf(stderr, "             sets VM optimization level:\n");
	fprintf(stderr, "                 0 = disable optimizations\n");
	fprintf(stderr, "                 1 = basic optimizations\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -L <what>\n");
	fprintf(stderr, "             logs intermediate representations:\n");
	fprintf(stderr, "                 ir        logs IR (after optimizations)\n");
	fprintf(stderr, "                 enc       logs VM encoding instructions\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -l <implementation>\n");
	fprintf(stderr, "             sets implementation type:\n");
	fprintf(stderr, "                 vm        interpret vm instructions (default)\n");
	fprintf(stderr, "                 asm       generate assembly and assemble\n");
	fprintf(stderr, "                 c         compile as per fsm_print_c()\n");
	fprintf(stderr, "                 vmc       compile as per fsm_print_vmc()\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "        -x <encoding>\n");
	fprintf(stderr, "             sets encoding type:\n");
	fprintf(stderr, "                 v1        version 0.1 variable length encoding\n");
	fprintf(stderr, "                 v2        version 0.2 fixed length encoding\n");
}

static const struct {
	const char *name;
	enum re_dialect dialect;
} dialect_lookup_table[] = {
/* TODO:
	{ "ere",     RE_ERE     },
	{ "bre",     RE_BRE     },
	{ "plan9",   RE_PLAN9   },
	{ "js",      RE_JS      },
	{ "python",  RE_PYTHON  },
	{ "sql99",   RE_SQL99   },
*/
	{ "like",    RE_LIKE    },
	{ "literal", RE_LITERAL },
	{ "glob",    RE_GLOB    },
	{ "native",  RE_NATIVE  },
	{ "pcre",    RE_PCRE    },
	{ "sql",     RE_SQL     }
};

static const char *
dialect_to_name(enum re_dialect dialect)
{
	const size_t num_dialects = sizeof dialect_lookup_table / sizeof dialect_lookup_table[0];
	size_t i;

	for (i = 0; i < num_dialects; i++) {
		if (dialect == dialect_lookup_table[i].dialect) {
			return dialect_lookup_table[i].name;
		}
	}

	return "unknown";
}

static enum re_dialect
parse_dialect_name(const char *name)
{
	const size_t num_dialects = sizeof dialect_lookup_table / sizeof dialect_lookup_table[0];
	size_t i;

	assert(name != NULL);

	for (i = 0; i < num_dialects; i++) {
		if (0 == strcmp(dialect_lookup_table[i].name, name)) {
			return dialect_lookup_table[i].dialect;
		}
	}

	fprintf(stderr, "unrecognised regexp dialect \"%s\"; valid dialects are: ", name);

	for (i = 0; i < num_dialects; i++) {
		fprintf(stderr, "%s%s",
			dialect_lookup_table[i].name,
			i + 1 < num_dialects ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static void *
xmalloc(size_t n)
{
	void *p = malloc(n);
	if (p == NULL) {
		perror("xmalloc");
		exit(EXIT_FAILURE);
	}

	return p;
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

static void
xclock_gettime(struct timespec *tp)
{
	assert(tp != NULL);

	if (clock_gettime(CLOCK_MONOTONIC, tp) != 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}
}

enum parse_escape_result {
	PARSE_OK = 0,
	PARSE_INVALID_ESCAPE,
	PARSE_INCOMPLETE_ESCAPE
};

static int
parse_escapes(char *s, char **errpos, int *lenp)
{
	int i,j,st;
	unsigned char ccode;
	int hexcurly, ndig;

	enum { ST_BARE, ST_ESC, ST_OCT, ST_HEX, ST_HEX_DIGIT };

	ccode = 0;
	hexcurly = 0;

	st = ST_BARE;
	for (i=0, j=0; s[i] != '\0'; i++) {
#if DEBUG_ESCAPES
		fprintf(stderr, "%3d | st=%d hexcurly=%d ndig=%d ccode=0x%02x | %2d %c\n",
			i, st, hexcurly, ndig, (unsigned int)ccode, s[i], 
			isprint(s[i]) ? s[i] : ' ');
#endif /* DEBUG_ESCAPES */

		if (s[i] == '\0') {
			break;
		}

		switch (st) {
		case ST_BARE:
bare:
			if (s[i] == '\\') {
				st = ST_ESC;
			} else {
				s[j++] = s[i];
			}
			break;

		case ST_ESC:
			switch (s[i]) {
			case 'a':  s[j++] = '\a'; st = ST_BARE; break;
			case 'b':  s[j++] = '\b'; st = ST_BARE; break;
			case 'e':  s[j++] = '\033'; st = ST_BARE; break;
			case 'f':  s[j++] = '\f'; st = ST_BARE; break;
			case 'n':  s[j++] = '\n'; st = ST_BARE; break;
			case 'r':  s[j++] = '\r'; st = ST_BARE; break;
			case 't':  s[j++] = '\t'; st = ST_BARE; break;
			case 'v':  s[j++] = '\v'; st = ST_BARE; break;
			case '"':  s[j++] = '\"'; st = ST_BARE; break;
			case '\\': s[j++] = '\\'; st = ST_BARE; break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				   ccode = 0; ndig = 0; st = ST_OCT; goto octdig;
			case 'x': ccode = 0; ndig = 0; hexcurly = 0; st = ST_HEX; break;

			default:
				if (errpos) {
					*errpos = &s[i];
					return PARSE_INVALID_ESCAPE;
				}
				break;
			}
			break;

		case ST_OCT:
octdig:
			if (ndig < 3 && s[i] >= '0' && s[i] <= '7') {
				ccode = (ccode * 8) + (s[i] - '0');
				ndig++;
			} else {
				s[j++] = ccode;
				st = ST_BARE;
				goto bare;
			}

			break;

		case ST_HEX:
			st = ST_HEX_DIGIT;
			if (s[i] != '{') {
				hexcurly = 0;
				goto hexdigit;
			}

			hexcurly = 1;
			break;

		case ST_HEX_DIGIT:
hexdigit:
			{
				unsigned char uc = toupper((unsigned char)s[i]);
				if (ndig < 2 && isxdigit(uc)) {
					if (uc >= '0' && uc <= '9') {
						ccode = (ccode * 16) + (uc - '0');
					} else {
						ccode = (ccode * 16) + (uc - 'A' + 10);
					}

					ndig++;
				} else {
					s[j++] = ccode;
					st = ST_BARE;

					if (!hexcurly) {
						goto bare;
					} else if (uc != '}') {
						if (errpos) {
							*errpos = &s[i];
						}
						return PARSE_INCOMPLETE_ESCAPE;
					}
				}
			}
			break;
		}
	}

	switch (st) {
	case ST_BARE:
		break;

	case ST_ESC:
		if (errpos) {
			*errpos = &s[i];
		}
		return PARSE_INVALID_ESCAPE;

	case ST_HEX:
	case ST_OCT:
	case ST_HEX_DIGIT:
		if (hexcurly && st == ST_HEX_DIGIT) {
			if (errpos) {
				*errpos = &s[i];
			}
			return PARSE_INCOMPLETE_ESCAPE;
		}

		s[j++] = (char)ccode;
		break;
	}

	s[j] = '\0';

	*lenp = j;

	return PARSE_OK;
}

struct single_error_record {
	char *filename;
	char *regexp;
	char *flags;
	char *failed_match;
	unsigned int line;
	enum error_type type;
	enum re_dialect dialect;
};

struct error_record {
	size_t len;
	size_t cap;

	struct single_error_record *errors;
};

#define ERROR_RECORD_INITIAL_SIZE 64

static int
error_record_init(struct error_record *erec)
{
	erec->errors = calloc(ERROR_RECORD_INITIAL_SIZE, sizeof erec->errors[0]);
	if (erec->errors == NULL) {
		return -1;
	}

	erec->len = 0;
	erec->cap = ERROR_RECORD_INITIAL_SIZE;

	return 0;
}

static char
nibble_char(unsigned hex) {
	static const char *dig = "0123456789abcdef";

	return dig[hex & 0xf];
}

static char *
dup_str_esc(const char *s, int *err)
{
	size_t len, numesc;
	const char *sp;
	char *cpy, *cp;

	if (err != NULL && *err != 0) {
		return NULL;
	}

	len = 0;
	numesc = 0;
	for (sp = s; *sp; sp++) {
		if (*sp != ' ' && isspace((unsigned char)(*sp))) {
			len += 2;
			numesc++;
		} else if (!isprint((unsigned char)*sp)) {
			len += 4;
			numesc++;
		} else {
			len++;
		}
	}

	cpy = xmalloc(len+1);
	if (numesc == 0) {
		memcpy(cpy, s, len+1);
		return cpy;
	}

	for (sp = s, cp = cpy; *sp; sp++) {
		switch (*sp) {
		case '\n': assert(cp+2 <= cpy+len); cp[0] = '\\'; cp[1] = 'n'; cp += 2; break;
		case '\r': assert(cp+2 <= cpy+len); cp[0] = '\\'; cp[1] = 'r'; cp += 2; break;
		case '\f': assert(cp+2 <= cpy+len); cp[0] = '\\'; cp[1] = 'f'; cp += 2; break;
		case '\t': assert(cp+2 <= cpy+len); cp[0] = '\\'; cp[1] = 't'; cp += 2; break;
		case '\v': assert(cp+2 <= cpy+len); cp[0] = '\\'; cp[1] = 'v'; cp += 2; break;

		default: 
			{
				unsigned char uc = (unsigned char)*sp;
				if (isprint(uc)) {
					assert(cp+1 <= cpy+len);
					cp[0] = *sp; cp ++; break;
				} else {
					assert(cp+4 <= cpy+len);
					cp[0] = '\\';
					cp[1] = 'x';
					cp[2] = nibble_char(uc>>4);
					cp[3] = nibble_char(uc & 0xf);
					cp += 4;
				}
			}
			break;
		}
	}

	*cp = '\0';

	return cpy;
}

static char *
dup_str(const char *s, int *err)
{
	size_t len;
	char *cpy;

	if (err != NULL && *err != 0) {
		return NULL;
	}

	len = strlen(s);
	cpy = malloc(len+1);
	if (cpy == NULL) {
		if (err != NULL) {
			*err = -1;
		}
		return NULL;
	}

	memcpy(cpy, s, len+1);
	return cpy;
}

static int
error_record_add(struct error_record *erec, enum error_type type, const char *fn, const char *re, const char *flags, const char *failed_match, unsigned int line, enum re_dialect dialect)
{
	size_t ind;
	int err;

	assert(erec != NULL);

	/* check if we need to expand the list */
	ind = erec->len;
	if (ind >= erec->cap) {
		size_t newcap, newsz;
		struct single_error_record *newrecs;

		newcap = erec->cap * 2; /* XXX - smarter about this? */
		newsz = newcap * sizeof erec->errors[0];

		newrecs = realloc(erec->errors, newsz);
		if (newrecs == NULL) {
			return -1;
		}

		erec->errors = newrecs;
		erec->cap = newcap;
	}

	/* add record */
	err = 0;
	erec->errors[ind].filename     = dup_str(fn,&err);
	erec->errors[ind].regexp       = re != NULL           ? dup_str_esc(re,&err)       : NULL;
	erec->errors[ind].flags        = flags != NULL        ? dup_str_esc(flags,&err)    : NULL;
	erec->errors[ind].failed_match = failed_match != NULL ? dup_str(failed_match,&err) : NULL;
	erec->errors[ind].line         = line;
	erec->errors[ind].type         = type;
	erec->errors[ind].dialect      = dialect;

	erec->len++;

	return err;
}

static void
single_error_record_print(FILE *f, const struct single_error_record *err)
{
	const char *dialect_name;

	dialect_name = dialect_to_name(err->dialect);

	fprintf(f, "(%s:%u) ", err->filename, err->line);
	switch (err->type) {
	case ERROR_NONE:               fprintf(f, "no error?\n");  break;
	case ERROR_BAD_RECORD_TYPE:    fprintf(f, "bad record\n"); break;
	case ERROR_ESCAPE_SEQUENCE:    fprintf(f, "bad escape sequence\n"); break;
	case ERROR_PARSING_REGEXP:     fprintf(f, "error parsing %s regexp /%s/%s\n", dialect_name, err->regexp, err->flags); break;
	case ERROR_DETERMINISING:
		fprintf(f, "error determinising %s regexp /%s/%s\n", dialect_name, err->regexp, err->flags);
		break;
	case ERROR_COMPILING_BYTECODE:
		fprintf(f, "error compiling %s regexp /%s/%s\n", dialect_name, err->regexp, err->flags);
		break;
	case ERROR_SHOULD_MATCH:
		fprintf(f, "%s regexp /%s/%s should match \"%s\" but doesn't\n",
			dialect_name, err->regexp, err->flags, err->failed_match);
		break;
	case ERROR_SHOULD_NOT_MATCH:
		fprintf(f, "%s regexp /%s/%s should not match \"%s\" but does\n",
			dialect_name, err->regexp, err->flags, err->failed_match);
		break;

	case ERROR_WATCHDOG:
		fprintf(f, "watchdog (%d secs) tripped while determinising %s regexp /%s/%s\n", watchdog_secs, dialect_name, err->regexp, err->flags);
		break;

	default:
		fprintf(f, "unknown error\n");
		break;
	}
}

static void
error_record_print(FILE *f, const struct error_record *erec)
{
	size_t i,n;
	n = erec->len;
	for (i=0; i < n; i++) {
		fprintf(f, "[%4u/%4u] ", (unsigned) i + 1, (unsigned) n);
		single_error_record_print(f, &erec->errors[i]);
	}
}

static void
error_record_finalize(struct error_record *erec)
{
	size_t i,n;

	if (erec == NULL) {
		return;
	}

	n = erec->len;
	for (i=0; i < n; i++) {
		free(erec->errors[i].filename);
		free(erec->errors[i].regexp);
		free(erec->errors[i].flags);
		free(erec->errors[i].failed_match);
	}

	free(erec->errors);
	erec->errors = NULL;
	erec->len = erec->cap = 0;
}

static char *
flagstring(enum re_flags flags, char buf[16])
{
	char *m;
	memset(&buf[0],0,16);

	m = &buf[0];
	if (flags & RE_ICASE)    { *m++ = 'i'; }
	if (flags & RE_TEXT)     { *m++ = 't'; }
	if (flags & RE_MULTI)    { *m++ = 'm'; }
	if (flags & RE_REVERSE)  { *m++ = 'r'; }
	if (flags & RE_SINGLE)   { *m++ = 's'; }
	if (flags & RE_ZONE)     { *m++ = 'z'; }
	if (flags & RE_ANCHORED) { *m++ = 'a'; }

	return &buf[0];
}

/* test file format:
 *
 * 1. lines starting with '#' are skipped
 * 2. lines starting with 'R' set the dialect.  A line with only 'R'
 *    resets the dialect to the default dialect (either PCRE or given
 *    by the -r option).
 * 2b. lines starting with "~" are treated as regular expressions with
 *     the initial "~" removed.  This is to allow regular expressions
 *     that start with an retest control line sequence like /M /
 * 2c. lines starting with "O +" turn on retest options, and lines
 *     starting with "O -" turn off retest options.  "O =" sets flags
 *     to the options on the rest of the line; if the rest of the line
 *     is blank, all options are disabled.
 *         retest options:
 *         e=RETEST_OPT_ESCAPE_REGEXP (retest parses escapes in the
 *         regexp line)
 * 2d. lines that start with "O &" will save the current runner opts
 *     and restore them after the next test case.
 * 3. lines starting with "M " set the flags:
 * 	i=RE_ICASE, t=RE_TEXT, m = RE_MULTI, r=RE_REVERSE,
 * 	s=RE_SINGLE, z=RE_ZONE, a=RE_ANCHORED
 * 4. records are separated by empty lines.  flags reset at each record
 *    to no flags.
 * 5. the first line of each record is the regular expression to be
 *    tested.  existing flags are used.
 * 6. after the regular expression, valid lines start with '#', '+', or '-'
 *     a. '+' indicates that the rest of the line should be matched with the
 *        current regular expression
 *     b. '-' indicates the the rest of the line should *not* be matched with
 *        the current regular expression
 *     c. '#' lines are comments
 */
static int
process_test_file(const char *fname, enum re_dialect default_dialect, enum implementation impl, int max_errors, struct error_record *erec)
{
	static const struct fsm_runner init_runner;

	char buf[4096];
	char cpy[sizeof buf];
	char *s;
	FILE *f;
	int num_re_errors, num_errors;
	int num_regexps, num_test_cases;
	int linenum;

	char *regexp;
	char flagdesc[16];

	enum error_type ret;
	bool impl_ready = false;
	struct fsm_runner runner = init_runner;
	enum re_flags flags;

	enum re_dialect dialect;
	enum retest_options runner_opts;
	enum retest_options saved_opts;
	int restore_runner_opts;
	const char *dialect_name;

	static const struct timespec t_zero;
	struct timespec t_start;

	regexp = NULL;

	/* XXX - fix this */
	opt.comments = 0;

	f = xopen(fname);

	num_regexps    = 0;
	num_re_errors  = 0;
	num_test_cases = 0;
	num_errors     = 0;

	linenum        = 0;
	flags          = RE_FLAGS_NONE;
	runner_opts    = RETEST_OPT_NONE;
	saved_opts     = runner_opts;
	dialect        = default_dialect;
	dialect_name   = dialect_to_name(dialect);

	t_start        = t_zero;

	restore_runner_opts = 0;

	memset(&flagdesc[0],0,sizeof flagdesc);

	while (s = fgets(buf, sizeof buf, f), s != NULL) {
		int len;

		linenum++;
		len = strlen(s);
		if (len > 0 && s[len-1] == '\n') {
			s[--len] = '\0';
		}

		if (len == 0) {
			if (regexp != NULL && do_timing && (t_start.tv_sec != 0 || t_start.tv_nsec != 0)) {
				struct timespec t_end = t_zero;
				xclock_gettime(&t_end);

				long diff_ns = t_end.tv_nsec - t_start.tv_nsec;
				double diff = difftime(t_end.tv_sec, t_start.tv_sec) + 1e-9 * (diff_ns);

				if (diff >= 0.001) {
					printf("[TIME  ] %s regexp /%s/%s tests took %.4f seconds\n",
						dialect_name, regexp, flagdesc, diff);
				} else {
					printf("[TIME  ] %s regexp /%s/%s tests took %.4e seconds\n",
						dialect_name, regexp, flagdesc, diff);
				}

				fflush(stdout);
			}

			free(regexp);
			regexp = NULL;
			flags = RE_FLAGS_NONE;

			if (impl_ready) {
				fsm_runner_finalize(&runner);
				runner     = init_runner;
				impl_ready = false;
			}

			if (restore_runner_opts) {
				runner_opts = saved_opts;
			}

			continue;
		}

		if (s[0] == '#') {
			continue;
		}

		if (s[0] == 'R' && (s[1] == '\0' || s[1] == ' ')) {
			if (s[1] == '\0') {
				dialect = default_dialect;
			}
			else {
				dialect = parse_dialect_name(&s[2]);
			}

			dialect_name = dialect_to_name(dialect);

			continue;
		}

		if (s[0] == 'O' && s[1] == ' ') {
			enum retest_options opt_arg = 0;

			if (s[2] == '&') {
				restore_runner_opts = 1;
				saved_opts = runner_opts;
				continue;
			}

			if (s[2] != '+' && s[2] != '-' && s[2] != '=') {
				fprintf(stderr, "line %d: O requires +, -, or =: %s\n", linenum, s);
				continue;
			}

			char *fstr;
			for (fstr = &s[3]; *fstr != '\0'; fstr++) {
				switch (*fstr) {
				case '\n': case ' ': case '\t':
					/* ignore */
					break;
				case 'e': opt_arg = opt_arg | RETEST_OPT_ESCAPE_REGEXP; break;
				default:
					fprintf(stderr, "line %d: unknown retest option '%c'\n", linenum, (unsigned char)(*fstr));
				}
			}

			if (s[2] == '=') {
				runner_opts = opt_arg;
			} else if (s[2] == '+') {
				runner_opts = runner_opts | opt_arg;
			} else if (s[2] == '-') {
				runner_opts = runner_opts & (~opt_arg);
			}

			continue;
		}

		if (s[0] == 'M' && s[1] == ' ') {
			char *fstr;
			for (fstr = &s[2]; *fstr != '\0'; fstr++) {
				switch (*fstr) {
				case '\n': case ' ': case '\t':
					/* ignore */
					break;
				case '0': flags = RE_FLAGS_NONE; break;

				case 'i': flags = flags | RE_ICASE;    break;
				case 't': flags = flags | RE_TEXT;     break;
				case 'm': flags = flags | RE_MULTI;    break;
				case 'r': flags = flags | RE_REVERSE;  break;
				case 's': flags = flags | RE_SINGLE;   break;
				case 'z': flags = flags | RE_ZONE;     break;
				case 'a': flags = flags | RE_ANCHORED; break;
				case 'x': flags = flags | RE_EXTENDED; break;
				default:
					fprintf(stderr, "line %d: unknown flag '%c'\n", linenum, (unsigned char)(*fstr));
				}
			}

			continue;
		}

		/* explicit regexp line */
		if (s[0] == '~') {
			s = &s[1];
			len -= 1;
		}

		if (regexp == NULL) {
			static const struct re_err err_zero;

			struct fsm *fsm;
			struct re_err err;
			char *re_str;
			int succ;

			assert(impl_ready == false);
			assert(s          != NULL);

			if (do_timing) {
				t_start = t_zero;
				xclock_gettime(&t_start);
			}

			regexp = xmalloc(len+1);
			memcpy(regexp, s, len+1);

			assert(strlen(regexp) == (size_t)len);
			assert(strcmp(regexp,s) == 0);

			err = err_zero;
			num_regexps++;

			if (runner_opts & RETEST_OPT_ESCAPE_REGEXP) {
				char *err = NULL;
				int newlen = len;
				int ret;

				ret = parse_escapes(regexp, &err, &newlen);
				if (ret != PARSE_OK) {
					fprintf(stderr, "line %d: invalid/incomplete escape sequence at column %d\n",
						linenum, (int) (err - regexp));

					/* ignore errors */
					error_record_add(erec,
						ERROR_ESCAPE_SEQUENCE, fname, regexp, flagdesc, NULL, linenum, dialect);

					num_re_errors++;
					continue;
				}
			}

			flagstring(flags, &flagdesc[0]);

			if (tty_output) {
				char *re  = dup_str_esc(regexp, NULL);
				printf("[      ] line %d: working on %s regexp /%s/%s ...\r",
					linenum, dialect_name, re, flagdesc);
				fflush(stdout);
				free(re);
			}

			re_str = regexp;
			fsm = re_comp(dialect, fsm_sgetc, &re_str, &opt, flags, &err);
			if (fsm == NULL) {
				fprintf(stderr, "line %d: error with %s regexp /%s/%s: %s\n",
					linenum, dialect_name, regexp, flagdesc, re_strerror(err.e));

				/* ignore errors */
				error_record_add(erec,
					ERROR_PARSING_REGEXP, fname, regexp, flagdesc, NULL, linenum, dialect);

				/* don't exit; instead we leave vm==NULL so we
				 * skip to next regexp ... */

				num_re_errors++;
				continue;
			}

			watchdog_tripped = 0;
			if (do_watchdog) {
				/* WARNING! this will leak memory and may cause other issues.  Do not use in production! */
				watchdog_on = 1;
				sigaction(SIGALRM, &watchdog_action_enabled, NULL);
				alarm(watchdog_secs);
			}

			succ = fsm_determinise(fsm) && fsm_minimise(fsm);

			if (do_watchdog) {
				watchdog_on = 0;
				sigaction(SIGALRM, &watchdog_action_disabled, NULL);
				alarm(0);
			}

			if (!succ && watchdog_tripped) {
				watchdog_tripped = 0;

				fprintf(stderr, "line %d: watchdog timer tripped on %s regexp /%s/%s\n",
					linenum, dialect_name, regexp, flagdesc);

				/* ignore errors */
				error_record_add(erec,
					ERROR_WATCHDOG, fname, regexp, flagdesc, NULL, linenum, dialect);

				/* try to free */
				fsm_free(fsm);

				/* don't exit; instead we leave vm==NULL so we
				 * skip to next regexp ... */

				num_re_errors++;
				continue;
			}

			if (!succ) {
				fprintf(stderr, "line %d: error determinising %s regexp /%s/%s: %s\n", linenum, dialect_name, regexp, flagdesc, strerror(errno));

				/* ignore errors */
				error_record_add(erec,
					ERROR_DETERMINISING, fname, regexp, flagdesc, NULL, linenum, dialect);

				/* try to free */
				fsm_free(fsm);

				/* don't exit; instead we leave vm==NULL so we
				 * skip to next regexp ... */

				num_re_errors++;
				continue;
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

#if DEBUG_TEST_REGEXP
			fprintf(stderr, "REGEXP matching for /%s/%s\n", regexp, flagdesc);
#endif /* DEBUG_TEST_REGEXP */

			ret = fsm_runner_initialize(fsm, &runner, impl, vm_opts);

			fsm_free(fsm);

			if (ret != ERROR_NONE) {
				fprintf(stderr, "line %d: error compiling %s regexp /%s/%s: %s\n",
					linenum, dialect_name, regexp, flagdesc, strerror(errno));

				/* ignore errors */
				error_record_add(erec, ret, fname, regexp, flagdesc, NULL, linenum, dialect);

				/* don't exit; instead we leave vm==NULL so we
				 * skip to next regexp ... */

				num_re_errors++;
				continue;
			}

			impl_ready = true;
		} else if (impl_ready) {
			int matching;
			char *orig;
			char *test;
			char *err;
			int tlen;
			int ret;

			if (s[0] != '-' && s[0] != '+') {
				fprintf(stderr, "line %d: unrecognized record type '%c': %s\n", linenum, s[0], s);
				/* ignore errors */
				error_record_add(erec,
					ERROR_BAD_RECORD_TYPE, fname, regexp, flagdesc, NULL, linenum, dialect);

				num_errors++;
				continue;
			}

			num_test_cases++;

			matching = (s[0] == '+');
			orig = &s[1];
			test = strcpy(cpy, orig);
			tlen = len-1;

			err = NULL;
			ret = parse_escapes(test, &err, &tlen);
			if (ret != PARSE_OK) {
				fprintf(stderr, "line %d: invalid/incomplete escape sequence at column %d\n",
					linenum, (int) (err - test));

				/* ignore errors */
				error_record_add(erec,
					ERROR_ESCAPE_SEQUENCE, fname, regexp, flagdesc, NULL, linenum, dialect);

				num_errors++;
				continue;
			}

			ret = fsm_runner_run(&runner, test, tlen);
			if (!!ret == !!matching) {
				char *re  = dup_str_esc(regexp, NULL);
				char *mat = dup_str_esc(orig, NULL);
				printf("[OK    ] line %d: %s regexp /%s/%s %s \"%s\"\n",
					linenum, dialect_name, re, flagdesc, matching ? "matched" : "did not match", mat);
				free(re);
				free(mat);
			} else {
				char *re  = dup_str_esc(regexp, NULL);
				char *mat = dup_str_esc(orig, NULL);
				printf("[NOT OK] line %d: %s regexp /%s/%s expected to %s \"%s\", but %s\n",
					linenum, dialect_name, re, flagdesc,
					matching ? "match" : "not match",
					mat,
					ret ? "did" : "did not");

				free(re);
				free(mat);
				num_errors++;

				/* ignore errors */
				error_record_add(erec,
						(matching)
							? ERROR_SHOULD_MATCH
							: ERROR_SHOULD_NOT_MATCH,
						fname, regexp, flagdesc, orig, linenum, dialect);


				if (max_errors > 0 && num_errors >= max_errors) {
					fprintf(stderr, "too many errors\n");
					goto finish;
				}
			}
		}
	}

	free(regexp);

	if (impl_ready) {
		fsm_runner_finalize(&runner);
		impl_ready = false;
	}

	if (ferror(f)) {
		fprintf(stderr, "line %d: error reading %s: %s\n", linenum, fname, strerror(errno));
		if (regexp != NULL) {
			num_errors++;
		}
	}

	fclose(f);

finish:
	printf("%s: %d regexps, %d test cases\n", fname, num_regexps, num_test_cases);
	printf("%s: %d re errors, %d errors\n", fname, num_re_errors, num_errors);

	if (max_errors > 0 && num_errors >= max_errors) {
		exit(EXIT_FAILURE);
	}

	return num_errors;
}

int
main(int argc, char *argv[])
{
	enum re_dialect dialect;
	enum implementation impl;
	int max_test_errors;

	int optlevel = 1;

	/* is output to a tty or not? */
	tty_output = isatty(fileno(stdout));

	/* note these defaults are the opposite than for fsm(1) */
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	opt.comments          = 1;
	opt.io                = FSM_IO_PAIR;

	dialect   = RE_PCRE;
	impl      = IMPL_INTERPRET;

	max_test_errors = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "O:L:l:x:" "e:E:" "r:" "tw:"), c != -1) {
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
				} else if (strcmp(optarg, "asm") == 0) {
					impl = IMPL_VMASM;
				} else if (strcmp(optarg, "vmc") == 0) {
					impl = IMPL_VMC;
				} else if (strcmp(optarg, "vmops") == 0) {
					impl = IMPL_VMOPS;
				} else if (strcmp(optarg, "go") == 0) {
					impl = IMPL_GO;
				} else if (strcmp(optarg, "goasm") == 0) {
					impl = IMPL_GOASM;
				} else {
					fprintf(stderr, "unknown argument to -l: %s\n", optarg);
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

			case 'e': opt.prefix = optarg;     break;

			case 'r': dialect = parse_dialect_name(optarg); break;

			case 'E':
				max_test_errors = strtoul(optarg, NULL, 10);
				/* XXX: error handling */
				break;

			case 'h':
				usage();
				return EXIT_SUCCESS;

			case 't':
				do_timing = 1;
				break;

			case 'w':
				do_watchdog = 1;
				watchdog_secs = strtoul(optarg, NULL, 10);
				if (watchdog_secs < 1) {
					do_watchdog = 0;
				}
				break;

			case '?':
			default:
				usage();
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (do_watchdog) {
		opt.alloc = &watchdog_alloc;
	}

	if (argc < 1) {
		usage();
		return EXIT_FAILURE;
	}

	if (optlevel > 0) {
		vm_opts.flags |= FSM_VM_COMPILE_OPTIM;
	}

	{
		int r, i;
		struct error_record erec;


		r = 0;

		if (error_record_init(&erec) != 0) {
			fprintf(stderr, "error initializing error state: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		for (i = 0; i < argc; i++) {
			int nerrs;

			nerrs = process_test_file(argv[i], dialect, impl, max_test_errors, &erec);

			if (nerrs > 0) {
				r |= 1;
				continue;
			}
		}

		if (erec.len > 0) {
			error_record_print(stderr, &erec);
			fprintf(stderr, "%u errors\n", (unsigned) erec.len);
		} else {
			fprintf(stderr, "no errors\n");
		}

		error_record_finalize(&erec);

		return r;
	}
}

