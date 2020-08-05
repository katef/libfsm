/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>
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

static struct fsm_options opt;
static struct fsm_vm_compile_opts vm_opts = { 0, FSM_VM_COMPILE_VM_V1, NULL };

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

static enum re_dialect
dialect_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum re_dialect dialect;
	} a[] = {
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
				unsigned char uc = toupper(s[i]);
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

		s[j++] = ccode;
		break;
	}

	s[j] = '\0';

	*lenp = j;

	return PARSE_OK;
}

struct single_error_record {
	char *filename;
	char *regexp;
	char *failed_match;
	unsigned int line;
	enum error_type type;
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
error_record_add(struct error_record *erec, enum error_type type, const char *fn, const char *re, const char *failed_match, unsigned int line)
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
	erec->errors[ind].regexp       = re != NULL           ? dup_str(re,&err)           : NULL;
	erec->errors[ind].failed_match = failed_match != NULL ? dup_str(failed_match,&err) : NULL;
	erec->errors[ind].line         = line;
	erec->errors[ind].type         = type;

	erec->len++;

	return err;
}

static void
single_error_record_print(FILE *f, const struct single_error_record *err)
{
	fprintf(f, "(%s:%u) ", err->filename, err->line);
	switch (err->type) {
	case ERROR_NONE:               fprintf(f, "no error?\n");  break;
	case ERROR_BAD_RECORD_TYPE:    fprintf(f, "bad record\n"); break;
	case ERROR_ESCAPE_SEQUENCE:    fprintf(f, "bad escape sequence\n"); break;
	case ERROR_PARSING_REGEXP:     fprintf(f, "error parsing regexp /%s/\n", err->regexp); break;
	case ERROR_DETERMINISING:
		fprintf(f, "error determinising regexp /%s/\n", err->regexp);
		break;
	case ERROR_COMPILING_BYTECODE:
		fprintf(f, "error compiling regexp /%s/\n", err->regexp);
		break;
	case ERROR_SHOULD_MATCH:
		fprintf(f, "regexp /%s/ should match \"%s\" but doesn't\n",
			err->regexp, err->failed_match);
		break;
	case ERROR_SHOULD_NOT_MATCH:
		fprintf(f, "regexp /%s/ should not match \"%s\" but does\n",
			err->regexp, err->failed_match);
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
		free(erec->errors[i].failed_match);
	}

	free(erec->errors);
	erec->errors = NULL;
	erec->len = erec->cap = 0;
}

/* test file format:
 *
 * 1. lines starting with '#' are skipped
 * 2. (TODO) lines starting with 'R' set the dialect
 * 3. records are separated by empty lines
 * 4. the first line of each record is the regular expression to be
 *    tested 
 * 5. after the regular expression, valid lines start with '#', '+', or '-'
 *     a. '+' indicates that the rest of the line should be matched with the
 *        current regular expression
 *     b. '-' indicates the the rest of the line should *not* be matched with
 *        the current regular expression
 *     c. '#' lines are comments
 */
static int
process_test_file(const char *fname, enum re_dialect dialect, enum implementation impl, int max_errors, struct error_record *erec)
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

	enum error_type ret;
	bool impl_ready = false;
	struct fsm_runner runner = init_runner;

	regexp = NULL;

	/* XXX - fix this */
	opt.comments = 0;

	f = xopen(fname);

	num_regexps    = 0;
	num_re_errors  = 0;
	num_test_cases = 0;
	num_errors     = 0;

	linenum        = 0;

	while (s = fgets(buf, sizeof buf, f), s != NULL) {
		int len;

		linenum++;
		len = strlen(s);
		if (len > 0 && s[len-1] == '\n') {
			s[--len] = '\0';
		}

		if (len == 0) {
			free(regexp);
			regexp = NULL;

			if (impl_ready) {
				fsm_runner_finalize(&runner);
				runner     = init_runner;
				impl_ready = false;
			}

			continue;
		}

		if (s[0] == '#') {
			continue;
		}

		if (regexp == NULL) {
			static const struct re_err err_zero;

			struct fsm *fsm;
			struct re_err err;
			enum re_flags flags;
			char *re_str;

			assert(impl_ready == false);
			assert(s          != NULL);

			regexp = xmalloc(len+1);
			memcpy(regexp, s, len+1);

			assert(strlen(regexp) == (size_t)len);
			assert(strcmp(regexp,s) == 0);

			flags = 0;
			err   = err_zero;

			num_regexps++;

			re_str = regexp;
			fsm = re_comp_new(dialect, fsm_sgetc, &re_str, &opt, flags, &err);
			if (fsm == NULL) {
				fprintf(stderr, "line %d: error with regexp /%s/: %s\n",
					linenum, regexp, re_strerror(err.e));

				/* ignore errors */
				error_record_add(erec,
					ERROR_PARSING_REGEXP, fname, regexp, NULL, linenum);

				/* don't exit; instead we leave vm==NULL so we
				 * skip to next regexp ... */

				num_re_errors++;
				continue;
			}

			/* XXX - minimize or determinize? */
			if (!fsm_determinise(fsm)) {
				fprintf(stderr, "line %d: error determinising /%s/: %s\n", linenum, regexp, strerror(errno));

				/* ignore errors */
				error_record_add(erec,
					ERROR_DETERMINISING, fname, regexp, NULL, linenum);

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
			fprintf(stderr, "REGEXP matching for /%s/\n", regexp);
#endif /* DEBUG_TEST_REGEXP */

			ret = fsm_runner_initialize(fsm, &runner, impl, vm_opts);
			if (ret != ERROR_NONE) {
				fprintf(stderr, "line %d: error compiling /%s/: %s\n", linenum, regexp, strerror(errno));

				/* ignore errors */
				error_record_add(erec, ret, fname, regexp, NULL, linenum);

				/* don't exit; instead we leave vm==NULL so we
				 * skip to next regexp ... */

				num_re_errors++;
				continue;
			}

			impl_ready = true;

			fsm_free(fsm);
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
					ERROR_BAD_RECORD_TYPE, fname, regexp, NULL, linenum);

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
					ERROR_ESCAPE_SEQUENCE, fname, regexp, NULL, linenum);

				num_errors++;
				continue;
			}

			ret = fsm_runner_run(&runner, test, tlen);
			if (!!ret == !!matching) {
				printf("[OK    ] line %d: regexp /%s/ %s \"%s\"\n",
					linenum, regexp, matching ? "matched" : "did not match", orig);
			} else {
				printf("[NOT OK] line %d: regexp /%s/ expected to %s \"%s\", but %s\n",
					linenum, regexp,
					matching ? "match" : "not match",
					orig,
					ret ? "did" : "did not");
				num_errors++;

				/* ignore errors */
				error_record_add(erec,
						(matching)
							? ERROR_SHOULD_MATCH
							: ERROR_SHOULD_NOT_MATCH,
						fname, regexp, orig, linenum);


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

	fclose(f);

	if (ferror(f)) {
		fprintf(stderr, "line %d: error reading %s: %s\n", linenum, fname, strerror(errno));
		if (regexp != NULL) {
			num_errors++;
		}
	}

finish:
	printf("%s: %d regexps, %d test cases\n", fname, num_regexps, num_test_cases);
	printf("%s: %d re errors, %d errors\n", fname, num_re_errors, num_errors);

	if (max_errors > 0 && num_errors >= max_errors) {
		exit(EXIT_FAILURE);
	}

	return (num_errors > 0);
}

int
main(int argc, char *argv[])
{
	enum re_dialect dialect;
	enum implementation impl;
	int max_test_errors;

	int optlevel = 1;

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

		while (c = getopt(argc, argv, "h" "O:L:l:x:" "e:E:" "r:" ), c != -1) {
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

			case 'r': dialect = dialect_name(optarg); break;

			case 'E':
				max_test_errors = strtoul(optarg, NULL, 10);
				/* XXX: error handling */
				break;

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

	{
		int r, i;
		struct error_record erec;


		r = 0;

		if (argc == 0) {
			fprintf(stderr, "-t requires at least one test file\n");
			return EXIT_FAILURE;
		}

		if (error_record_init(&erec) != 0) {
			fprintf(stderr, "error initializing error state: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		for (i = 0; i < argc; i++) {
			int succ;

			succ = process_test_file(argv[i], dialect, impl, max_test_errors, &erec);

			if (!succ) {
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

