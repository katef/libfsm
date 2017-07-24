/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/out.h>
#include <fsm/options.h>

#include <re/re.h>

#include "libfsm/internal.h" /* XXX */

/*
 * TODO: accepting a delimiter would be useful: /abc/. perhaps provide that as
 * a convenience function, especially wrt. escaping for lexing. Also convenient
 * for specifying flags: /abc/g
 * TODO: flags; -r for RE_REVERSE, etc
 */

struct match {
	const char *s;
	struct match *next;
};

static struct fsm_options opt;

static void
usage(void)
{
	fprintf(stderr, "usage: re    [-r <dialect>] [-niusyz] [-x] <re> ... [ <text> | -- <text> ... ]\n");
	fprintf(stderr, "       re    [-r <dialect>] [-niusyz] {-q <query>} <re> ...\n");
	fprintf(stderr, "       re -p [-r <dialect>] [-niusyz] [-l <language>] [-awc] [-k <io>] [-e <prefix>] <re> ...\n");
	fprintf(stderr, "       re -m [-r <dialect>] [-niusyz] <re> ...\n");
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

static enum fsm_out
language(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum fsm_out format;
	} a[] = {
		{ "api",  FSM_OUT_API  },
		{ "c",    FSM_OUT_C    },
		{ "csv",  FSM_OUT_CSV  },
		{ "dot",  FSM_OUT_DOT  },
		{ "fsm",  FSM_OUT_FSM  },
		{ "json", FSM_OUT_JSON }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].format;
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
		{ "pcre",    RE_PCRE    },
		{ "js",      RE_JS      },
		{ "python",  RE_PYTHON  },
		{ "sql99",   RE_SQL99   },
*/
		{ "like",    RE_LIKE    },
		{ "literal", RE_LITERAL },
		{ "glob",    RE_GLOB    },
		{ "native",  RE_NATIVE  },
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
addmatch(struct match **head, const char *s)
{
	struct match *new;

	assert(head != NULL);
	assert(s != NULL);

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

	new->s = s;

	new->next = *head;
	*head     = new;

	return new;
}

static void
carryopaque(const struct fsm_state **set, size_t n,
	struct fsm *fsm, struct fsm_state *state)
{
	struct match *matches;
	struct match *m;
	size_t i;

	assert(set != NULL); /* TODO: right? */
	assert(n > 0); /* TODO: right? */
	assert(fsm != NULL);
	assert(state != NULL);

	if (!fsm_isend(NULL, fsm, state)) {
		return;
	}

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
		if (!fsm_isend(NULL, fsm, set[i])) {
			continue;
		}

		assert(fsm_getopaque(fsm, set[i]) != NULL);

		for (m = fsm_getopaque(fsm, set[i]); m != NULL; m = m->next) {
			if (!addmatch(&matches, m->s)) {
				perror("addmatch");
				goto error;
			}
		}
	}

	fsm_setopaque(fsm, state, matches);

	return;

error:

	/* XXX: free matches */

	fsm_setopaque(fsm, state, NULL);

	return;
}

static void
printexample(FILE *f, const struct fsm *fsm, const struct fsm_state *state)
{
	char buf[256]; /* TODO */
	int n;

	assert(f != NULL);
	assert(fsm != NULL);
	assert(state != NULL);

	n = fsm_example(fsm, state, buf, sizeof buf);
	if (-1 == n) {
		perror("fsm_example");
		exit(EXIT_FAILURE);
	}

	/* TODO: escape hex etc */
	fprintf(f, "%s%s", buf,
		n >= (int) sizeof buf - 1 ? "..." : "");
}

int
main(int argc, char *argv[])
{
	struct fsm *(*join)(struct fsm *, struct fsm *);
	int (*query)(const struct fsm *, const struct fsm *);
	enum fsm_out format;
	enum re_dialect dialect;
	struct fsm *fsm;
	enum re_flags flags;
	int xfiles, yfiles;
	int print;
	int example;
	int keep_nfa;
	int patterns;
	int ambig;

	/* note these defaults are the opposite than for fsm(1) */
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	opt.comments          = 1;
	opt.io                = FSM_IO_GETC;

	flags    = 0U;
	xfiles   = 0;
	yfiles   = 0;
	print    = 0;
	example  = 0;
	keep_nfa = 0;
	patterns = 0;
	ambig    = 0;
	query    = NULL;
	join     = fsm_union;
	format   = FSM_OUT_FSM;
	dialect  = RE_NATIVE;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acwe:k:" "i" "sq:r:l:" "upmnxyz"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 0;          break;
			case 'c': opt.consolidate_edges = 0;          break;
			case 'w': opt.fragment          = 1;          break;
			case 'e': opt.prefix            = optarg;     break;
			case 'k': opt.io                = io(optarg); break;

			case 'i': flags |= RE_ICASE; break;

			case 's':
				join = fsm_concat;
				break;

			case 'q': query   = comparison(optarg);       break;
			case 'r': dialect = dialect_name(optarg);     break;
			case 'l': format  = language(optarg);         break;

			case 'u': ambig    = 1; break;
			case 'p': print    = 1; break;
			case 'x': xfiles   = 1; break;
			case 'y': yfiles   = 1; break;
			case 'm': example  = 1; break;
			case 'n': keep_nfa = 1; break;
			case 'z': patterns = 1; break;

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

	if (print + example + !!query && xfiles) {
		fprintf(stderr, "-x applies only when executing\n");
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

	if (!print) {
		keep_nfa = 0;
	}

	if (keep_nfa) {
		ambig = 1;
	}

	flags |= RE_MULTI;

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		perror("fsm_new");
		return EXIT_FAILURE;
	}

	{
		int i;

		for (i = 0; i < argc - !(print || example || !!query || argc <= 1); i++) {
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

				new = re_comp(dialect, re_fgetc, f, &opt, flags, &err);

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				new = re_comp(dialect, re_sgetc, &s, &opt, flags, &err);
			}

			if (new == NULL) {
				re_perror(dialect, &err,
					 yfiles ? argv[i] : NULL,
					!yfiles ? argv[i] : NULL);
				return EXIT_FAILURE;
			}

			if (!keep_nfa) {
				if (!fsm_minimise(new)) {
					perror("fsm_minimise");
					return EXIT_FAILURE;
				}
			}

			/* TODO: associate argv[i] with new's end state */
			{
				struct fsm_state *s;

				/*
				 * Attach this mapping to each end state for this regexp.
				 * XXX: we should share the same matches struct for all end states
				 * in the same regexp, and keep an argc-sized array of pointers to free().
				 * XXX: then use fsm_setendopaque() here.
				 */
				for (s = new->sl; s != NULL; s = s->next) {
					if (fsm_isend(NULL, new, s)) {
						struct match *matches;

						matches = NULL;

						if (!addmatch(&matches, argv[i])) {
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

	if ((print || example) && argc > 0) {
		fprintf(stderr, "too many arguments\n");
		return EXIT_FAILURE;
	}

	if (!ambig) {
		const struct fsm_state *s;
		struct fsm *dfa;

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

		for (s = dfa->sl; s != NULL; s = s->next) {
			const struct match *matches;

			if (!fsm_isend(NULL, dfa, s)) {
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
		/*
		 * Minimise only when we don't need to keep the end state information
		 * separated per regexp. Otherwise, convert to a DFA.
		 */
		if (!patterns && !example) {
			if (!fsm_minimise(fsm)) {
				perror("fsm_minimise");
				return EXIT_FAILURE;
			}
		} else {
			opt.carryopaque = carryopaque;

			if (!fsm_determinise(fsm)) {
				perror("fsm_determinise");
				return EXIT_FAILURE;
			}

			opt.carryopaque = NULL;
		}
	}

	if (example) {
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!fsm_isend(NULL, fsm, s)) {
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

	if (print) {
		/* TODO: print examples in comments for end states;
		 * patterns in comments for the whole FSM */

		fsm_print(fsm, stdout, format);

/* XXX: free fsm */

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
				const struct fsm_state *state;

				if (xfiles) {
					FILE *f;

					f = xopen(argv[0]);

					state = fsm_exec(fsm, fsm_fgetc, f);

					fclose(f);
				} else {
					const char *s;

					s = argv[i];

					state = fsm_exec(fsm, fsm_sgetc, &s);
				}

				if (state == NULL) {
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

		return r;
	}
}

