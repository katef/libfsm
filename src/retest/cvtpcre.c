#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/options.h>

#include <re/re.h>

#include <adt/xalloc.h>

#include <unistd.h>

struct str {
	char *s;
	size_t len;
	size_t cap;
};

static const struct str init_str;

static void
str_append(struct str *str, int ch)
{
	assert(str != NULL);

	if (str->len+1 >= str->cap) {
		size_t newcap;

		if (str->cap < 16) {
			newcap = 32;
		} else if (str->cap < 1024) {
			newcap = 2*str->cap;
		} else {
			newcap = str->cap + str->cap/2;
		}

		str->s = xrealloc(str->s, newcap);
		str->cap = newcap;
	}

	str->s[str->len++] = ch;
	str->s[str->len] = '\0';
}

static void
str_clear(struct str *str)
{
	assert(str != NULL);
	str->len = 0;
}

static struct str *
readline(FILE *f, struct str *str)
{
	int ch;

	str->len = 0;

	while (ch = getc(f), ch != EOF) {
		str_append(str, ch);

		if (ch == '\n') {
			return str;
		}
	}

	return (str->len > 0) ? str : NULL;
}

static int
allws(const char *s)
{
	while (*s && isspace((unsigned char)*s)) {
		s++;
	}

	return *s == '\0';
}

static char *
lstrip(char *s)
{
	while (*s && isspace((unsigned char)*s)) {
		s++;
	}

	return s;
}

static char *
rstrip(char *s)
{
	char *e;
	size_t n;

	assert(s != NULL);

	n = strlen(s);
	if (n > 0) {
		e = s+n-1;
		while (e>s && isspace(*e)) {
			*e = '\0';
			e--;
		}
	}

	return s;
}

static char *
stripws(char *s)
{
	assert(s != NULL);
	return rstrip(lstrip(s));
}

static char *
decode_escapes(char *s)
{
	char *b = s, *p = s;

	while (*s) {
		if (*s != '\\') {
			*p++ = *s++;
			continue;
		}

		s++;
		if (*s == '\0') {
			continue;
		}

		switch (s[0]) {
		case 'a': 
		case 'b':
		case 'e':
		case 'f':
		case 'n':
		case 'r':
		case 't':
		case 'x':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '\\':
			*p++ = '\\'; *p++ = s[0]; break;
		default:
			  *p++ = *s; break;
		}

		s++;
	}

	*p = '\0';
	return b;
}

static void
usage(const char *progname)
{
	fprintf(stderr, "%s -h\n", progname);
	fprintf(stderr, "%s [infile] [outfile]\n", progname);
}

enum mod_supported {
	MOD_UNSUPPORTED = 0,
	MOD_SUPPORTED = 1,
};

static const struct {
	const char *longname;
	const char *shortname;
	int supported;
	enum re_flags flags;
} modifier_table[] = {
	{ "allow_empty_class"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ALLOW_EMPTY_CLASS */
	{ "allow_surrogate_escapes", NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES */
	{ "alt_bsux"               , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ALT_BSUX */
	{ "alt_circumflex"         , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ALT_CIRCUMFLEX */
	{ "alt_verbnames"          , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ALT_VERBNAMES */
	{ "anchored"               , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ANCHORED */
	{ "auto_callout"           , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_AUTO_CALLOUT */
	{ "bad_escape_is_literal"  , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL */
	{ "caseless"               , "i",  MOD_SUPPORTED  , RE_ICASE      }, /* PCRE2_CASELESS */
	{ "dollar_endonly"         , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_DOLLAR_ENDONLY */
	{ "dotall"                 , "s",  MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_DOTALL */
	{ "dupnames"               , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_DUPNAMES */
	{ "endanchored"            , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ENDANCHORED */
	{ "escaped_cr_is_lf"       , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_ESCAPED_CR_IS_LF */
	{ "extended"               , "x",  MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTENDED */
	{ "extended_more"          , "xx", MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTENDED_MORE */
	{ "extra_alt_bsux"         , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_ALT_BSUX */
	{ "firstline"              , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_FIRSTLINE */
	{ "literal"                , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_LITERAL */
	{ "match_line"             , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_MATCH_LINE */
	{ "match_invalid_utf"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_MATCH_INVALID_UTF */
	{ "match_unset_backref"    , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_MATCH_UNSET_BACKREF */
	{ "match_word"             , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_MATCH_WORD */
	{ "multiline"              , "m",  MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_MULTILINE */
	{ "never_backslash_c"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NEVER_BACKSLASH_C */
	{ "never_ucp"              , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NEVER_UCP */
	{ "never_utf"              , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NEVER_UTF */
	{ "no_auto_capture"        , "n",  MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NO_AUTO_CAPTURE */
	{ "no_auto_possess"        , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NO_AUTO_POSSESS */
	{ "no_dotstar_anchor"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NO_DOTSTAR_ANCHOR */
	{ "no_start_optimize"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NO_START_OPTIMIZE */
	{ "no_utf_check"           , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NO_UTF_CHECK */
	{ "ucp"                    , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_UCP */
	{ "ungreedy"               , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_UNGREEDY */
	{ "use_offset_limit"       , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_USE_OFFSET_LIMIT */
	{ "utf"                    , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_UTF */
};

static const size_t modifier_table_size = sizeof modifier_table / sizeof modifier_table[0];

static int
parse_modifiers(size_t linenum, const char *s, enum re_flags *mods_p)
{
	enum re_flags mods = RE_FLAGS_NONE;
	char *buf, *tok, *pos;
	bool first;
	int result;

	assert(s != NULL);

	buf = xstrdup(s);
	if (buf == NULL) {
		return 0;
	}

	tok = buf;
	pos = tok;
	first = true;

	result = 0;

	for (;;) {
		if (*pos == ',' || *pos == '\n' || *pos == '\0') {
			char c, *end;
			size_t mi;

			c = *pos;
			end = pos;

			*end = '\0';
			end--;
			while (end > tok && isspace((unsigned char)*end)) {
				*end = '\0';
				end--;
			}

			if (*tok != '\0') {
				/* TODO: handle -modifier */
				for (mi=0; mi < modifier_table_size; mi++) {
					if (strcmp(tok,modifier_table[mi].longname) == 0) {
						break;
					}
				}

				if (mi < modifier_table_size) {
					if (modifier_table[mi].supported == MOD_SUPPORTED) {
						mods = mods | modifier_table[mi].flags;
					} else {
						fprintf(stderr, "line %5zu: unsupported regexp modifier %c\n",
								linenum, *tok);
						result = 0;
						goto finish;
					}
				} else if (first) {
					for (; *tok != '\0'; tok++) {
						if (*tok == 'i') {
							mods = mods | RE_ICASE;
						} else {
							fprintf(stderr, "line %5zu: unsupported regexp modifier %c\n",
								linenum, *tok);
							result = 0;
							goto finish;
						}
					}
				} else {
					fprintf(stderr, "line %5zu: unknown regexp modifier %s\n",
						linenum, tok);
					result = 0;
					goto finish;

				}
			}

			first = false;

			for (tok = pos+1; isspace((unsigned char)*tok); ) {
				tok++;
			}

			if (c == '\0') {
				break;
			}
		} else {
			pos++;
		}
	}

	result = 1;

finish:
	free(buf);
	if (result && mods_p != NULL) {
		*mods_p = mods;
	}
	return result;
}

enum state {
	ST_DEFAULT,
	ST_COMMAND,
	ST_PATTERN,
	ST_MATCHES,
	ST_NOTMATCHES
};

static const struct fsm_options zero_options;

int main(int argc, char **argv)
{
	struct str l;
	enum state state;
	size_t count, nparsed, linenum;
	int re_ok = 0;
	struct fsm_options opt;

	FILE *in, *out;

	struct str regexp = init_str;
	bool regexp_esc   = false;
	char regexp_delim = '/';
	const char *regexp_delim_list = "/!\"\'`-=_:;,%&@~";

	(void)argc;
	(void)argv;

	l = init_str;
	state = ST_DEFAULT;

	opt = zero_options;
	opt.comments = 0;
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.io = FSM_IO_GETC;

	count   = 0;
	nparsed = 0;
	linenum = 0;

	in  = stdin;
	out = stdout;

	{
		const char *progname = argv[0];

		int o;
		while (o = getopt(argc,argv, "h"), o != -1) {
			switch (o) {
			case 'h':
				usage(progname);
				exit(0);
				break;

			default:
				fprintf(stderr, "%s: unknown option '%c'\n", progname, optopt);
				usage(progname);
				exit(1);
				break;
			}
		}

		argc -= optind;
		argv += optind;

		if (argc > 2) {
			fprintf(stderr, "%s: too many arguments\n", progname);
			usage(progname);
			exit(1);
		}

		if (argc > 0) {
			in = fopen(argv[0], "r");
			if (in == NULL) {
				fprintf(stderr, "%s: error opening '%s' for input: %s\n",
					progname, argv[0], strerror(errno));
				exit(1);
			}
		}

		if (argc > 1) {
			out = fopen(argv[1], "w");
			if (out == NULL) {
				fprintf(stderr, "%s: error opening '%s' for output: %s\n",
					progname, argv[1], strerror(errno));
				exit(1);
			}
		}
	}

	while (readline(in, &l) != NULL) {
		char *s = l.s;
		size_t n = l.len;
		int reset = 0;

		linenum++;

restart:
		switch (state) {
		case ST_DEFAULT:
			if (n > 0) {
				char *delim = strchr(regexp_delim_list, s[0]);
				if (delim != NULL) {
					regexp_delim = *delim;
					state = ST_PATTERN;
					s++;
					goto restart;
				}
				else if (s[0] == '#') {
					state = ST_COMMAND;
					goto restart;
				}
				else {
					/* ignore line */
				}
			}
			break;

		case ST_PATTERN:
			{
				size_t i;
				enum re_flags mods = RE_FLAGS_NONE;

				for (i=0; i < l.len; i++) {
					if (regexp_esc) {
						regexp_esc = false;

						if (s[i] != regexp_delim) {
							str_append(&regexp,'\\');
						}
						str_append(&regexp,s[i]);
					}
					else if (s[i] == '\\') {
						regexp_esc = true;
					}
					else if (s[i] == regexp_delim) {
						s += i+1;
						break;
					}
					else {
						str_append(&regexp,s[i]);
					}
				}

				if (*s == '\\') {
					str_append(&regexp, '\\');
					s++;
				}

				/* end of regexp... check for modifiers
				 */
				if (parse_modifiers(linenum, s, &mods)) {
					char *re = regexp.s;
					static const struct re_err err_zero;

					struct re_err comp_err;
					struct fsm *fsm;

					comp_err = err_zero;

					fsm = re_comp(RE_PCRE, fsm_sgetc, &re, &opt, mods, &comp_err);
					re_ok = (fsm != NULL);
					if (re_ok) {
						fsm_free(fsm);
						nparsed++;
						if (count > 0) {
							fprintf(out,"\n");
						}

						fprintf(out,"# input line %zu\n", linenum);
						if (mods != 0) {
							char mod_str[17], *m;
							memset(&mod_str,0,sizeof mod_str);
							m = &mod_str[0];
							if (mods & RE_ICASE)    { *m++ = 'i'; }
							if (mods & RE_TEXT)     { *m++ = 't'; }
							if (mods & RE_MULTI)    { *m++ = 'm'; }
							if (mods & RE_REVERSE)  { *m++ = 'r'; }
							if (mods & RE_SINGLE)   { *m++ = 'S'; }
							if (mods & RE_ZONE)     { *m++ = 'Z'; }
							if (mods & RE_ANCHORED) { *m++ = 'a'; }

							fprintf(out, "M %s\n", &mod_str[0]);
						}

						fprintf(out,"%s\n", regexp.s);
					} else {
						fprintf(stderr, "line %5zu: could not parse regexp /%s/: %s\n",
							linenum, regexp.s, re_strerror(comp_err.e));
					}
				}

				str_clear(&regexp);

				state = ST_MATCHES;
				count++;
			}
			break;

		case ST_COMMAND:
			{
				/* TODO: need to handle some of the pcre2test commands */
				state = ST_DEFAULT; /* ignore for now! */
			}
			break;

		case ST_MATCHES:
			if (n > 1 && s[0] == '\\' && s[1] == '=') {
				state = ST_NOTMATCHES;
			} else if (n == 0 || allws(s)) {
				reset = 1;
			} else if (n > 0 && s[0] == '/') {
				fprintf(stderr, "state machine failure at line %zu: regexp before matches/notmatches ends\n", linenum);
				reset = 1;
			} else {
				if (re_ok) {
					fprintf(out, "+%s\n", decode_escapes(stripws(s)));
				}
			}
			break;

		case ST_NOTMATCHES:
			if (n == 0 || allws(s)) {
				reset = 1;
			} else if (s[0] == '/') {
				fprintf(stderr, "state machine failure at line %zu: regexp before matches/notmatches ends\n", linenum);
				reset = 1;
			} else {
				if (re_ok) {
					fprintf(out, "-%s\n", decode_escapes(stripws(s)));
				}
			}
			break;
		}

		if (reset) {
			state = ST_DEFAULT;
			re_ok = 0;
		}

		if ((linenum % 500) == 0) {
			fprintf(stderr, "line %5zu: %zu entries, %zu parsed correctly\n", linenum, count, nparsed);
		}
	}

	free(l.s);
	free(regexp.s);

	if (ferror(in)) {
		perror("reading input");
		exit(EXIT_FAILURE);
	}

	if (ferror(out)) {
		perror("writing output");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "done.  %zu lines, %zu entries, %zu parsed correctly\n", linenum, count, nparsed);

	return EXIT_SUCCESS;
}

