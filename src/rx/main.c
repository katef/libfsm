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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

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

enum category {
	CATEGORY_EMPTY,
	CATEGORY_LITERAL,
	CATEGORY_GENERAL,
	CATEGORY_ERROR,
	CATEGORY_UNSUPPORTED,
	CATEGORY_UNSATISFIABLE,
	CATEGORY_ZERO,
	CATEGORY_PERMITTED
};

struct literal {
	const void *p;
	size_t n;
	fsm_end_id_t id;
};

struct literal_set {
	size_t count;
	struct literal *a;
};

struct id_set {
	size_t count;
	fsm_end_id_t *a;
};

/*
TODO: flag to have the generated api only ever return a single endid, and resolve ambiguity by one of several ways:
1. earlier line (lower endid) wins
2. longest match/most specific regex wins (this doesn't work for dfa)
3. error about it at compile time (in particular use rx -q as a lint to find conflicts). this is the opposite to -u. give examples, lx style
- unsure if there are any other strategies for allowing ambiguities
4. allow ambiguious patterns and have the generated api return a set
*/
enum ambig {
	AMBIG_ERROR,
	AMBIG_EARLIEST,
	AMBIG_MULTIPLE
};

static bool
permitted_chars(const char *p, size_t n, const char *accept, const char *reject)
{
	char *s;
	bool r = true;

	assert(p != NULL);
	assert(accept != NULL || reject != NULL);

	/*
	 * p,n is a slice into a \n-terminated buffer, so we can't strcspn() here.
	 * We could provide our own implementation (i.e. memcspn), but until it's
	 * actually a problem I'd prefer to allocate and \0-terminate here just
	 * for the sake of using strcspn().
	 */
	s = xstrndup(p, n);

	if (accept != NULL) {
		r &= s[strspn(s, accept)] == '\0';
	}

	if (reject != NULL) {
		r &= s[strcspn(s, reject)] == '\0';
	}

	free(s);

	return r;
}

int
nlgetc(void *opaque)
{
	const char **s = opaque;
	char c;

	assert(s != NULL);
	assert(*s != NULL);

	c = **s;

	/* skip the newline too */
	(*s)++;

	if (c == '\n') {
		return EOF;
	}

	return (unsigned char) c;
}

static void
append_id(struct id_set *set, fsm_end_id_t id)
{
	assert(set != NULL);
	assert(set->a != NULL);

	set->a[set->count] = id;
	set->count++;
}

static void
append_literal(struct literal_set *set, const char *p, size_t n, fsm_end_id_t id)
{
	assert(set != NULL);
	assert(set->a != NULL);
	assert(p != NULL);

	set->a[set->count].id = id;
	set->a[set->count].p  = p;
	set->a[set->count].n  = n;
	set->count++;
}

static fsm_end_id_t
min_id(const fsm_end_id_t *ids, size_t count)
{
	fsm_end_id_t min;

	assert(ids != NULL);
	assert(count > 0);

	min = ids[0];
	for (fsm_end_id_t i = 1; i < count; i++) {
		if (ids[i] < min) {
			min = ids[i];
		}
	}

	return min;
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

	if (charset == NULL) {
		return fsm;
	}

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
	case CATEGORY_ZERO:          return "contains \\0, which a charset cannot express";
	case CATEGORY_PERMITTED:     return "charset/reject";

	default:
		assert(!"unreached");
		abort();
	}
}

/* return an error reason if a pattern was not successfully appended to a set */
static enum category
categorize(const struct fsm_options *opt, enum re_dialect dialect, enum re_flags flags,
	const char *charset, const char *reject, const char *s,
	struct re_err *err,
	char **lit_s_out, size_t *lit_n_out)
{
	enum re_literal_category category;
	char *lit_s;
	size_t lit_n;
	int r;

	assert(err != NULL);
	assert(lit_s_out != NULL);
	assert(lit_n_out != NULL);

	/*
	 *  -1: Error
	 *   1: Literal, *s may or may not be NULL (i.e. for an empty string), *n >= 0
	 *      and *category is set.
	 */
	const char *q = s;
	r = re_is_literal(dialect, nlgetc, &q, opt, flags, err, &category,
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
		return CATEGORY_UNSATISFIABLE;
	}

	/* is a literal */
	if (r == 1) {
		assert(lit_s != NULL);

		for (size_t i = 0; i < lit_n; i++) {
			if (charset != NULL && lit_s[i] == '\0') {
				return CATEGORY_ZERO;
			}
		}

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

// TODO: explain reject does not apply
		if (!permitted_chars(lit_s, lit_n, charset, NULL)) {
			return CATEGORY_PERMITTED;
		}

		enum re_strings_flags flags = literal_flags(category);
		assert(flags >= 0 && flags <= 3);

		*lit_s_out = lit_s;
		*lit_n_out = lit_n;

		return CATEGORY_LITERAL;
	}

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
// TODO: would need to pass charset here *plus all the regex syntax operators*
// we shouldn't enforce positive charset pre-lexing here, this would need to be done at AST time
	if (!permitted_chars(s, strcspn(s, "\n"), NULL, reject)) {
// XXX: this accidentally includes regex operators, we need those in the charset too
		return CATEGORY_PERMITTED;
	}

	/* not a literal */
	return CATEGORY_GENERAL;
}

/*
 * This is re_strings() but for an array of struct literal
 */
static struct fsm *
literal_strings(const struct fsm_options *opt,
	const struct literal *a, size_t n, enum re_strings_flags flags)
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

	fsm = re_strings_build(g, opt, flags);
	re_strings_free(g);

	/* this is already a DFA, courtesy of re_strings() */
	assert(fsm_all(fsm, fsm_isdfa));

	return fsm;
}

static struct fsm *
build_literals_fsm(const struct fsm_options *opt, bool show_stats,
	const char *charset,
	struct literal_set *set, enum re_strings_flags flags)
{
	struct fsm *fsm;

	assert(set != NULL);

	fsm = literal_strings(opt, set->a, set->count, flags);
// XXX: we do produce an empty fsm for 0 literals assert(!fsm_empty(fsm));

	fsm = intersect_charset(show_stats, charset, fsm);
	if (fsm == NULL) {
		perror("intersect_charset");
		exit(EXIT_FAILURE);
	}

	/* We don't minimise here because this fsm has multiple endids,
	 * and the resulting FSM would be very similar to the current DFA */

	if (fsm_countstates(fsm) == 0) {
		fprintf(stderr, "literals produced empty intersection\n");
		exit(EXIT_FAILURE);
	}

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
build_regex_fsm(const struct fsm_options *opt, bool show_stats,
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
	fsm = re_comp(dialect, nlgetc, &q, opt, flags, &err);

	if (strict && fsm == NULL) {
		const char *p;

		/* patterns[] is \n-terminated per pattern.
		 * We allocate here so we can \0-terminate for re_perror() */
		p = xstrndup(s, strcspn(s, "\n"));
		re_perror(dialect, &err, NULL, p);
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
		fprintf(stdout, "re_comp: /%.*s/ regex is unsatisfiable\n",
			(int) strcspn(s, "\n"), s);
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

	if (!fsm_setendid(fsm, id)) {
		perror("fsm_setendid");
		exit(EXIT_FAILURE);
	}

	fsm = intersect_charset(show_stats, charset, fsm);
	if (fsm == NULL) {
		perror("intersect_charset");
		exit(EXIT_FAILURE);
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

	return fsm;
}

static int
endleaf_c(FILE *f, const fsm_end_id_t *ids, size_t count,
	const void *endleaf_opaque)
{
	assert(ids != NULL);
	assert(endleaf_opaque != NULL);

	enum ambig ambig = * (const enum ambig *) endleaf_opaque;

	/* morally an assertion, but I feel better leaving this in for various user data */
	if (count == 0) {
		fprintf(stderr, "no IDs attached to one accepting state\n");
		exit(EXIT_FAILURE);
	}

	for (fsm_end_id_t i = 0; i < count; i++) {
		assert(ids[i] <= INT_MAX);
	}

	/* exactly one end id means no ambiguious patterns */

	switch (ambig) {
	case AMBIG_ERROR:
		if (count > 1) {
// TODO: explain this more clearly
			fprintf(stderr, "ambigious patterns:");

			for (fsm_end_id_t i = 0; i < count; i++) {
				fprintf(stderr, " %u", ids[i]);
			}

// TODO: give example, lx-style

			fprintf(stderr, "\n");

			exit(EXIT_FAILURE);
		}

		fprintf(f, "return %u;", ids[0]);
		break;

	case AMBIG_EARLIEST:
		/*
		 * The libfsm api guarentees these ids are unique,
		 * and only appear once each, but are not sorted.
		 */
		fprintf(f, "return %u;", min_id(ids, count));
		break;

	case AMBIG_MULTIPLE:
		/*
		 * Here I would like to emit (static unsigned []) { 1, 2, 3 }
		 * but specifying a storage duration for compound literals
		 * is a compiler extension.
		 * So I'm emitting a static const variable declaration instead.
		 */

		fprintf(f, "{\n");
		fprintf(f, "\t\tstatic const unsigned a[] = { ");
		for (fsm_end_id_t i = 0; i < count; i++) {
			fprintf(f, "%u", ids[i]);
			if (i + 1 < count) {
				fprintf(f, ", ");
			}
		}
		fprintf(f, " };\n");
		fprintf(f, "\t\t*ids = a;\n");
		fprintf(f, "\t\t*count = %zu;\n", count);
		fprintf(f, "\t\treturn 0;\n");
		fprintf(f, "\t}");
		break;

	default:
		assert(!"unreached");
		abort();
	}

// TODO: override return -1

	return 0;
}

void
print_fsm_c(struct fsm *fsm, const char *prefix, enum ambig ambig)
{
	const struct fsm_options *old;
	struct fsm_options tmp;

	if (prefix == NULL) {
		prefix = "fsm";
	}

	fprintf(stdout, "/* generated */\n");
	fprintf(stdout, "\n");

	fprintf(stdout, "#include <stdbool.h>\n");
	fprintf(stdout, "#include <stddef.h>\n");
	fprintf(stdout, "\n");

// TODO: not bool?
// TODO: return type depends on enum ambig
// TODO: but also override return -1
	fprintf(stdout, "int\n");
	fprintf(stdout, "%s_main(const char *s, size_t n", prefix);
	if (ambig == AMBIG_MULTIPLE) {
		fprintf(stdout, ",\n");
		fprintf(stdout, "\tconst unsigned **ids, size_t *count");
	}
	fprintf(stdout, ")\n");
	fprintf(stdout, "{\n");
	fprintf(stdout, "\tconst char *b = s, *e = s + n;\n");

	old = fsm_getoptions(fsm);
	assert(old != NULL);

	tmp = *old;
	tmp.fragment = true,
	tmp.endleaf_opaque = &ambig,
	tmp.endleaf = endleaf_c,

	fsm_setoptions(fsm, &tmp);
	fsm_print_vmc(stdout, fsm);
	fsm_setoptions(fsm, old);

	fprintf(stdout, "}\n");
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

static
void usage(const char *name)
{

	if (name == NULL) {
		name = "";
	} else {
		const char *p = strrchr(name, '/');
		name = p != NULL ? p + 1 : name;
	}

	printf("usage: %s: [-ciQquv] [-C charset] [-r dialect] [-R reject] input-file [declined-file]\n", name);
	printf("       %s -h\n", name);
}

int
main(int argc, char *argv[])
{
	bool quiet = false;
	bool strict = false;
	bool verbose = false;
	bool show_stats = false;
	enum ambig ambig = AMBIG_ERROR;

	size_t general_limit = 0;

	const char *charset = NULL;
	const char *input_file = NULL;
	const char *declined_file = NULL;

	enum re_dialect dialect = RE_PCRE;
// TODO: we need to provide a way to disable RE_END_NL
// TODO: RE_SINGLE. also be careful, flags disrupt re_is_literal()
	const enum re_flags flags = 0; // TODO: notably not RE_END_NL

	// TODO: explain we decline these at compile time because NFA->DFA is too costly
	const char *reject = "*+{";

	const struct fsm_options opt = {
		.anonymous_states = true,
		.consolidate_edges = true,
		.comments = false,
		.case_ranges = true,
		.always_hex = false,
		.group_edges = true,
		.io = FSM_IO_PAIR,
// TODO: cli option to set io api
	};

// XXX: actual character set in practice
charset =
	/* RFC9110 field-value stuff... */
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwzyx"
	"0123456789"
	"!#$%&'*+-.^_`|~"

	/* ...plus some extras found in practice */
	"'\"/()@,:; ";

// TODO: -l cli option for codegen lang
// TODO: optional extra argv[1] to output declined patterns (we could iteratively munch down on the list)
// TODO: cli option for numeric limit for general.count to decline patterns
// TODO: cli option for memory limit to decline patterns (implement via allocation hooks)
// TODO: manpage
// TODO: cli option for worker pool threads
// TODO: probably handle each of fsms[] in a separate thread, use worker pool
// TODO: cli flag to set prefix

	{
		const char *name = argv[0];
		int c;

		while (c = getopt(argc, argv, "h" "C:cin:r:R:Qquv"), c != -1) {
			switch (c) {
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

			case 'h':
				usage(name);
				exit(EXIT_SUCCESS);

			case 'i':
				strict = true;
				break;

			case 'n':
				general_limit = atoi(optarg);
				break;

			case 'R':
				reject = optarg;
				break;

			case 'r':
				dialect = dialect_name(optarg);
				break;

			case 'Q':
				/* one-off stats on resource consumption */
				show_stats = true;
				break;

			case 'q':
				/* "quiet" applies to the generated code only */
				quiet = true;
				break;

			case 'u':
				ambig = AMBIG_MULTIPLE;
				break;

			case 'v':
				/* "verbose" means showing information about every pattern and FSM,
				 * this is O(n) output to the length of the input file */
				verbose = true;
				break;

			case '?':
			default:
				usage(name);
				exit(EXIT_FAILURE);
			}
		}

		argc -= optind;
		argv += optind;

		switch (argc) {
		case 2:
			declined_file = argv[1];
			/* fallthrough */
		case 1:
			input_file = argv[0];
			break;

		default:
			usage(name);
			exit(EXIT_FAILURE);
		}
	}

// TODO: error if charset contains \n (it also can't contain \0)

	if (show_stats) {
		fprintf(stderr, "charset: [%s]\n", charset);
		fprintf(stderr, "reject: [%s]\n", reject);
	}

	struct stat sb;
	char *addr;
	int fd;

	size_t patterns_count;

	/*
	 * Phases:
	 *
	 * 1. per byte, delimiting
	 * 2. per pattern, categorization
	 * 3. per set of literals/general regex to FSMs
	 * 4. union FSMs, codegen
	 */

	{
		fd = open(input_file, O_RDONLY);
		if (fd == -1) {
			perror(input_file);
			exit(EXIT_FAILURE);
		}

		if (fstat(fd, &sb) == -1) {
			perror(input_file);
			exit(EXIT_FAILURE);
		}

		addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (addr == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* we assume a trailing newline per pattern in the getc callback */
		if (sb.st_size == 0 || addr[sb.st_size - 1] != '\n') {
			fprintf(stderr, "%s: missing newline at end of file\n", input_file);
			exit(EXIT_FAILURE);
		}

		/*
		 * Here we iterate per byte, finding the \n delimiter for patterns.
		 * Henceforth we iterate per pattern id, not per byte.
		 */
		patterns_count = 0;
		for (const char *p = addr; p - addr < sb.st_size; p++) {
			if (*p == '\n') {
				patterns_count++;
			}
		}

		/*
		 * This is so we can fit the endid value in an int for the
		 * return value of the generated code.
		 */
		if (patterns_count > INT_MAX) {
			fprintf(stderr, "pattern count overflow\n");
			exit(EXIT_FAILURE);
		}

		if (show_stats) {
			fprintf(stderr, "patterns: %zu\n", patterns_count);
		}
	}

	if (general_limit == 0) {
		general_limit = patterns_count;
	}

// we're keying id -> spelling 
// we don't use patterns[] just for debugging, we use it for passing to re_comp(). but not to re_strings(), we pass lit_s there
// note this is \n-terminated, not \0-terminated. need %.*s for printing
// TODO: explain the index here is an endid

	const char **patterns;

	patterns = xmalloc(patterns_count * sizeof *patterns);

	// TODO: patterns_set, fsm_set
	struct id_set declined;
	struct id_set general;
	struct literal_set literals[4]; /* unanchored, left, right, both */

	// TODO: explain we use the worst case of patterns_count for all these, prior to categorization
	declined.a = xmalloc(patterns_count * sizeof *declined.a);
	declined.count = 0;
	general.a  = xmalloc(patterns_count * sizeof *general.a);
	general.count = 0;
		
// XXX: malloc()ing these conservatively is large, i don't like it
	for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
		literals[i].a = xmalloc(patterns_count * sizeof *literals[i].a);
		literals[i].count = 0;
	}

	/*
	 * On end id numbering: The UI must be one pattern per line, because
	 * it's the only format that doesn't introduce a syntax with escaping.
	 * So that means we need to track the line number associated with each
	 * regex. And that means the line number is neccessarily the end id.
	 * So patterns[] is indexed by the end id.
	 *
	 * We have four whole FSMs from re_strings(), each potentially contains
	 * a set of end ids. And each regex in general.a[i] is a single end id.
	 * So general.a[i] is the index into patterns[].
	 */

	/*
	 * Categorize patterns
	 */
	{
		const char *q = addr;

		for (fsm_end_id_t id = 0; id < patterns_count; id++) {
			struct re_err err;
			char *lit_s;
			size_t lit_n;
			enum category r;

			patterns[id] = q;
			if (verbose) {
				fprintf(stderr, "pattern #%u: /%.*s/\n",
					id, (int) strcspn(q, "\n"), patterns[id]);
			}

			r = categorize(&opt, dialect, flags,
				charset, reject, patterns[id],
				&err, &lit_s, &lit_n);

			/*
			 * On error the parser may not have consumed all the input.
			 * So we can't use the current value of q because it potentially
			 * points somewhere mid-way along the regex.
			 */
			q += strcspn(patterns[id], "\n") + 1;

			if (verbose) {
				switch (r) {
				case CATEGORY_LITERAL:
					fprintf(stderr, "literal #%u '%.*s'\n",
						id, (int) lit_n, lit_s);
					break;

				case CATEGORY_EMPTY:
					/* fallthrough */
				case CATEGORY_GENERAL:
					fprintf(stderr, "general #%u: /%.*s/\n",
						id, (int) strcspn(patterns[id], "\n"), patterns[id]);
					break;

				case CATEGORY_ERROR:
					fprintf(stderr, "declined (%s) #%u: /%.*s/",
						category_reason(r), id,
						(int) strcspn(patterns[id], "\n"), patterns[id]);
					re_perror(dialect, &err, NULL, NULL);
					break;

				default:
					fprintf(stderr, "declined (%s) #%u: /%.*s/\n",
						category_reason(r), id,
						(int) strcspn(patterns[id], "\n"), patterns[id]);
					break;
				}
			}

			switch (r) {
			case CATEGORY_LITERAL:
				append_literal(&literals[flags], lit_s, lit_n, id);
				break;

			case CATEGORY_EMPTY:
				/* fallthrough */
			case CATEGORY_GENERAL:
				append_id(&general, id);
				break;

			default:
				if (strict) {
					exit(EXIT_FAILURE);
				}

				append_id(&declined, id);
			}
		}

		/*
		 * realloc down to size, note this leaves some .a arrays NULL
		 * when its count is 0.
		 */
		declined.a = xrealloc(declined.a, declined.count * sizeof *declined.a);
		general.a  = xrealloc(general.a, general.count * sizeof *general.a);
		for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
			literals[i].a  = xrealloc(literals[i].a, literals[i].count * sizeof *literals[i].a);
		}
	}

	/*
	 * This is conservative allocation, the declined list may still grow yet.
	 * We track actual usage in fsm_count.
	 */
	struct fsm **fsms = xmalloc(((sizeof literals / sizeof *literals)
		+ patterns_count - declined.count) * sizeof *fsms);
	size_t fsm_count = 0;

	/*
	 * The order of processing literals and regexps here is arbitrary.
	 * If this were a threaded program, each set of literals and each
	 * individual general regex could be handled in parallel.
	 *
	 * Also, the order of FSMs in the fsms[] array is not significant,
	 * they could be populated in any order. They're just going to get
	 * unioned anyway.
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

			struct fsm *fsm = build_literals_fsm(&opt, show_stats, charset,
				&literals[i], flags[i]);
			assert(fsm != NULL);

			if (verbose) {
				fprintf(stderr, "fsms[%zu] <- literal[%zu]\n", fsm_count, i);
			}

			fsms[fsm_count] = fsm;
			fsm_count++;
		}
	}

	for (size_t i = 0; i < sizeof literals / sizeof *literals; i++) {
		for (size_t j = 0; j < literals[i].count; j++) {
			free((void *) literals[i].a[j].p); /* allocated by re_is_literal() */
		}
		free(literals[i].a);
	}

	/* individual regexps */
	{
		for (size_t i = 0; i < general.count; i++) {
			if (i > general_limit) {
				append_id(&declined, i);
				continue;
			}

			if (verbose) {
				fprintf(stderr, "general[%zu]: #%u /%.*s/\n",
					i, general.a[i],
					(int) strcspn(patterns[general.a[i]], "\n"), patterns[general.a[i]]);
			}

			struct fsm *fsm = build_regex_fsm(&opt, show_stats, charset,
				dialect, strict,
				patterns[general.a[i]], flags, general.a[i]);
			if (fsm == NULL) {
				append_id(&declined, i);
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

	free(general.a);

	if (show_stats) {
		fprintf(stderr, "declined: %zu patterns\n",
			declined.count);
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
	}

	if (verbose) {
		for (size_t i = 0; i < declined.count; i++) {
			fprintf(stderr, "declined[%zu]: #%u /%.*s/\n",
				i, declined.a[i],
				(int) strcspn(patterns[declined.a[i]], "\n"), patterns[declined.a[i]]);
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
			fprintf(f, "%.*s\n",
				(int) strcspn(patterns[declined.a[i]], "\n"), patterns[declined.a[i]]);
		}

		fclose(f);
	}

	free(declined.a);
	free(patterns);

	if (-1 == munmap(addr, sb.st_size)) {
		perror("munmap");
		exit(EXIT_FAILURE);
	}

	if (-1 == close(fd)) {
		perror("close");
		exit(EXIT_FAILURE);
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

		if (!quiet) {
			print_fsm_c(fsm, opt.prefix, ambig);
		}

		fsm_free(fsm);
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

