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
#include <fsm/parser.h>
#include <fsm/vm.h>
#include <fsm/walk.h>

#include <re/re.h>
#include <re/literal.h>

#include <print/esc.h>

#include <adt/xalloc.h>

#include "libfsm/internal.h" /* XXX */
#include "libre/class.h" /* XXX */
#include "libre/ast.h" /* XXX */
#include "libre/ast_new_from_fsm.h" /* XXX */
#include "libre/print.h" /* XXX */

#define DEBUG_ESCAPES     0
#define DEBUG_VM_FSM      0
#define DEBUG_TEST_REGEXP 0

/*
 * TODO: accepting a delimiter would be useful: /abc/. perhaps provide that as
 * a convenience function, especially wrt. escaping for lexing. Also convenient
 * for specifying flags: /abc/g
 * TODO: flags; -r for RE_REVERSE, etc
 */

static char *const *matchv;
static size_t matchc;

static void
usage(void)
{
	fprintf(stderr, "usage: re    [-r <dialect>] [-nbiusfyz] [-x] <re> ... [ <text> | -- <text> ... ]\n");
	fprintf(stderr, "       re    [-r <dialect>] [-nbiusfyz] {-q <query>} <re> ...\n");
	fprintf(stderr, "       re -p [-r <dialect>] [-nbiusfyz] [-l <language>] [-acwX] [-k <io>] [-E <package_prefix>] [-e <prefix>] <re> ...\n");
	fprintf(stderr, "       re -m [-r <dialect>] [-nbiusfyz] <re> ...\n");
	fprintf(stderr, "       re -G <max_length> [-r <dialect>] [-biu] <re>\n");
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

/* TODO: centralise */
static void
lang_name(const char *name, enum fsm_print_lang *fsm_lang, enum ast_print_lang *ast_lang)
{
	size_t i;

	const struct {
		const char *name;
		enum fsm_print_lang lang;
	} a[] = {
		{ "amd64",      FSM_PRINT_AMD64_NASM },
		{ "amd64_att",  FSM_PRINT_AMD64_ATT  },
		{ "amd64_go",   FSM_PRINT_AMD64_GO   },
		{ "amd64_nasm", FSM_PRINT_AMD64_NASM },

		{ "api",        FSM_PRINT_API        },
		{ "awk",        FSM_PRINT_AWK        },
		{ "c",          FSM_PRINT_C          },
		{ "dot",        FSM_PRINT_DOT        },
		{ "fsm",        FSM_PRINT_FSM        },
		{ "go",         FSM_PRINT_GO         },
		{ "ir",         FSM_PRINT_IR         },
		{ "irjson",     FSM_PRINT_IRJSON     },
		{ "json",       FSM_PRINT_JSON       },
		{ "llvm",       FSM_PRINT_LLVM       },
		{ "rust",       FSM_PRINT_RUST       },
		{ "sh",         FSM_PRINT_SH         },
		{ "vmc",        FSM_PRINT_VMC        },

		{ "vmdot",      FSM_PRINT_VMDOT      },
		{ "vmops_c",    FSM_PRINT_VMOPS_C    },
		{ "vmops_h",    FSM_PRINT_VMOPS_H    },
		{ "vmops_main", FSM_PRINT_VMOPS_MAIN }
	};

	const struct {
		const char *name;
		enum ast_print_lang lang;
	} b[] = {
		{ "abnf",       AST_PRINT_ABNF       },
		{ "ast",        AST_PRINT_DOT        },
		{ "pcre",       AST_PRINT_PCRE       },
		{ "tree",       AST_PRINT_TREE       }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			*fsm_lang = a[i].lang;
			return;
		}
	}

	for (i = 0; i < sizeof b / sizeof *b; i++) {
		if (0 == strcmp(b[i].name, name)) {
			*ast_lang = b[i].lang;
			return;
		}
	}

	fprintf(stderr, "unrecognised output language; valid languages are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			", ");
	}

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
conflict(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata, const char *example,
	void *hook_opaque)
{
	size_t i;

	assert(opt != NULL);
	assert(opt->ambig == AMBIG_ERROR);
	assert(state_metadata->end_id_count > 1);

	(void) f;
	(void) opt;
	(void) hook_opaque;

	/* TODO: // delimeters depend on dialect */
	/* TODO: would deal with dialect: prefix here, too */

	fprintf(stderr, "ambiguous matches for ");

	for (i = 0; i < state_metadata->end_id_count; i++) {
		assert(state_metadata->end_ids[i] < matchc);

		/* TODO: print nicely */
		fprintf(stderr, "/%s/", matchv[state_metadata->end_ids[i]]);

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(stderr, ", ");
		}
	}

	if (example != NULL) {
		/* TODO: escape hex etc. or perhaps make_ir() did that for us */
		fprintf(stderr, "; for example on input '%s'", example);
	}

	fprintf(stderr, "\n");

	return 0;
}

static int
comment_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *hook_opaque)
{
	size_t i;

	assert(opt != NULL);
	assert(hook_opaque == NULL);

	(void) opt;
	(void) hook_opaque;

	fprintf(f, "/* ");

	for (i = 0; i < state_metadata->end_id_count; i++) {
		assert(state_metadata->end_ids[i] < matchc);

		fprintf(f, "\"%s\"", matchv[state_metadata->end_ids[i]]); /* XXX: escape string (and comment) */

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, " */");

	return 0;
}

static int
comment_rust(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *hook_opaque)
{
	size_t i;

	assert(opt != NULL);
	assert(hook_opaque == NULL);

	(void) opt;
	(void) hook_opaque;

	fprintf(f, "// ");

	for (i = 0; i < state_metadata->end_id_count; i++) {
		assert(state_metadata->end_ids[i] < matchc);

		fprintf(f, "\"%s\"", matchv[state_metadata->end_ids[i]]); /* XXX: escape string (and comment) */

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(f, ", ");
		}
	}

	return 0;
}

static int
comment_llvm(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *hook_opaque)
{
	size_t i;

	assert(opt != NULL);
	assert(hook_opaque == NULL);

	(void) opt;
	(void) hook_opaque;

	fprintf(f, "; ");

	for (i = 0; i < state_metadata->end_id_count; i++) {
		assert(state_metadata->end_ids[i] < matchc);

		fprintf(f, "\"%s\"", matchv[state_metadata->end_ids[i]]); /* XXX: escape string (and comment) */

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(f, ", ");
		}
	}

	return 0;
}

static int
comment_dot(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *hook_opaque)
{
	size_t i;

	assert(opt != NULL);
	assert(hook_opaque == NULL);

	(void) opt;
	(void) hook_opaque;

	fprintf(f, "/* ");

	for (i = 0; i < state_metadata->end_id_count; i++) {
		assert(state_metadata->end_ids[i] < matchc);

		fprintf(f, "\"%s\"", matchv[state_metadata->end_ids[i]]); /* XXX: escape string (and comment) */

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, " */");

	return 0;
}

static struct fsm *fsm_to_cleanup = NULL;

static void
do_fsm_cleanup(void)
{
	if (fsm_to_cleanup != NULL) {
		fsm_free(fsm_to_cleanup);
		fsm_to_cleanup = NULL;
	}
}

static void
parse_flags(const char *arg, enum re_flags *flags)
{
	for (; *arg; arg++) {
		switch (*arg) {
		case 'b':
			*flags = *flags | RE_ANCHORED;
			break;

		case 'i':
			*flags = *flags | RE_ICASE;
			break;

		case 's':
			*flags = *flags | RE_SINGLE;
			break;

		case 'x':
			*flags = *flags | RE_EXTENDED;
			break;

		/* others? */

		default:
			printf("unknown flag '%c'\n", *arg);
			usage();
			exit(EXIT_FAILURE);
		}
	}
}

int
main(int argc, char *argv[])
{
	static const struct fsm_options zero_options;
	static const struct fsm_hooks zero_hooks;

	/* TODO: use alloc hooks for -Q accounting */
	struct fsm_alloc *alloc = NULL;
	struct fsm_options opt;
	struct fsm_hooks hooks;

	struct fsm *fsm;
	struct fsm *(*join)(struct fsm *, struct fsm *,
	    struct fsm_combine_info *);
	int (*query)(const struct fsm *, const struct fsm *);
	enum fsm_print_lang fsm_lang;
	enum ast_print_lang ast_lang;
	enum re_dialect dialect;
	enum re_flags flags;
	size_t generate_bounds = 0;
	int xfiles, yfiles;
	int fsmfiles;
	int example;
	int isliteral;
	int keep_nfa;
	int patterns;
	int makevm;

	struct fsm_dfavm *vm;

	atexit(do_fsm_cleanup);

	opt = zero_options;
	hooks = zero_hooks;

	/* note these defaults are the opposite than for fsm(1) */
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	opt.ambig             = AMBIG_ERROR;
	opt.comments          = 1;
	opt.io                = FSM_IO_GETC;

	flags     = 0U;
	fsmfiles  = 0;
	xfiles    = 0;
	yfiles    = 0;
	example   = 0;
	isliteral = 0;
	keep_nfa  = 0;
	patterns  = 0;
	makevm    = 0;
	fsm_lang  = FSM_PRINT_NONE;
	ast_lang  = AST_PRINT_NONE;
	query     = NULL;
	join      = fsm_union;
	dialect   = RE_NATIVE;
	vm        = NULL;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acCwXe:E:G:k:" "bi" "sq:r:l:F:" "upMmnftxyz"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 0;          break;
			case 'c': opt.consolidate_edges = 0;          break;
			case 'C': opt.comments          = 0;          break;
			case 'w': opt.fragment          = 1;          break;
			case 'X': opt.always_hex        = 1;          break;
			case 'e': opt.prefix            = optarg;     break;
			case 'E': opt.package_prefix	= optarg;     break;
			case 'k': opt.io                = io(optarg); break;

			case 'b': flags |= RE_ANCHORED; break;
			case 'i': flags |= RE_ICASE;    break;

			case 's':
				join = fsm_concat;
				break;

			case 'l':
				lang_name(optarg, &fsm_lang, &ast_lang);
				break;

			case 'F':
				parse_flags(optarg, &flags);
				break;

			case 'p': fsm_lang  = FSM_PRINT_FSM;        break;
			case 'q': query     = comparison(optarg);   break;
			case 'r': dialect   = dialect_name(optarg); break;
			case 'u': opt.ambig = AMBIG_MULTIPLE;       break;

			case 'f': fsmfiles  = 1; break;
			case 'x': xfiles    = 1; break;
			case 'y': yfiles    = 1; break;
			case 'm': example   = 1; break;
			case 'n': keep_nfa  = 1; break;
			case 't': isliteral = 1; break;
			case 'z': patterns  = 1; break;
			case 'M': makevm    = 1; break;

			case 'G':
				generate_bounds = strtoul(optarg, NULL, 10);
				if (generate_bounds == 0) {
					usage();
					exit(EXIT_FAILURE);
				}
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

	if ((fsm_lang != FSM_PRINT_NONE) + (ast_lang != AST_PRINT_NONE) + example + isliteral + !!query > 1) {
		fprintf(stderr, "-m, -p, -q and -t are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	if ((fsm_lang != FSM_PRINT_NONE) + (ast_lang != AST_PRINT_NONE) + example + isliteral + !!query && xfiles) {
		fprintf(stderr, "-x applies only when executing\n");
		return EXIT_FAILURE;
	}

	if (makevm && (keep_nfa || example || isliteral || query)) {
		fprintf(stderr, "-M cannot be used with -m, -n, -q, or -t\n");
		return EXIT_FAILURE;
	}

	if (patterns && !!query) {
		fprintf(stderr, "-z does not apply for querying\n");
		return EXIT_FAILURE;
	}

	if (generate_bounds > 0 && (keep_nfa || example || isliteral || query)) {
		fprintf(stderr, "-G cannot be used with -m, -n, -q, or -t\n");
		return EXIT_FAILURE;
	}

	if (join == fsm_concat && patterns) {
		fprintf(stderr, "-s is not implemented when printing patterns by -z\n");
		return EXIT_FAILURE;
	}

	if (fsm_lang == FSM_PRINT_NONE) {
		keep_nfa = 0;
	}

	if (keep_nfa) {
		opt.ambig = AMBIG_MULTIPLE;
	}

	/* XXX: repetitive */
	if (isliteral) {
		enum re_literal_category literal_category;
		struct re_err err;
		char *literal_s;
		size_t literal_n;
		int literal_r;
		size_t i;

		if (argc != 1) {
			fprintf(stderr, "single regexp only for -t\n");
			return EXIT_FAILURE;
		}

		if (fsmfiles) {
			fprintf(stderr, "-f cannot be used with -t\n");
			return EXIT_FAILURE;
		}

		if (yfiles) {
			FILE *f;

			f = xopen(argv[0]);

			literal_r =
			re_is_literal(dialect, fsm_fgetc, f,
				flags, &err,
				&literal_category, &literal_s, &literal_n);

			fclose(f);
		} else {
			const char *s;

			s = argv[0];

			literal_r =
			re_is_literal(dialect, fsm_sgetc, &s,
				flags, &err,
				&literal_category, &literal_s, &literal_n);
		}

		if (literal_r == -1) {
			re_perror(dialect, &err,
				 yfiles ? argv[0] : NULL,
				!yfiles ? argv[0] : NULL);

			if (err.e == RE_EUNSUPPORTED) {
				return 2;
			}

			return EXIT_FAILURE;
		}

		if (literal_r == 0) {
			return EXIT_FAILURE;
		}

		assert(literal_r == 1);

		if (literal_category == RE_LITERAL_UNSATISFIABLE) {
			printf("unsatisfiable\n");
			return EXIT_SUCCESS;
		}

		printf("anchors: ");
		if (literal_category & RE_LITERAL_ANCHOR_START) {
			printf("^");
		}
		if (literal_category & RE_LITERAL_ANCHOR_END) {
			printf("$");
		}
		printf("\n");

		printf("literal: ");
		for (i = 0; i < literal_n; i++) {
			(void) c_escputc_str(stdout, &opt, literal_s[i]);
		}
		printf("\n");

		free(literal_s);

		return !literal_r;
	}

	/* XXX: repetitive */
	if (ast_lang != AST_PRINT_NONE) {
		struct ast *ast;
		struct re_err err;

		if (argc != 1) {
			fprintf(stderr, "single regexp only for this output format\n");
			return EXIT_FAILURE;
		}

		if (fsmfiles || yfiles) {
			FILE *f;

			f = xopen(argv[0]);

			if (!fsmfiles) {
				ast = re_parse(dialect, fsm_fgetc, f, flags, &err, NULL);
			} else {
				struct fsm *fsm;

				fsm = fsm_parse(f, alloc);
				if (fsm == NULL) {
					perror("fsm_parse");
					return EXIT_FAILURE;
				}

				ast = ast_new_from_fsm(fsm);
				if (ast == NULL) {
					perror("ast_new_from_fsm");
					fsm_free(fsm);
					return EXIT_FAILURE;
				}

				fsm_free(fsm);
			}

			fclose(f);
		} else {
			const char *s;

			s = argv[0];

			ast = re_parse(dialect, fsm_sgetc, &s, flags, &err, NULL);
		}

		if (ast == NULL) {
			re_perror(dialect, &err,
				 yfiles ? argv[0] : NULL,
				!yfiles ? argv[0] : NULL);

			if (err.e == RE_EUNSUPPORTED) {
				return 2;
			}

			return EXIT_FAILURE;
		}

		ast_print(stdout, ast, &opt, flags, ast_lang);

		ast_free(ast);

		return 0;
	}

	flags |= RE_MULTI;

	fsm = fsm_new(alloc);
	if (fsm == NULL) {
		perror("fsm_new");
		return EXIT_FAILURE;
	}

	fsm_to_cleanup = fsm;

	{
		int i;

		for (i = 0; i < argc - !(fsm_lang != FSM_PRINT_NONE || example || isliteral || !!query || argc <= 1); i++) {
			struct re_err err;
			struct fsm *new, *q;

			/* TODO: handle possible "dialect:" prefix */

			if (0 == strcmp(argv[i], "--")) {
				argc--;
				argv++;

				break;
			}

			if (fsmfiles || yfiles) {
				FILE *f;

				f = xopen(argv[i]);

				if (!fsmfiles) {
					new = re_comp(dialect, fsm_fgetc, f, alloc, flags, &err);
				} else {
					new = fsm_parse(f, alloc);
					if (new == NULL) {
						perror("fsm_parse");
						return EXIT_FAILURE;
					}
				}

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				new = re_comp(dialect, fsm_sgetc, &s, alloc, flags, &err);
			}

			if (new == NULL) {
				re_perror(dialect, &err,
					 yfiles ? argv[i] : NULL,
					!yfiles ? argv[i] : NULL);

				if (err.e == RE_EUNSUPPORTED) {
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

			/*
			 * Attach this mapping to each end state for this regexp.
			 * We pick this up from matchv[] indexed by the end id.
			 */
			if (!fsm_setendid(new, i)) {
				perror("fsm_setendid");
				return EXIT_FAILURE;
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

			fsm = join(fsm, new, NULL);
			if (fsm == NULL) {
				perror("fsm_union/concat");
				return EXIT_FAILURE;
			}

			fsm_to_cleanup = fsm;

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

		matchv = argv;
		matchc = i;

		argc -= i;
		argv += i;
	}

	if (query) {
		return EXIT_SUCCESS;
	}

	if ((fsm_lang != FSM_PRINT_NONE || example) && argc > 0) {
		fprintf(stderr, "too many arguments\n");
		return EXIT_FAILURE;
	}

	hooks.conflict = conflict;

	if ((opt.ambig | AMBIG_SINGLE)) {
		struct fsm *dfa;
		int r;

		dfa = fsm_clone(fsm);
		if (dfa == NULL) {
			perror("fsm_clone");
			return EXIT_FAILURE;
		}

		if (!fsm_determinise(dfa)) {
			perror("fsm_determinise");
			return EXIT_FAILURE;
		}

		/*
		 * This is silly, we're doing this just to find conflicts,
		 * because otherwise we won't find them during print,
		 * because in re(1) we don't always print.
		 *
		 * I don't like that we have two places where we call fsm_print().
		 */
		// XXX: stdout is a non-NULL placeholder, would prefer f=NULL because FSM_PRINT_NONE
		r = fsm_print(stdout, dfa, &opt, &hooks, FSM_PRINT_NONE);

		fsm_free(dfa);

		if (r < 0) {
			exit(EXIT_FAILURE);
		}

		if (r == 1) {
			exit(EXIT_FAILURE);
		}
	}

	if (!keep_nfa) {
		/*
		 * Convert to a DFA, then minimise unless we need to keep the end
		 * state information separated per regexp. */
// TODO: redundant wrt above
		if (!fsm_determinise(fsm)) {
			perror("fsm_determinise");
			return EXIT_FAILURE;
		}

		if (!patterns && example && fsm_lang != FSM_PRINT_C) {
			if (!fsm_minimise(fsm)) {
				perror("fsm_minimise");
				return EXIT_FAILURE;
			}
		}

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
			    fsm_end_id_t *ids;
			    size_t count, i;

			    count = fsm_endid_count(fsm, s);
			    if (count > 0) {
				int res;

				ids = xmalloc(count * sizeof *ids);

				res = fsm_endid_get(fsm, s, count, ids);
				assert(res == 1);

				for (i = 0; i < count; i++) {
				    /* TODO: print nicely */
				    printf("/%s/", matchv[ids[i]]);
				    if (i + 1 < count) {
					    printf(", ");
				    }
				}

				free(ids);
			    }

			    printf(": ");
			}

			printexample(stdout, fsm, s);
			printf("\n");
		}

/* XXX: free fsm */

		return 0;
	}

	if (generate_bounds > 0) {
		if (!fsm_generate_matches(fsm, generate_bounds, fsm_generate_cb_printf_escaped, &opt)) {
			exit(EXIT_FAILURE);
		}

		return 0;
	}

	if (fsm_lang != FSM_PRINT_NONE) {
		switch (fsm_lang) {
		case FSM_PRINT_NONE:
			break;

		case FSM_PRINT_C:
		case FSM_PRINT_VMC:
			hooks.comment = comment_c;
			break;

		case FSM_PRINT_RUST:
		case FSM_PRINT_GO: /* close enough */
			hooks.comment = comment_rust;
			break;

		case FSM_PRINT_LLVM:
			hooks.comment = comment_llvm;
			break;

		case FSM_PRINT_DOT:
		case FSM_PRINT_VMDOT:
			hooks.comment = patterns ? comment_dot : NULL;
			break;

		case FSM_PRINT_JSON:
			break;

		default:
			break;
		}

		if (-1 == fsm_print(stdout, fsm, &opt, &hooks, fsm_lang)) {
			if (errno == ENOTSUP) {
				fprintf(stderr, "unsupported IO API\n");
			} else {
				perror("fsm_print");
			}
			exit(EXIT_FAILURE);
		}

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
						e = fsm_exec(fsm, fsm_fgetc, f, &state, NULL);
					}

					fclose(f);
				} else {
					const char *s;

					s = argv[i];

					if (vm != NULL) {
						e = fsm_vm_match_buffer(vm, s, strlen(s));
					} else {
						e = fsm_exec(fsm, fsm_sgetc, &s, &state, NULL);
					}
				}

				if (e != 1) {
					r |= 1;
					continue;
				}

				if (patterns) {
					fsm_end_id_t ids[1];
					size_t count, i;
					int res;

					count = fsm_endid_count(fsm, state);
					assert(count == 1);

					res = fsm_endid_get(fsm, state, count, ids);
					assert(res == 1);

					for (i = 0; i < count; i++) {
						/* TODO: print nicely */
						printf("match: /%s/\n", matchv[ids[i]]);
					}
				}
			}
		}

		/* XXX: free opaques */

		fsm_free(fsm);
		fsm_to_cleanup = NULL;

		if (vm != NULL) {
			fsm_vm_free(vm);
		}

		return r;
	}
}
