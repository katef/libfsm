/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> /* XXX: for ast.h */
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

static void
usage(void)
{
	fprintf(stderr, "usage: re    [-r <dialect>] [-nbiusyz] [-x] <re> ... [ <text> | -- <text> ... ]\n");
	fprintf(stderr, "       re    [-r <dialect>] [-nbiusyz] {-q <query>} <re> ...\n");
	fprintf(stderr, "       re -p [-r <dialect>] [-nbiusyz] [-l <language>] [-acwX] [-k <io>] [-e <prefix>] <re> ...\n");
	fprintf(stderr, "       re -m [-r <dialect>] [-nbiusyz] <re> ...\n");
	fprintf(stderr, "       re -h\n");
}

static enum fsm_io
io(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum fsm_io io;
	} a[] = {
		{ "getc", FSM_IO_GETC },
		{ "str",  FSM_IO_STR  },
		{ "pair", FSM_IO_PAIR }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].io;
		}
	}

	fprintf(stderr, "unrecognised IO API; valid IO APIs are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static void
print_name(const char *name,
	fsm_print **print_fsm, ast_print **print_ast)
{
	size_t i;

	struct {
		const char *name;
		fsm_print *print_fsm;
		ast_print *print_ast;
	} a[] = {
		{ "api",    fsm_print_api,    NULL },
		{ "c",      fsm_print_c,      NULL },
		{ "dot",    fsm_print_dot,    NULL },
		{ "fsm",    fsm_print_fsm,    NULL },
		{ "ir",     fsm_print_ir,     NULL },
		{ "irjson", fsm_print_irjson, NULL },
		{ "json",   fsm_print_json,   NULL },
		{ "vmc",    fsm_print_vmc,    NULL },
		{ "vmdot",  fsm_print_vmdot,  NULL },
		{ "sh",     fsm_print_sh,     NULL },
		{ "go",     fsm_print_go,     NULL },

		{ "amd64",      fsm_print_vmasm,            NULL },
		{ "amd64_att",  fsm_print_vmasm_amd64_att,  NULL },
		{ "amd64_nasm", fsm_print_vmasm_amd64_nasm, NULL },
		{ "amd64_go",   fsm_print_vmasm_amd64_go,   NULL },

		{ "tree",   NULL, ast_print_tree },
		{ "abnf",   NULL, ast_print_abnf },
		{ "ast",    NULL, ast_print_dot  },
		{ "pcre",   NULL, ast_print_pcre }
	};

	assert(name != NULL);
	assert(print_fsm != NULL);
	assert(print_ast != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			*print_fsm = a[i].print_fsm;
			*print_ast = a[i].print_ast;
			return;
		}
	}

	fprintf(stderr, "unrecognised output language; valid languages are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
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

static int
(*comparison(const char *name))
(const struct fsm *, const struct fsm *)
{
	size_t i;

	struct {
		const char *name;
		int (*comparison)(const struct fsm *, const struct fsm *);
	} a[] = {
		{ "equal",    fsm_equal },
		{ "isequal",  fsm_equal },
		{ "areequal", fsm_equal }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].comparison;
		}
	}

	fprintf(stderr, "unrecognised comparison; valid comparisons are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
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

static struct match *
addmatch(struct match **head, int i, const char *s)
{
	struct match *new;

	assert(head != NULL);
	assert(s != NULL);

	if ((1U << i) > INT_MAX) {
		fprintf(stderr, "Too many patterns for int bitmap\n");
		exit(EXIT_FAILURE);
	}

	/* TODO: explain we find duplicate; return success */
	/*
	 * This is a purposefully shallow comparison (rather than calling strcmp)
	 * so that 're xyz xyz' will find the ambiguity.
	 */
	{
		struct match *p;

		for (p = *head; p != NULL; p = p->next) {
			if (p->s == s) {
				return p;
			}
		}
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->i = i;
	new->s = s;

	new->next = *head;
	*head     = new;

	return new;
}

static void
carryopaque(struct fsm *src_fsm, const fsm_state_t *src_set, size_t n,
	struct fsm *dst_fsm, fsm_state_t dst_state)
{
	struct match *matches;
	struct match *m;
	size_t i;

	assert(src_fsm != NULL);
	assert(src_set != NULL);
	assert(n > 0);
	assert(dst_fsm != NULL);
	assert(fsm_isend(dst_fsm, dst_state));
	assert(fsm_getopaque(dst_fsm, dst_state) == NULL);

	/*
	 * Here we mark newly-created DFA states with the same regexp string
	 * as from their corresponding source NFA states.
	 *
	 * Because all the accepting states are reachable together, they
	 * should all share the same regexp, unless re(1) was invoked with
	 * a regexp which is a subset of (or equal to) another.
	 */

	matches = NULL;

	for (i = 0; i < n; i++) {
		/*
		 * The opaque data is attached to end states only, so we skip
		 * non-end states here.
		 */
		if (!fsm_isend(src_fsm, src_set[i])) {
			continue;
		}

		assert(fsm_getopaque(src_fsm, src_set[i]) != NULL);

		for (m = fsm_getopaque(src_fsm, src_set[i]); m != NULL; m = m->next) {
			if (!addmatch(&matches, m->i, m->s)) {
				perror("addmatch");
				goto error;
			}
		}
	}

	fsm_setopaque(dst_fsm, dst_state, matches);

	return;

error:

	/* XXX: free matches */

	fsm_setopaque(dst_fsm, dst_state, NULL);

	return;
}

static void
printexample(FILE *f, const struct fsm *fsm, fsm_state_t state)
{
	char buf[256]; /* TODO */
	int n;

	assert(f != NULL);
	assert(fsm != NULL);

	n = fsm_example(fsm, state, buf, sizeof buf);
	if (-1 == n) {
		perror("fsm_example");
		exit(EXIT_FAILURE);
	}

	/* TODO: escape hex etc */
	fprintf(f, "%s%s", buf,
		n >= (int) sizeof buf - 1 ? "..." : "");
}

static int
endleaf_c(FILE *f, const void *state_opaque, const void *endleaf_opaque)
{
	const struct match *m;
	int n;

	assert(state_opaque != NULL);
	assert(endleaf_opaque == NULL);

	(void) f;
	(void) endleaf_opaque;

	n = 0;

	for (m = state_opaque; m != NULL; m = m->next) {
		n |= 1 << m->i;
	}

	fprintf(f, "return %#x;", (unsigned) n);

	fprintf(f, " /* ");

	for (m = state_opaque; m != NULL; m = m->next) {
		fprintf(f, "\"%s\"", m->s); /* XXX: escape string (and comment) */

		if (m->next != NULL) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, " */");

	return 0;
}

static int
endleaf_dot(FILE *f, const void *state_opaque, const void *endleaf_opaque)
{
	const struct match *m;

	assert(f != NULL);
	assert(state_opaque != NULL);
	assert(endleaf_opaque == NULL);

	(void) endleaf_opaque;

	fprintf(f, "label = <");

	for (m = state_opaque; m != NULL; m = m->next) {
		fprintf(f, "#%u", m->i);

		if (m->next != NULL) {
			fprintf(f, ",");
		}
	}

	fprintf(f, ">");

	/* TODO: only if comments */
	/* TODO: centralise to libfsm/print/dot.c */

#if 0
	fprintf(f, " # ");

	for (m = state_opaque; m != NULL; m = m->next) {
		fprintf(f, "\"%s\"", m->s); /* XXX: escape string (and comment) */

		if (m->next != NULL) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, "\n");
#endif

	return 0;
}

static int
endleaf_json(FILE *f, const void *state_opaque, const void *endleaf_opaque)
{
	const struct match *m;

	assert(f != NULL);
	assert(state_opaque != NULL);
	assert(endleaf_opaque == NULL);

	(void) endleaf_opaque;

	fprintf(f, "[ ");

	for (m = state_opaque; m != NULL; m = m->next) {
		fprintf(f, "%u", m->i);

		if (m->next != NULL) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, " ]");

	/* TODO: only if comments */
	/* TODO: centralise to libfsm/print/json.c */

	return 0;
}

int
main(int argc, char *argv[])
{
	struct fsm *(*join)(struct fsm *, struct fsm *);
	int (*query)(const struct fsm *, const struct fsm *);
	fsm_print *print_fsm;
	ast_print *print_ast;
	enum re_dialect dialect;
	struct fsm *fsm;
	enum re_flags flags;
	int xfiles, yfiles;
	int example;
	int keep_nfa;
	int patterns;
	int ambig;
	int makevm;

	struct fsm_dfavm *vm;

	/* note these defaults are the opposite than for fsm(1) */
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	opt.comments          = 1;
	opt.io                = FSM_IO_GETC;

	flags     = 0U;
	xfiles    = 0;
	yfiles    = 0;
	example   = 0;
	keep_nfa  = 0;
	patterns  = 0;
	ambig     = 0;
	makevm    = 0;
	print_fsm = NULL;
	print_ast = NULL;
	query     = NULL;
	join      = fsm_union;
	dialect   = RE_NATIVE;
	vm        = NULL;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acwXe:k:" "bi" "sq:r:l:" "upMmnxyz"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 0;          break;
			case 'c': opt.consolidate_edges = 0;          break;
			case 'w': opt.fragment          = 1;          break;
			case 'X': opt.always_hex        = 1;          break;
			case 'e': opt.prefix            = optarg;     break;
			case 'k': opt.io                = io(optarg); break;

			case 'b': flags |= RE_ANCHORED; break;
			case 'i': flags |= RE_ICASE;    break;

			case 's':
				join = fsm_concat;
				break;

			case 'l':
				print_name(optarg, &print_fsm, &print_ast);
				break;

			case 'p': print_fsm = fsm_print_fsm;        break;
			case 'q': query     = comparison(optarg);   break;
			case 'r': dialect   = dialect_name(optarg); break;

			case 'u': ambig    = 1; break;
			case 'x': xfiles   = 1; break;
			case 'y': yfiles   = 1; break;
			case 'm': example  = 1; break;
			case 'n': keep_nfa = 1; break;
			case 'z': patterns = 1; break;
			case 'M': makevm   = 1; break;

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

	if (!!print_fsm + !!print_ast + example + !!query > 1) {
		fprintf(stderr, "-m, -p and -q are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	if (!!print_fsm + !!print_ast + example + !!query && xfiles) {
		fprintf(stderr, "-x applies only when executing\n");
		return EXIT_FAILURE;
	}

	if (makevm && (keep_nfa || example || query)) {
		fprintf(stderr, "-M cannot be used with -m, -q, or -n\n");
		return EXIT_FAILURE;
	}

	if (patterns && !!query) {
		fprintf(stderr, "-z does not apply for querying\n");
		return EXIT_FAILURE;
	}

	if (join == fsm_concat && patterns) {
		fprintf(stderr, "-s is not implemented when printing patterns by -z\n");
		return EXIT_FAILURE;
	}

	if (print_fsm == NULL) {
		keep_nfa = 0;
	}

	if (keep_nfa) {
		ambig = 1;
	}

	/* XXX: repetitive */
	if (print_ast != NULL) {
		struct ast *ast;
		struct re_err err;

		if (argc != 1) {
			fprintf(stderr, "single regexp only for this output format\n");
			return EXIT_FAILURE;
		}

		if (yfiles) {
			FILE *f;

			f = xopen(argv[0]);

			ast = re_parse(dialect, fsm_fgetc, f, &opt, flags, &err, NULL);

			fclose(f);
		} else {
			const char *s;

			s = argv[0];

			ast = re_parse(dialect, fsm_sgetc, &s, &opt, flags, &err, NULL);
		}

		if (ast == NULL) {
			re_perror(dialect, &err,
				 yfiles ? argv[0] : NULL,
				!yfiles ? argv[0] : NULL);

			if (err.e == RE_EXUNSUPPORTD) {
				return 2;
			}

			return EXIT_FAILURE;
		}

		print_ast(stdout, &opt, ast);

		return 0;
	}

	flags |= RE_MULTI;

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		perror("fsm_new");
		return EXIT_FAILURE;
	}

	{
		int i;

		for (i = 0; i < argc - !(print_fsm || example || !!query || argc <= 1); i++) {
			struct re_err err;
			struct fsm *new, *q;

			/* TODO: handle possible "dialect:" prefix */

			if (0 == strcmp(argv[i], "--")) {
				argc--;
				argv++;

				break;
			}

			if (yfiles) {
				FILE *f;

				f = xopen(argv[i]);

				new = re_comp_new(dialect, fsm_fgetc, f, &opt, flags, &err);

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				new = re_comp_new(dialect, fsm_sgetc, &s, &opt, flags, &err);
			}

			if (new == NULL) {
				re_perror(dialect, &err,
					 yfiles ? argv[i] : NULL,
					!yfiles ? argv[i] : NULL);

				if (err.e == RE_EXUNSUPPORTD) {
					return 2;
				}

				return EXIT_FAILURE;
			}

			if (!keep_nfa) {
				if (!fsm_determinise(new)) {
					perror("fsm_determinise");
					return EXIT_FAILURE;
				}
				if (!fsm_minimise(new)) {
					perror("fsm_minimise");
					return EXIT_FAILURE;
				}
			}

			{
				size_t s;

				/*
				 * Attach this mapping to each end state for this regexp.
				 * XXX: we should share the same matches struct for all end states
				 * in the same regexp, and keep an argc-sized array of pointers to free().
				 * XXX: then use fsm_setendopaque() here.
				 */
				for (s = 0; s < new->statecount; s++) {
					if (fsm_isend(new, s)) {
						struct match *matches;

						matches = NULL;

						if (!addmatch(&matches, i, argv[i])) {
							perror("addmatch");
							return EXIT_FAILURE;
						}

						assert(fsm_getopaque(new, s) == NULL);
						fsm_setopaque(new, s, matches);
					}
				}
			}

			/* TODO: implement concatenating patterns for -s in conjunction with -z.
			 * Note that depends on the regexp dialect */

			if (query != NULL) {
				q = fsm_clone(new);
				if (q == NULL) {
					perror("fsm_clone");
					return EXIT_FAILURE;
				}
			}

			/* TODO: instead of re_comp_new() above, could parse directly into the running fsm */
			fsm = join(fsm, new);
			if (fsm == NULL) {
				perror("fsm_union/concat");
				return EXIT_FAILURE;
			}

			if (query != NULL) {
				int r;

				r = query(fsm, q);
				if (r == -1) {
					perror("query");
					return EXIT_FAILURE;
				}

				if (r == 0) {
					return EXIT_FAILURE;
				}

				fsm_free(q);
			}
		}

		argc -= i;
		argv += i;
	}

	if (query) {
		return EXIT_SUCCESS;
	}

	if ((print_fsm || example) && argc > 0) {
		fprintf(stderr, "too many arguments\n");
		return EXIT_FAILURE;
	}

	if (!ambig) {
		struct fsm *dfa;
		size_t s;

		dfa = fsm_clone(fsm);
		if (dfa == NULL) {
			perror("fsm_clone");
			return EXIT_FAILURE;
		}

		{
			opt.carryopaque = carryopaque;

			if (!fsm_determinise(dfa)) {
				perror("fsm_determinise");
				return EXIT_FAILURE;
			}

			opt.carryopaque = NULL;
		}

		for (s = 0; s < dfa->statecount; s++) {
			const struct match *matches;

			if (!fsm_isend(dfa, s)) {
				continue;
			}

			matches = fsm_getopaque(dfa, s);
			assert(matches != NULL);

			if (matches->next != NULL) {
				const struct match *m;

				/* TODO: // delimeters depend on dialect */
				/* TODO: would deal with dialect: prefix here, too */

				fprintf(stderr, "ambiguous matches for ");
				for (m = matches; m != NULL; m = m->next) {
					/* TODO: print nicely */
					fprintf(stderr, "/%s/", m->s);
					if (m->next != NULL) {
						fprintf(stderr, ", ");
					}
				}

				fprintf(stderr, "; for example on input '");
				printexample(stderr, dfa, s);
				fprintf(stderr, "'\n");

				/* TODO: consider different error codes */
				return EXIT_FAILURE;
			}
		}

		/* TODO: free opaques */

		fsm_free(dfa);
	}

	if (!keep_nfa) {
		opt.carryopaque = carryopaque;

		/*
		 * Convert to a DFA, then minimise unless we need to keep the end
		 * state information separated per regexp. */
		if (!fsm_determinise(fsm)) {
			perror("fsm_determinise");
			return EXIT_FAILURE;
		}

		if (!patterns && !example && print_fsm != fsm_print_c) {
			if (!fsm_minimise(fsm)) {
				perror("fsm_minimise");
				return EXIT_FAILURE;
			}
		}

		opt.carryopaque = NULL;

		if (makevm) {
			vm = fsm_vm_compile(fsm);
		}
	}

	if (example) {
		fsm_state_t s;

		for (s = 0; s < fsm->statecount; s++) {
			if (!fsm_isend(fsm, s)) {
				continue;
			}

			/* TODO: would deal with dialect: prefix here, too */
			if (patterns) {
				const struct match *m;

				assert(fsm_getopaque(fsm, s) != NULL);

				for (m = fsm_getopaque(fsm, s); m != NULL; m = m->next) {
					/* TODO: print nicely */
					printf("/%s/", m->s);
					if (m->next != NULL) {
						printf(", ");
					}
				}

				printf(": ");
			}

			printexample(stdout, fsm, s);
			printf("\n");
		}

/* XXX: free fsm */

		return 0;
	}

	if (print_fsm != NULL) {
		/* TODO: print examples in comments for end states;
		 * patterns in comments for the whole FSM */

		if (print_fsm == fsm_print_c || print_fsm == fsm_print_vmc) {
			opt.endleaf = endleaf_c;
		} else if (print_fsm == fsm_print_dot || print_fsm == fsm_print_vmdot) {
			opt.endleaf = patterns ? endleaf_dot : NULL;
		} else if (print_fsm == fsm_print_json) {
			opt.endleaf = patterns ? endleaf_json : NULL;
		}

		print_fsm(stdout, fsm);

/* XXX: free fsm */
		
		if (vm != NULL) {
			fsm_vm_free(vm);
		}

		return 0;
	}

	/* execute */
	{
		int r;

		r = 0;

		if (argc > 0) {
			int i;

			/* TODO: option to print input texts which match. like grep(1) does.
			 * This is not the same as printing patterns which match (by associating
			 * a pattern to the end state), like lx(1) does */

			for (i = 0; i < argc; i++) {
				fsm_state_t state;
				int e;

				if (xfiles) {
					FILE *f;

					f = xopen(argv[i]);

					if (vm != NULL) {
						e = fsm_vm_match_file(vm, f);
					} else {
						e = fsm_exec(fsm, fsm_fgetc, f, &state);
					}

					fclose(f);
				} else {
					const char *s;

					s = argv[i];

					if (vm != NULL) {
						e = fsm_vm_match_buffer(vm, s, strlen(s));
					} else {
						e = fsm_exec(fsm, fsm_sgetc, &s, &state);
					}
				}

				if (e != 1) {
					r |= 1;
					continue;
				}

				if (patterns) {
					const struct match *m;

					assert(fsm_getopaque(fsm, state) != NULL);

					for (m = fsm_getopaque(fsm, state); m != NULL; m = m->next) {
						/* TODO: print nicely */
						printf("match: /%s/\n", m->s);
					}
				}
			}
		}

		/* XXX: free opaques */

		fsm_free(fsm);

		if (vm != NULL) {
			fsm_vm_free(vm);
		}

		return r;
	}
}

