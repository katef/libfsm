/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/out.h>	/* XXX */
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/exec.h>
#include <fsm/graph.h>

#include <re/re.h>
#include <re/group.h>

#include "libfsm/internal.h" /* XXX */

/*
 * TODO: pass a language option for dumping output.
 * output to various forms, too (print the fsm as ERE, glob, etc)
 * do this through a proper interface, instead of including internal.h
 *
 * TODO: accepting a delimiter would be useful: /abc/. perhaps provide that as
 * a convenience function, especially wrt. escaping for lexing. Also convenient
 * for specifying flags: /abc/g
 * TODO: flags; -i for RE_ICASE, -r for RE_REVERSE, etc
 */

struct match {
	const char *s;
	struct match *next;
};

static void
usage(void)
{
	fprintf(stderr, "usage: re -d [-acidmn] <re> ...\n");
	fprintf(stderr, "       re -m [-acidmn] <re> ...\n");
	fprintf(stderr, "       re -x [-acidmn] <re> ... [ <file> | -- <file> ... ]\n");
	fprintf(stderr, "       re    [-acidmn] <re> ... [ <string> | -- <string> ... ]\n");
	fprintf(stderr, "       re -h\n");
}

static enum re_form
form_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum re_form form;
	} a[] = {
/* TODO:
		{ "ere",     RE_ERE     },
		{ "bre",     RE_BRE     },
		{ "plan9",   RE_PLAN9   },
		{ "pcre",    RE_PCRE    },
*/
		{ "literal", RE_LITERAL },
		{ "glob",    RE_GLOB    },
		{ "simple",  RE_SIMPLE  }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].form;
		}
	}

	fprintf(stderr, "unrecognised re form \"%s\"", name);
	exit(EXIT_FAILURE);
}

/* TODO: centralise */
static int
escputc(int c, FILE *f)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '\\', "\\\\" },
		{ '\'', "\\\'" },

		{ '\a', "\\a"  },
		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  },
		{ '\v', "\\v"  }
	};

	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\%03o", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
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
carryopaque(struct state_set *set, struct fsm *fsm, struct fsm_state *state)
{
	struct state_set *s;
	struct match *m;
	struct match *matches;

	assert(set != NULL); /* TODO: right? */
	assert(fsm != NULL);
	assert(state != NULL);

	if (!fsm_isend(fsm, state)) {
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

	for (s = set; s != NULL; s = s->next) {
		if (!fsm_isend(fsm, s->state)) {
			continue;
		}

		assert(s->state->opaque != NULL);

		for (m = s->state->opaque; m != NULL; m = m->next) {
			assert(s->state->opaque != NULL);

			if (!addmatch(&matches, m->s)) {
				perror("addmatch");
				goto error;
			}
		}
	}

	state->opaque = matches;

	return;

error:

	/* XXX: free matches */

	state->opaque = NULL;

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
	enum re_form form;
	struct fsm *fsm;
	int ifiles, xfiles;
	int dump;
	int group;
	int example;
	int keep_nfa;
	int patterns;
	int ambig;
	int r;

	if (argc < 1) {
		usage();
		return EXIT_FAILURE;
	}

	fsm = re_new_empty();
	if (fsm == NULL) {
		perror("re_new_empty");
		return EXIT_FAILURE;
	}

	ifiles   = 0;
	xfiles   = 0;
	dump     = 0;
	group    = 0;
	example  = 0;
	keep_nfa = 0;
	patterns = 0;
	ambig    = 0;
	join     = fsm_union;
	form     = RE_SIMPLE;

	{
		int c;

		while (c = getopt(argc, argv, "hacdgixmnr:p"), c != -1) {
			switch (c) {
			case 'h':
				usage();
				return EXIT_SUCCESS;

			case 'c':
				join = fsm_concat;
				break;

			case 'r':
				form = form_name(optarg);
				break;

			case 'a': ambig    = 1; break;
			case 'd': dump     = 1; break;
			case 'g': group    = 1; break;
			case 'i': ifiles   = 1; break;
			case 'x': xfiles   = 1; break;
			case 'm': example  = 1; break;
			case 'n': keep_nfa = 1; break;
			case 'p': patterns = 1; break;

			case '?':
			default:
				usage();
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (dump + example + group > 1) {
		fprintf(stderr, "-d, -g and -m are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	if (group) {
		struct re_err err;
		int r;

		if (argc != 1) {
			fprintf(stderr, "expected one group input\n");
			return EXIT_FAILURE;
		}

		/* TODO: pick escputc based on *output* dialect.
		 * centralise with escchar() in parser.act? */
		/* TODO: re_flags */

		if (ifiles) {
			FILE *f;

			f = xopen(argv[0]);

			r = re_group_print(form, re_fgetc, f,
				0, &err, stdout, escputc);

			fclose(f);
		} else {
			const char *s;

			s = argv[0];

			r = re_group_print(form, re_sgetc, &s,
				0, &err, stdout, escputc);
		}

		if (r == -1) {
			re_perror("re_group_print", RE_SIMPLE, &err,
				 ifiles ? argv[0] : NULL,
				!ifiles ? argv[0] : NULL);
			return EXIT_FAILURE;
		}

		printf("\n");

		return 0;
	}

	if (!dump) {
		keep_nfa = 0;
	}

	if (keep_nfa) {
		ambig = 1;
	}

	{
		int i;

		for (i = 0; i < argc - !(dump || example); i++) {
			struct re_err err;
			struct fsm *new;

			/* TODO: handle possible "form:" prefix */

			if (0 == strcmp(argv[i], "--")) {
				argc--;
				argv++;

				break;
			}

			if (ifiles) {
				FILE *f;

				f = xopen(argv[i]);

				new = re_new_comp(form, re_fgetc, f, 0, &err);

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				new = re_new_comp(form, re_sgetc, &s, 0, &err);
			}

			if (new == NULL) {
				re_perror("re_new_comp", form, &err,
					 ifiles ? argv[i] : NULL,
					!ifiles ? argv[i] : NULL);
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
				 * We could share the same matches struct for all end states
				 * in the same regexp, but that would be tricky to free().
				 */
				for (s = new->sl; s != NULL; s = s->next) {
					if (fsm_isend(new, s)) {
						struct match *matches;

						matches = NULL;

						if (!addmatch(&matches, argv[i])) {
							perror("addmatch");
							return EXIT_FAILURE;
						}

						assert(s->opaque == NULL);

						s->opaque = matches;
					}
				}
			}

			fsm = join(fsm, new);
			if (fsm == NULL) {
				perror("fsm_union/concat");
				return EXIT_FAILURE;
			}
		}

		argc -= i;
		argv += i;
	}

	if ((dump || example) && argc > 0) {
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

		if (!fsm_determinise_opaque(dfa, carryopaque)) {
			perror("fsm_determinise_opaque");
			return EXIT_FAILURE;
		}

		for (s = dfa->sl; s != NULL; s = s->next) {
			const struct match *matches;

			if (!fsm_isend(dfa, s)) {
				continue;
			}

			assert(s->opaque != NULL);

			matches = s->opaque;

			if (matches->next != NULL) {
				const struct match *m;

				/* TODO: // delimeters depend on form */
				/* TODO: would deal with form: prefix here, too */

				fprintf(stderr, "ambigious matches for ");
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
			if (!fsm_determinise_opaque(fsm, carryopaque)) {
				perror("fsm_determinise_opaque");
				return EXIT_FAILURE;
			}
		}
	}

	if (example) {
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!fsm_isend(fsm, s)) {
				continue;
			}

			/* TODO: would deal with form: prefix here, too */
			if (patterns) {
				const struct match *m;

				assert(s->opaque != NULL);

				for (m = s->opaque; m != NULL; m = m->next) {
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

		return 0;
	}

	if (dump) {
		/* TODO: print examples in comments for end states;
		 * patterns in comments for the whole FSM */

		fsm_print(fsm, stdout, FSM_OUT_FSM, NULL);

		return 0;
	}

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

				for (m = state->opaque; m != NULL; m = m->next) {
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

