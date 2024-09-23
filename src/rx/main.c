/*
 * Copyright 2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200809L

#ifdef __linux__
#include <sys/resource.h>
#endif

#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <adt/xalloc.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#include <re/re.h>
#include <re/literal.h>
#include <re/strings.h>

/* placeholder for alloc hooks */
static const struct fsm_alloc *alloc;

enum category {
	CATEGORY_EMPTY,
	CATEGORY_LITERAL,
	CATEGORY_GENERAL,
	CATEGORY_ERROR,
	CATEGORY_UNSUPPORTED,
	CATEGORY_UNSATISFIABLE,
	CATEGORY_NOT_PERMITTED
};

struct pattern {
	fsm_end_id_t id; /* either per file or per pattern */
	enum re_dialect dialect;
	const char *s;
};

struct literal {
	fsm_end_id_t id; /* either per file or per literal */
	const void *p;
	size_t n;
};

struct literal_set {
	size_t count;
	struct literal *a;
};

struct pattern_set {
	size_t count;
	struct pattern *a;
};

struct check_env {
	struct literal_set (*literals)[4];
	const struct pattern_set *general;
};

// TODO: centralise, maybe adt/alloc.h
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

// TODO: centralise
static char *
xgetline(FILE *f)
{
	char *buf;
	size_t len;
	ssize_t n;

	buf = NULL;
	len = 0;

	n = getline(&buf, &len, f);
	if (n == -1) {
		free(buf);

		if (feof(f)) {
			return NULL;
		}

		perror("getline");
		exit(EXIT_FAILURE);
	}

	/* n == 0 for an empty line */
	if (n > 0) {
		assert(buf[n - 1] == '\n');
		buf[n - 1] = '\0';
	}

	return buf;
}

static void
append_pattern(struct pattern_set *set,
	fsm_end_id_t id, enum re_dialect dialect, const char *s)
{
	const size_t low    = 16; /* must be power of 2 */
	const size_t factor =  2; /* must be even */

	assert(set != NULL);
	assert(s != NULL);

	if (set->count == 0) {
		set->a = xmalloc(low * sizeof *set->a);
	} else if (set->count >= low && (set->count & (set->count - 1)) == 0) {
		size_t new = set->count * factor;
		if (new < set->count) {
			errno = E2BIG;
			perror("realloc");
			exit(EXIT_FAILURE);
		}

		set->a = xrealloc(set->a, new * sizeof *set->a);
	}

	set->a[set->count].id = id;
	set->a[set->count].dialect = dialect;
	set->a[set->count].s = s;
	set->count++;
}

static void
append_literal(struct literal_set *set,
	const char *p, size_t n, fsm_end_id_t id)
{
	const size_t low    = 16; /* must be power of 2 */
	const size_t factor =  2; /* must be even */

	assert(set != NULL);
	assert(p != NULL);

	if (set->count == 0) {
		set->a = xmalloc(low * sizeof *set->a);
	} else if (set->count >= low && (set->count & (set->count - 1)) == 0) {
		size_t new = set->count * factor;
		if (new < set->count) {
			errno = E2BIG;
			perror("realloc");
			exit(EXIT_FAILURE);
		}

		set->a = xrealloc(set->a, new * sizeof *set->a);
	}

	set->a[set->count].id = id;
	set->a[set->count].p  = p;
	set->a[set->count].n  = n;
	set->count++;
}

static enum re_strings_flags
literal_flags(enum re_literal_category category)
{
	switch (category) {
	case RE_LITERAL_UNANCHORED:
		return 0;

	case RE_LITERAL_ANCHOR_START:
		return RE_STRINGS_ANCHOR_LEFT;

	case RE_LITERAL_ANCHOR_END:
		return RE_STRINGS_ANCHOR_RIGHT;

	case RE_LITERAL_ANCHOR_BOTH:
		return RE_STRINGS_ANCHOR_LEFT | RE_STRINGS_ANCHOR_RIGHT;

	default:
		assert(!"unreached");
		abort();
	}
}

/* this interface doesn't allow us to have \0 in the character set */
static struct fsm *
intersect_charset(bool show_stats, const char *charset, struct fsm *fsm)
{
	struct fsm *q;

	assert(charset != NULL);
	assert(fsm_all(fsm, fsm_isdfa));

	size_t pre = fsm_countstates(fsm);
	if (show_stats) {
		fprintf(stderr, "pre-intersection dfa: %zu states\n", pre);
	}

	q = fsm_intersect_charset(fsm, strlen(charset), charset);
	if (q == NULL) {
		perror("fsm_intersect_charset");
		exit(EXIT_FAILURE);
	}

	/* walking two DFA produces a DFA */
	assert(fsm_all(q, fsm_isdfa));

	size_t post = fsm_countstates(q);
	if (show_stats) {
		fprintf(stderr, "post-intersection dfa: %zu states\n", post);
	}

	/* intersecting with the charset FSM should never introduce new states */
	assert(post <= pre);

	return q;
}

const char *
category_reason(enum category r)
{
	switch (r) {
	case CATEGORY_EMPTY:         return "empty string";
	case CATEGORY_LITERAL:       return "literal";
	case CATEGORY_GENERAL:       return "general";
	case CATEGORY_ERROR:         return "parse error";
	case CATEGORY_UNSUPPORTED:   return "unsupported";
	case CATEGORY_UNSATISFIABLE: return "unsatisfiable";
	case CATEGORY_NOT_PERMITTED: return "charset/reject";

	default:
		assert(!"unreached");
		abort();
	}
}

/* return an error reason if a pattern was not successfully appended to a set */
static enum category
categorize(enum re_dialect dialect, enum re_flags flags,
	const char *charset, const char *reject, const char *s,
	struct re_err *err,
	const char **lit_s_out, size_t *lit_n_out, enum re_strings_flags *strings_flags)
{
	enum re_literal_category category;
	char *lit_s;
	size_t lit_n;
	int r;

	assert(err != NULL);
	assert(lit_s_out != NULL);
	assert(lit_n_out != NULL);

	/*
	 * As an optimisation, we skip libre's parsing for RE_LITERAL and
	 * categorize empirically based on the dialect. We can't do this
	 * inside re_is_literal() because that uses the getc callback,
	 * and this shortcut depends on the storage for the input string s.
	 */
	if (dialect == RE_LITERAL) {
		*lit_s_out = s;
		*lit_n_out = strlen(s);
		*strings_flags = RE_STRINGS_ANCHOR_LEFT | RE_STRINGS_ANCHOR_RIGHT;
		return CATEGORY_LITERAL;
	}

	/*
	 *  -1: Error
	 *   1: Literal, *s may or may not be NULL (i.e. for an empty string), *n >= 0
	 *      and *category is set.
	 */
	const char *q = s;
	r = re_is_literal(dialect, fsm_sgetc, &q, flags, err, &category,
		&lit_s, &lit_n);

	/* unsupported */
	if (r == -1 && err->e == RE_EUNSUPPORTED) {
		return CATEGORY_UNSUPPORTED;
	}

	/* parse error */
	if (r == -1) {
		return CATEGORY_ERROR;
	}

	/* empty string */
	if (r == 1 && lit_n == 0) {
		assert(lit_s == NULL);
		return CATEGORY_EMPTY;
	}

	/* unsatisfiable */
	if (r == 1 && category == RE_LITERAL_UNSATISFIABLE) {
		assert(lit_s == NULL);
		return CATEGORY_UNSATISFIABLE;
	}

	/* is a literal */
	if (r == 1) {
		assert(lit_s != NULL);

		if (category == RE_LITERAL_ANCHOR_END || category == RE_LITERAL_ANCHOR_BOTH) {
			/*
			 * This newline is appended in ast_is_literal() as an attempt to
			 * account for the PCRE2_DOTALL behaviour. I'm not interested in
			 * newlines for this program (optional or legitimate),
			 * so I'm just cutting it off.
			 */
// TODO: maybe only if it's not in the character set?
			if (lit_n > 0 && lit_s[lit_n - 1] == '\n') {
				lit_n--;
			}
		}

		/*
		 * The reject set does not apply for literals,
		 * it's intended only for regex operators.
		 *
		 * We're walking lit_s longhand here (rather than strcspn)
		 * because it isn't a \0-terminated string.
		 */
		if (charset != NULL) {
			for (size_t i = 0; i < lit_n; i++) {
				if (!strchr(charset, lit_s[i])) {
					free(lit_s);
					return CATEGORY_NOT_PERMITTED;
				}
			}
		}

		*lit_s_out = lit_s;
		*lit_n_out = lit_n;
		*strings_flags = literal_flags(category);

		return CATEGORY_LITERAL;
	}

	/*
	 * XXX: The charset accidentally includes regex operators,
	 * because we're cheesily looking at the string pre-parse,
	 * we need those in the charset too, in addition to the
	 * charset matching literals within the regex.
	 *
	 * So we shouldn't enforce positive charset pre-lexing here,
	 * this would need to be done at AST time. Until this is done,
	 * I'm just skipping the reject set.
	 */
	(void) reject;
#if 0
	/*
	 * Reject regexps that contain characters that may be expensive.
	 *
	 * If the regex contains a character that's not in the charset,
	 * we can also save some compilation time and decline it up front.
	 *
	 * This isn't perfect, for example consider hex encoded values.
	 * My design assumption here is that it's okay to pre-filter strings
	 * down to a character set. This means /./ doesn't match any character,
	 * it means any character within the charset.
	 *
	 * So we're relying on that here, and this is just a best-effort help.
	 */
	if (reject != NULL && s[strcspn(s, reject)]) {
		return CATEGORY_NOT_PERMITTED;
	}
#endif

	/* not a literal */
	return CATEGORY_GENERAL;
}

/*
 * This is re_strings() but for an array of struct literal
 */
static struct fsm *
literal_strings(const struct literal *a, size_t n, enum re_strings_flags flags)
{
	struct re_strings *g;
	struct fsm *fsm;

	assert(a != NULL || n == 0);

	g = re_strings_new();
	if (g == NULL) {
		return NULL;
		perror("re_strings_new");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < n; i++) {
		if (!re_strings_add_raw(g, a[i].p, a[i].n, & (fsm_end_id_t) { a[i].id })) {
			perror("re_strings_add_raw");
			exit(EXIT_FAILURE);
		}
	}

	fsm = re_strings_build(g, alloc, flags);
	re_strings_free(g);

	/* this is already a DFA, courtesy of re_strings() */
	assert(fsm_all(fsm, fsm_isdfa));

	return fsm;
}

static struct fsm *
build_literals_fsm(bool show_stats,
	const char *charset,
	struct literal_set *set, enum re_strings_flags flags)
{
	struct fsm *fsm;

	assert(set != NULL);

	fsm = literal_strings(set->a, set->count, flags);

	/* we do produce an empty fsm for 0 literals */
	if (set->count == 0) {
		assert(fsm_empty(fsm));
	} else {
		assert(!fsm_empty(fsm));
	}

	if (charset != NULL) {
		fsm = intersect_charset(show_stats, charset, fsm);
		if (fsm == NULL) {
			perror("intersect_charset");
			exit(EXIT_FAILURE);
		}

		if (fsm_countstates(fsm) == 0) {
			fprintf(stderr, "literals produced empty intersection\n");
			exit(EXIT_FAILURE);
		}
	}

	/* We don't minimise here because this fsm has multiple endids,
	 * and the resulting FSM would be very similar to the current DFA */

#ifndef NDEBUG
	/*
	 * We could test to see that the fsm isn't any different.
	 * For general regexps, lots will be different because of
	 * mapping down /./ edges to the character set.
	 *
	 * But literal strings should be no different after the mapping,
	 * except for the implicit .* at either end for anchoring.
	 *
	 * So in order to assert here, we'd depend on constructing
	 * those .* long-hand at union time.
	 */
#endif

	return fsm;
}

static struct fsm *
build_pattern_fsm(bool show_stats,
	const char *charset,
	enum re_dialect dialect, bool strict,
	const char *s, enum re_flags flags, fsm_end_id_t id)
{
	struct fsm *fsm;
	struct re_err err;

	assert(s != NULL);

// TODO: want to compile these as if anchored, then do the unanchored parts during unioning
// then we can also construct the unanchored parts from the charset

	const char *q = s;
// TODO: RE_SINGLE here? i think it shouldn't have any effect when \n isn't present in the charset
	fsm = re_comp(dialect, fsm_sgetc, &q, alloc, flags, &err);

	if (strict && fsm == NULL) {
		re_perror(dialect, &err, NULL, s);
		exit(EXIT_FAILURE);
	}

	if (fsm == NULL && err.e == RE_EUNSUPPORTED) {
		return NULL;
	}

	/*
	 * An unsatisfiable regex compiles to an FSM that matches nothing,
	 * so that unioning it with other regexes will still work.
	 */
	if (strict && fsm_empty(fsm)) {
		fprintf(stdout, "re_comp: /%s/ regex is unsatisfiable\n", s);
		exit(EXIT_FAILURE);
	}

	/*
	 * There's no need to minimise here, we only need a DFA in order
	 * to fsm_intersect_charset(). We'd want to minimise again anyway.
	 */
	if (!fsm_determinise(fsm)) {
		perror("fsm_determinise");
		exit(EXIT_FAILURE);
	}

	assert(!fsm_empty(fsm));

	if (charset != NULL) {
// XXX: this is the wrong operation!
// XXX: i *think* intersect_charset must be doing the wrong thing. somehow
		(void) charset;
		(void) show_stats;
/*
		fsm = intersect_charset(show_stats, charset, fsm);
		if (fsm == NULL) {
			perror("intersect_charset");
			exit(EXIT_FAILURE);
		}
*/
	}

	/*
	 * This FSM has only one endid. fsm_minimise() does now maintain
	 * uniqueness for endids, but here we get the same FSM as if they
	 * didn't exist at all.
	 */
	if (!fsm_minimise(fsm)) {
		perror("fsm_minimise");
		exit(EXIT_FAILURE);
	}

	assert(!fsm_empty(fsm));

	if (!fsm_setendid(fsm, id)) {
		perror("fsm_setendid");
		exit(EXIT_FAILURE);
	}

	return fsm;
}

static int
conflict(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	const char *example,
	void *hook_opaque)
{
	const struct check_env *env = hook_opaque;

	assert(env != NULL);
	assert(env->literals != NULL);
	assert(env->general != NULL);

	/* in rx all end states have an end id */
	assert(state_metadata->end_id_count > 0);

	/* ...and we only conflict when there's more than one */
	assert(state_metadata->end_id_count > 1);

	(void) f;
	(void) opt;

	fprintf(stderr, "error: ambiguous patterns");
	if (example != NULL) {
		fprintf(stderr, ", for example on input '%s':", example);
	}
	fprintf(stderr, "\n");

	for (fsm_end_id_t i = 0; i < state_metadata->end_id_count; i++) {
		for (size_t k = 0; k < sizeof *env->literals / sizeof **env->literals; k++) {
			const struct literal_set *literals = &(*env->literals)[k];
			for (size_t j = 0; j < literals->count; j++) {
				if (literals->a[j].id == state_metadata->end_ids[i]) {
					// TODO: escape, centralise with libre's re_perror()
					fprintf(stderr, "#%u: /%s%.*s%s/\n",
						state_metadata->end_ids[i],
						(k & RE_STRINGS_ANCHOR_LEFT) ? "^" : "",
						(int) literals->a[j].n, (const char *) literals->a[j].p,
						(k & RE_STRINGS_ANCHOR_RIGHT) ? "$" : "");
					goto next;
				}
			}
		}

		for (size_t j = 0; j < env->general->count; j++) {
			// TODO: delimiters per dialect, centralise with libre's re_perror()
			if (env->general->a[i].id == state_metadata->end_ids[i]) {
				fprintf(stderr, "#%u: /%s/\n",
					state_metadata->end_ids[i],
					env->general->a[i].s);
				goto next;
			}
		}

next: ;
	}

	return 0;
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

static enum fsm_print_lang
print_name(const char *name, enum fsm_ambig ambig)
{
	size_t i;
	bool failed_mask;

	struct {
		const char *name;
		enum fsm_print_lang lang;
		enum fsm_ambig mask;
	} a[] = {
		{ "c",          FSM_PRINT_VMC,        AMBIG_ANY   },
		{ "dot",        FSM_PRINT_DOT,        AMBIG_ANY   },
		{ "fsm",        FSM_PRINT_FSM,        AMBIG_ANY   },
		{ "rust",       FSM_PRINT_RUST,       AMBIG_ANY   },
		{ "go",         FSM_PRINT_GO,         AMBIG_ANY   },
		{ "ir",         FSM_PRINT_IR,         AMBIG_ANY   },
		{ "irjson",     FSM_PRINT_IRJSON,     AMBIG_ANY   },
		{ "llvm",       FSM_PRINT_LLVM,       AMBIG_ANY   },
		{ "vmdot",      FSM_PRINT_VMDOT,      AMBIG_ANY   },
		{ "vmops_c",    FSM_PRINT_VMOPS_C,    AMBIG_ANY   },
		{ "vmops_h",    FSM_PRINT_VMOPS_H,    AMBIG_ANY   },
		{ "vmops_main", FSM_PRINT_VMOPS_MAIN, AMBIG_ANY   },
		{ "amd64_att",  FSM_PRINT_AMD64_ATT,  AMBIG_ERROR },
		{ "amd64_nasm", FSM_PRINT_AMD64_NASM, AMBIG_ERROR },
		{ "amd64_go",   FSM_PRINT_AMD64_GO,   AMBIG_ERROR }
	};

	assert(name != NULL);

	failed_mask = false;

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 != strcmp(a[i].name, name)) {
			continue;
		}

		if ((a[i].mask & ambig)) {
			return a[i].lang;
		}

		failed_mask = true;
	}

	if (failed_mask) {
		fprintf(stderr, "unsupported ambig mode for output language\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "unrecognised output language; valid languages are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

/* TODO: centralize */
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
			exit(EXIT_FAILURE);
		}
	}
}

static void
usage(const char *name)
{

	if (name == NULL) {
		name = "";
	} else {
		const char *p = strrchr(name, '/');
		name = p != NULL ? p + 1 : name;
	}

	printf("usage: %s: [-aciQqstuvwXx] [-C charset] [-k io] [-l <language> ] [-r dialect] [-R reject] [-d declined-file] [-E <package_prefix>] [-e <prefix>] input-file...\n", name);
	printf("       %s -h\n", name);
}

int
main(int argc, char *argv[])
{
	bool quiet = false;
	bool strict = false;
	bool verbose = false;
	bool show_stats = false;
	bool override_dialect = false;
	bool unanchored_literals = false;
	const char *lang = "c";
	enum fsm_print_lang fsm_lang = FSM_PRINT_VMC;

	size_t general_limit = 0;

	const char *charset = NULL;
	const char *declined_file = NULL;

	enum re_dialect default_dialect = RE_PCRE;

	/*
	 * TODO: we need to provide a way to disable RE_END_NL
	 * TODO: RE_SINGLE. also be careful, flags disrupt re_is_literal()
	 */
	enum re_flags flags = 0; // TODO: notably not RE_END_NL

	/* regex syntax characters we decline at compile time
	 * because NFA->DFA is too costly */
	const char *reject = "";

	struct fsm_options opt = {
		.anonymous_states = true,
		.consolidate_edges = true,
		.comments = false,
		.case_ranges = true,
		.always_hex = false,
		.group_edges = true,
		.io = FSM_IO_PAIR,
		.ambig = AMBIG_ERROR
	};

// TODO: prepend comment with rx invocation to output
// TODO: cli option for memory limit to decline patterns (implement via allocation hooks)
// TODO: consider de-duplicating arrays. not sure there's any reason
// TODO: decide if i want dialect==RE_LITERAL unanchored or not, maybe a cli flag either way

	{
		const char *name = argv[0];
		int c;

		while (c = getopt(argc, argv, "h" "aC:cd:E:e:ik:F:l:n:r:R:stQquvwXx"), c != -1) {
			switch (c) {
			case 'a':
				opt.anonymous_states = false;
				break;

			case 'C':
				charset = optarg;
				break;

			case 'c':
				/*
				 * https://www.rfc-editor.org/rfc/rfc9110#name-tokens
				 *
				 *   tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*"
				 *         / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
				 *         / DIGIT / ALPHA
				 *         ; any VCHAR, except delimiters
				 *
				 * https://www.rfc-editor.org/rfc/rfc9110#appendix-A
				 *   field-name = token
				 *   token          = 1*tchar
				 *
				 * So a valid header field-name has at least one character.
				 * But we don't enforce that here.
				 */
// TODO: although we could, with the charset FSM as [a-z]+ rather than [a-z]*
				charset =
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"abcdefghijklmnopqrstuvwzyx"
					"0123456789"
					"!#$%&'*+-.^_`|~";
				break;

			case 'd':
				declined_file = optarg;
				break;

			case 'E':
				opt.package_prefix = optarg;
				break;

			case 'e':
				opt.prefix = optarg;
				break;

			case 'h':
				usage(name);
				exit(EXIT_SUCCESS);

			case 'i':
				strict = true;
				break;

			case 'k':
				opt.io = io(optarg);
				break;

			case 'l':
				lang = optarg;
				break;

			case 'F':
				parse_flags(optarg, &flags);
				break;

			case 'n':
				general_limit = atoi(optarg);
				break;

			case 'R':
				reject = optarg;

				/* XXX: see categorize() */
				fprintf(stderr, "reject charset not implemented\n");
				exit(EXIT_FAILURE);
				break;

			case 'r':
				default_dialect = dialect_name(optarg);
				break;

			case 's':
				/* override dialect by file extension */
				override_dialect = true;
				break;

			case 'Q':
				/* one-off stats on resource consumption */
				show_stats = true;
				break;

			case 'q':
				/* "quiet" applies to the generated code only */
				quiet = true;
				break;

			case 't':
				opt.ambig = AMBIG_EARLIEST;
				break;

			case 'u':
				opt.ambig = AMBIG_MULTIPLE;
				break;

			case 'v':
				/* "verbose" means showing information about every pattern and FSM,
				 * this is O(n) output to the length of the input file */
				verbose = true;
				break;

			case 'w':
				opt.fragment = true;
				break;

			case 'X':
				opt.always_hex = true;
				break;

			case 'x':
				unanchored_literals = true;
				break;

			case '?':
			default:
				usage(name);
				exit(EXIT_FAILURE);
			}
		}

		argc -= optind;
		argv += optind;

		if (argc == 0) {
			usage(name);
			exit(EXIT_FAILURE);
		}
	}

	fsm_lang = print_name(lang, opt.ambig);

	if (quiet) {
		fsm_lang = FSM_PRINT_NONE;
	}

	if (charset != NULL && strchr(charset, '\n')) {
		fprintf(stderr, "charset cannot contain \\n "
			"(because regexes are delimited by newlines)\n");
		exit(EXIT_FAILURE);
	}

	if (show_stats) {
		fprintf(stderr, "charset: [%s]\n",
			charset == NULL ? "(none)" : charset);
		fprintf(stderr, "reject: [%s]\n", reject);
		fprintf(stderr, "flags: %#x\n", (unsigned) flags);
	}

	/*
	 * Phases:
	 *
	 * 1. per pattern, categorization
	 * 2. per set of literals/general regex to FSMs
	 * 3. union FSMs, codegen
	 */

	if (general_limit == 0) {
		general_limit = SIZE_MAX;
	}

	/*
	 * We're keying unique id to regex pattern spellings. The index
	 * here is used as an fsm_end_id_t.
	 *
	 * We don't use patterns[] just for debugging, we use it for passing
	 * to re_comp(). But not to re_strings(), we pass lit_s there.
	 */

	struct pattern_set declined;
	struct pattern_set general;
	struct literal_set literals[4]; /* unanchored, left, right, both */

	declined.count = 0;
	general.count  = 0;

	for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
		literals[i].count = 0;
	}

	/*
	 * On end id numbering: The UI must be one pattern per line, because
	 * it's the only format that doesn't introduce a syntax with escaping.
	 * So that means we need to track the line number associated with each
	 * regex. And that means the line number is necessarily the end id.
	 * So each pattern carries its associated end id.
	 *
	 * We have four whole FSMs from re_strings(), each potentially contains
	 * a set of end ids. And each regex in general.a[i] has a single end id.
	 */

	/*
	 * Categorize patterns
	 */
	for (int arg = 0; arg < argc; arg++) {
		fsm_end_id_t id = 0;
		FILE *f;
		char *s;
		enum re_dialect dialect = default_dialect;

		if (override_dialect) {
			const char *ext = strrchr(argv[arg], '.');
			if (ext != NULL) {
				ext++;
				fprintf(stderr, "overriding dialect by extension for %s: %s\n",
					argv[arg], ext);
				dialect = dialect_name(ext);
			}
		}

		f = xopen(argv[arg]);

		if (argc > 1) {
			id = arg;
		}

		while (s = xgetline(f), s != NULL) {
			struct re_err err;
			const char *lit_s;
			size_t lit_n;
			enum category r;
			enum re_strings_flags strings_flags;

			/*
			 * This is so we can fit the endid value in an int for the
			 * return value of the generated code.
			 */
			if (id > INT_MAX) {
				fprintf(stderr, "pattern count overflow\n");
				exit(EXIT_FAILURE);
			}

			r = categorize(dialect, flags,
				charset, reject, s,
				&err, &lit_s, &lit_n, &strings_flags);

			/*
			 * For pattern sets, s storage belongs to the entry in the set
			 * (and is freed when the set is destroyed). For literals, we're
			 * done with the original syntax, and use lit_s/lit_n instead.
			 */
			if (dialect != RE_LITERAL && r == CATEGORY_LITERAL) {
				free(s);
			}

			if (dialect == RE_LITERAL && r == CATEGORY_LITERAL && unanchored_literals) {
				strings_flags = 0;
			}

			if (verbose) {
				switch (r) {
				case CATEGORY_LITERAL:
					fprintf(stderr, "literal %s:#%u '%.*s'\n",
						argv[arg], id, (int) lit_n, lit_s);
					fprintf(stderr, "literal %s:#%u /%s%.*s%s/\n",
						argv[arg], id,
						(strings_flags & RE_STRINGS_ANCHOR_LEFT) ? "^" : "",
						(int) lit_n, lit_s,
						(strings_flags & RE_STRINGS_ANCHOR_RIGHT) ? "$" : "");
					break;

				case CATEGORY_EMPTY:
					/* fallthrough */
				case CATEGORY_GENERAL:
					fprintf(stderr, "general %s:#%u: /%s/\n",
						argv[arg], id, s);
					break;

				case CATEGORY_ERROR:
					fprintf(stderr, "declined (%s) %s:#%u: /%s/",
						category_reason(r),
						argv[arg], id, s);
					re_perror(dialect, &err, NULL, NULL);
					break;

				default:
					fprintf(stderr, "declined (%s) %s:#%u: /%s/\n",
						category_reason(r),
						argv[arg], id, s);
					break;
				}
			}

			switch (r) {
			case CATEGORY_LITERAL:
				assert(strings_flags >= 0 && strings_flags <= 3);
				append_literal(&literals[strings_flags],
					lit_s, lit_n,
					id);
				break;

			case CATEGORY_EMPTY:
				/* fallthrough */
			case CATEGORY_GENERAL:
				append_pattern(
					general.count < general_limit ? &general : &declined,
					id, dialect, s);
				break;

			default:
				if (strict) {
					exit(EXIT_FAILURE);
				}

				append_pattern(&declined, id, dialect, s);
			}

			if (argc == 1) {
				id++;
			}
		}

		fclose(f);
	}

	/*
	 * realloc down to size, note this leaves some .a arrays NULL
	 * when its count is 0.
	 */
	if (declined.count > 0) {
		declined.a = xrealloc(declined.a, declined.count * sizeof *declined.a);
	}
	if (general.count > 0) {
		general.a  = xrealloc(general.a, general.count * sizeof *general.a);
	}
	for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
		if (literals[i].count > 0) {
			literals[i].a  = xrealloc(literals[i].a, literals[i].count * sizeof *literals[i].a);
		}
	}

	/*
	 * This is conservative allocation, the declined list may still grow yet.
	 * We track actual usage in fsm_count.
	 */
	struct fsm **fsms = xmalloc(
		(general.count + sizeof literals / sizeof *literals)
		* sizeof *fsms);
	size_t fsm_count = 0;

	/*
	 * The order of processing literals and regexps here is arbitrary.
	 * If this were a threaded program, each set of literals and each
	 * individual general regex could be handled in parallel.
	 *
	 * Also, the order of FSMs in the fsms[] array is not significant,
	 * they could be populated in any order. They're just going to get
	 * unioned anyway.
	 *
	 * We'd probably have a CLI option for the size of the worker pool.
	 * However the bottleneck is fsm_determinise() after union, not here.
	 * So this doesn't help until fsm_determinise() also runs in parallel.
	 */

// TODO: also have strict mode error for empty strings and unsatisfiable expressions
// TODO: better to do this when looking at the DFA for both literals and for regexps

	/* sets of literals */
	{
		enum re_strings_flags flags[sizeof literals / sizeof *literals] = {
			0,
			RE_STRINGS_ANCHOR_LEFT,
			RE_STRINGS_ANCHOR_RIGHT,
			RE_STRINGS_ANCHOR_LEFT | RE_STRINGS_ANCHOR_RIGHT
		};

		for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
			if (show_stats) {
				fprintf(stderr, "literals[%zu].count = %zu\n", i, literals[i].count);
			}

			struct fsm *fsm = build_literals_fsm(show_stats, charset,
				&literals[i], flags[i]);
			assert(fsm != NULL);

			if (verbose) {
				fprintf(stderr, "fsms[%zu] <- literal[%zu]\n", fsm_count, i);
			}

			fsms[fsm_count] = fsm;
			fsm_count++;
		}
	}

	/* individual regexps */
	{
		for (size_t i = 0; i < general.count; i++) {
			if (verbose) {
				fprintf(stderr, "general[%zu]: #%u /%s/\n",
					i, general.a[i].id, general.a[i].s);
			}

			struct fsm *fsm = build_pattern_fsm(show_stats, charset,
				general.a[i].dialect, strict,
				general.a[i].s, flags, general.a[i].id);
			if (fsm == NULL) {
				append_pattern(&declined, i, general.a[i].dialect, general.a[i].s);
				continue;
			}

			if (verbose) {
				fprintf(stderr, "fsms[%zu] <- general[%zu]\n", fsm_count, i);
			}

			fsms[fsm_count] = fsm;
			fsm_count++;
		}

		if (general.count > general_limit) {
			general.count = general_limit;
		}
	}

	fsms = xrealloc(fsms, fsm_count * sizeof *fsms);

	if (show_stats) {
		fprintf(stderr, "literals (unanchored): %zu patterns, %u states\n",
			literals[0].count, fsm_countstates(fsms[0]));
		fprintf(stderr, "literals (^left): %zu patterns, %u states\n",
			literals[1].count, fsm_countstates(fsms[1]));
		fprintf(stderr, "literals (right$): %zu patterns, %u states\n",
			literals[2].count, fsm_countstates(fsms[2]));
		fprintf(stderr, "literals (^both$): %zu patterns, %u states\n",
			literals[3].count, fsm_countstates(fsms[3]));
		fprintf(stderr, "general: %zu patterns (limit %zu)\n",
			general.count, general_limit);
		fprintf(stderr, "declined: %zu patterns\n",
			declined.count);
	}

	if (verbose) {
		for (size_t i = 0; i < declined.count; i++) {
			fprintf(stderr, "declined[%zu]: #%u /%s/\n",
				i, declined.a[i].id, declined.a[i].s);
		}
	}

	/*
	 * The declined patterns are intended to be handled by some other regex
	 * engine. The user is expected to disambiguate these by pattern spelling.
	 * This gives a way to map back to the per-line ID from the original list.
	 */
	if (declined_file != NULL) {
		FILE *f;

		f = fopen(declined_file, "w");
		if (f == NULL) {
			perror(declined_file);
			exit(EXIT_FAILURE);
		}

		for (size_t i = 0; i < declined.count; i++) {
			fprintf(f, "%s\n", declined.a[i].s);
		}

		fclose(f);
	}

	for (size_t i = 0; i < declined.count; i++) {
		free((void *) declined.a[i].s);
	}
	if (declined.count > 0) {
		free(declined.a);
	}

	if (show_stats) {
		fprintf(stderr, "fsm_count = %zu FSMs prior to union\n", fsm_count);
	}

	if (verbose) {
		for (size_t i = 0; i < fsm_count; i++) {
 			fprintf(stderr, "fsm[%zu] = %u states\n", i, fsm_countstates(fsms[i]));
		}
	}

	/*
	 * Union down and codegen.
	 */
	{
		struct fsm *fsm;

		struct fsm_hooks hooks = {
			.conflict = conflict,
			.hook_opaque = & (struct check_env) { &literals, &general }
		};

		/*
		 * fsm_union_array introduces epsilons.
		 * If we construct anchored DFA and deal with ^.* ourselves here,
		 * we can union union without introducing epislons.
		 * This should reduce pressure on fsm_determinise() below.
		 */

		fsm = fsm_union_array(fsm_count, fsms, NULL);
		if (fsm == NULL) {
			perror("fsm_union_array");
			exit(EXIT_FAILURE);
		}

		free(fsms);

		if (show_stats) {
			fprintf(stderr, "nfa: %u states\n", fsm_countstates(fsm));
		}

		/*
		 * We determinise but do not minimise the single DFA, like in lx.
		 * This keeps end states (and therefore end ids) unique. But unlike lx,
		 * end ids don't overlap between patterns (because they represent lines
		 * in the input file). So there isn't an advantage to minimising here.
		 */
		if (!fsm_determinise(fsm)) {
			perror("fsm_determinise");
			exit(EXIT_FAILURE);
		}

		if (show_stats) {
			fprintf(stderr, "dfa: %u states\n", fsm_countstates(fsm));
		}

		if (-1 == fsm_print(stdout, fsm, &opt, &hooks, fsm_lang)) {
			exit(EXIT_FAILURE);
		}

		fsm_free(fsm);
	}

	for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
		for (size_t j = 0; j < literals[i].count; j++) {
			free((void *) literals[i].a[j].p); /* allocated by re_is_literal() */
		}
		if (literals[i].count > 0) {
			free(literals[i].a);
		}
	}

	for (size_t i = 0; i < general.count; i++) {
		free((void *) general.a[i].s);
	}
	if (general.count > 0) {
		free(general.a);
	}

#ifdef __linux__
	if (show_stats) {
		struct rusage ru;

		if (getrusage(RUSAGE_SELF, &ru) != 0) {
			perror("getrusage");
			exit(EXIT_FAILURE);
		}

		fprintf(stderr, "rusage.utime: %g\n", (double) ru.ru_utime.tv_sec + (double) ru.ru_utime.tv_usec / 1000000);
		fprintf(stderr, "rusage.stime: %g\n", (double) ru.ru_stime.tv_sec + (double) ru.ru_stime.tv_usec / 1000000);
		fprintf(stderr, "rusage.maxrss: %zu MiB\n", (size_t) ru.ru_maxrss / 1024);
	}
#endif
}

