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

#define DEBUG_PARSING 0

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

static int
data_mods_supported(char *mods)
{
	mods = lstrip(mods);

	/* currently we do not support any data modifiers */
	return *mods == '\0';
}

static char *
decode_escapes(char *s, char **mods)
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

		// end of data, modifiers follow
		case '=':
			*p = '\0';
			if (mods != NULL) {
				*mods = s+1;
			}
			return b;

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
	fprintf(stderr, "%s {-s <regexp>} [infile] [outfile]\n", progname);
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
	{ "dotall"                 , "s",  MOD_SUPPORTED  , RE_SINGLE     }, /* PCRE2_DOTALL */
	{ "dupnames"               , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_DUPNAMES */
	{ "endanchored"            , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_ENDANCHORED */
	{ "escaped_cr_is_lf"       , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_ESCAPED_CR_IS_LF */
	{ "extended"               , "x",  MOD_SUPPORTED  , RE_EXTENDED   }, /* PCRE2_EXTENDED */
	{ "extended_more"          , "xx", MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTENDED_MORE */
	{ "extra_alt_bsux"         , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_ALT_BSUX */
	{ "firstline"              , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_FIRSTLINE */

	/* global search is currently unimplemented.  libfsm stops after the first match.  This should
	 * be safe to allow. */
	{ "global"                 , "g" , MOD_SUPPORTED  , RE_FLAGS_NONE }, /* global search */

	{ "literal"                , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_LITERAL */
	{ "match_line"             , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_MATCH_LINE */
	{ "match_invalid_utf"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_MATCH_INVALID_UTF */
	{ "match_unset_backref"    , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_MATCH_UNSET_BACKREF */
	{ "match_word"             , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_EXTRA_MATCH_WORD */
	{ "multiline"              , "m",  MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_MULTILINE */
	{ "never_backslash_c"      , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NEVER_BACKSLASH_C */
	{ "never_ucp"              , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NEVER_UCP */
	{ "never_utf"              , NULL, MOD_UNSUPPORTED, RE_FLAGS_NONE }, /* PCRE2_NEVER_UTF */

	/* no_auto_capture will be meaningful when we support capture groups, but is currently
	 * not meaningful
	 */
	{ "no_auto_capture"        , "n",  MOD_SUPPORTED  , RE_FLAGS_NONE }, /* PCRE2_NO_AUTO_CAPTURE */
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

	pos = tok = buf;
	first = true;

	result = 0;

	while (*pos != '\0') {
		char *end;
		size_t mi;

		/* skip to comma delimiter, EOL or EOS */
		while (*pos != ',' && *pos != '\n' && *pos != '\0') {
			pos++;
		}

		end = pos;
		if (*pos) {
			/* if not EOS, then advance pos and set *end to
			 * NUL
			 */
			pos++;
			*end = '\0';
		}

		/* strip beginning whitespace */
		while (tok < end && isspace((unsigned char) *tok)) {
			tok++;
		}

		/* now strip end whitespace */
		if (end > tok) {
			/* if string is not empty, point to character behind ',' or \n' or EOS */
			end--;
		}

		while (end > tok && isspace((unsigned char) *end)) {
			*end = '\0';
			end--;
		}

		/* if we still have a string, process it */
		if (*tok != '\0') {
#if DEBUG_PARSING 
			fprintf(stderr, "  >> modifier is %s\n", tok);
#endif /* DEBUG_PARSING */

			/* TODO: handle -modifier */
			for (mi=0; mi < modifier_table_size; mi++) {
				if (modifier_table[mi].longname && strcmp(tok,modifier_table[mi].longname) == 0) {
					break;
				}
			}

			if (mi < modifier_table_size) {
#if DEBUG_PARSING 
				fprintf(stderr, "  >> found %zu: %s | %s | %s | 0x%04x\n",
					mi,
					modifier_table[mi].longname ? modifier_table[mi].longname : "(none)",
					modifier_table[mi].shortname ? modifier_table[mi].shortname : "(none)",
					modifier_table[mi].supported ? "supported" : "not supported",
					(unsigned)modifier_table[mi].flags);
#endif /* DEBUG_PARSING */

				if (modifier_table[mi].supported == MOD_SUPPORTED) {
					mods = mods | modifier_table[mi].flags;
				} else {
					fprintf(stderr, "line %5zu: unsupported regexp modifier %s\n",
						linenum, tok);
					result = 0;
					goto finish;
				}
			} else if (first && strspn(tok, "BIgimnsx") == strlen(tok)) {
				for (; *tok != '\0'; tok++) {
					if (*tok == 'i') {
						mods = mods | RE_ICASE;
					} else if (*tok == 's') {
						mods = mods | RE_SINGLE;
					} else if (*tok == 'x') {
						/* support 'x' but not 'xx'
						 *
						 * 'x' is extended regexp syntax
						 * "xx" is extended-more syntax
						 */
						if (tok[1] == 'x' ) {
							fprintf(stderr, "line %5zu: unsupported regexp modifier xx\n",
									linenum);
							result = 0;
							goto finish;
						}

						mods = mods | RE_EXTENDED;
					} else if (strchr("gn", *tok) != NULL) {
						/* options that are ignored or default:
						 *
						 * 'g' is global search, which we ignore
						 * 'n' is no_auto_capture, which is not relevant until we support capture groups
						 * 's' is dotall, which is default
						 *
						 * XXX: implement global search
						 * XXX: implement EOL handling so we can specify not-dotall
						 */
					} else if (strchr("BIm", *tok) != NULL) {
						/* options that we don't (yet) support:
						 *
						 * 'B' and 'I' are used to dump PCRE2 internal state information,
						 * and not particularly useful for testing libfsm
						 *
						 * 'm' is multiline support
						 */
						fprintf(stderr, "line %5zu: unsupported regexp modifier %c\n",
							linenum, *tok);
						result = 0;
						goto finish;
					} else {
						fprintf(stderr, "line %5zu: unknown regexp modifier %c\n",
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
		tok = pos;
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

static const char *
state_name(enum state st)
{
	switch (st) {
		case ST_DEFAULT: return "dft";
		case ST_COMMAND: return "cmd";
		case ST_PATTERN: return "pat";
		case ST_MATCHES: return "mat";
		case ST_NOTMATCHES: return "nom";
	}

	return "unk";
}

static const struct fsm_options zero_options;

int main(int argc, char **argv)
{
	struct str l;
	enum state state;
	size_t count, nparsed, linenum, regexp_line;
	int re_ok = 0;
	struct fsm_options opt;
	char **skip;
	size_t nskip;

	FILE *in, *out;

	struct str regexp = init_str;
	bool regexp_esc   = false;
	char regexp_delim = '/';
	const char *regexp_delim_list = "/!\"\'`-=_:;,%&@~";

	/* silence unused warnings */
	(void)argc;
	(void)argv;
	(void)state_name;

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
	nskip   = 0;

	in  = stdin;
	out = stdout;

	skip = xmalloc(argc * sizeof *skip); /* upper bound */

	{
		const char *progname = argv[0];

		int o;
		while (o = getopt(argc,argv, "hs:"), o != -1) {
			switch (o) {
			case 'h':
				usage(progname);
				exit(0);
				break;

			case 's':
				skip[nskip++] = optarg;
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
#if DEBUG_PARSING 
		fprintf(stderr, "line %6zu, state (%d) %s, line: %s%s",
			linenum, state, state_name(state),
			l.len > 0 ? l.s : "",
			l.len > 0 && l.s[l.len-1] == '\n' ? "" : "\n");
#endif /* DEBUG_PARSING */

		switch (state) {
		case ST_DEFAULT:
			if (n > 0) {
				char *delim = strchr(regexp_delim_list, s[0]);
				if (delim != NULL) {
					regexp_delim = *delim;
					regexp_line = linenum;
					state = ST_PATTERN;
					s++;
					n--;
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
				bool end_of_regexp = false;
				enum re_flags mods = RE_FLAGS_NONE;

				for (i=0; i < n; i++) {
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
						end_of_regexp = true;
						s += i+1;
						break;
					}
					else {
						str_append(&regexp,s[i]);
					}
				}

				if (end_of_regexp) {
					char *orig_mods;

					if (*s == '\\') {
						str_append(&regexp, '\\');
						s++;
					}

					orig_mods = xstrdup(s);
#if DEBUG_PARSING
					fprintf(stderr, "<<< regexp: %s :regexp>>>\n", regexp.s);
#endif /* DEBUG_PARSING */
					/* end of regexp... check for modifiers
					*/
					if (parse_modifiers(linenum, s, &mods)) {
						char *re = regexp.s;
						static const struct re_err err_zero;
						size_t j;

						struct re_err comp_err;
						struct fsm *fsm;

						for (j = 0; j < nskip; j++) {
							if (0 == strcmp(regexp.s, skip[j])) {
								fprintf(stderr, "line %5zu: skipping regexp /%s/\n",
									linenum, regexp.s);
								re_ok = false;
								goto skip;
							}
						}

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
								if (mods & RE_SINGLE)   { *m++ = 's'; }
								if (mods & RE_ZONE)     { *m++ = 'z'; }
								if (mods & RE_ANCHORED) { *m++ = 'a'; }
								if (mods & RE_EXTENDED) { *m++ = 'x'; }

								fprintf(out, "M %s\n", &mod_str[0]);
							}

							// check if there's an embedded backslash CR, NL, or NUL
							if (strcspn(regexp.s, "\n\r") < regexp.len || strlen(regexp.s) < regexp.len) {
								size_t i;

								fprintf(stderr, "  >>> embedded CR, NL, or NUL!\n");

								/* turn on retest escape handling in the regexp */
								fprintf(out, "O &\nO +e\n");

								// print regexp prefix
								fputc('~', out);
								for (i = 0; i < regexp.len; i++) {
									switch (regexp.s[i]) {
									case '\\':
										/* have to escape all backslashes in the regexp */
										fputc('\\', out);
										fputc('\\', out);
										break;

									case '\0':
										fputc('\\', out);
										fputc('0', out);
										break;

									case '\n':
										fputc('\\', out);
										fputc('n', out);
										break;

									case '\r':
										fputc('\\', out);
										fputc('r', out);
										break;

									default:
										fputc(regexp.s[i], out);
										break;
									}
								}
								fputc('\n', out);
							} else {
								fprintf(out,"%s\n", regexp.s);
							}
						} else {
							if (regexp_line == linenum) {
								fprintf(stderr, "line %5zu: could not parse regexp /%s/: %s\n",
										linenum, regexp.s, re_strerror(comp_err.e));
							} else {
								fprintf(stderr, "lines %5zu .. %5zu: could not parse regexp /%s/: %s\n",
										regexp_line, linenum, regexp.s, re_strerror(comp_err.e));
							}
						}
					} else {
						size_t orig_mods_len = strlen(orig_mods);
						/* strip off any trailing newlines */
						while (orig_mods_len > 0 && orig_mods[orig_mods_len-1] == '\n') {
							orig_mods[--orig_mods_len] = '\0';
						}

						if (regexp_line == linenum) {
							fprintf(stderr, "line %5zu: unsupported or unknown modifiers for /%s/: %s\n",
									linenum, regexp.s, orig_mods);
						} else {
							fprintf(stderr, "lines %5zu .. %5zu: unsupported or unknown modifiers for /%s/: %s\n",
									regexp_line, linenum, regexp.s, orig_mods);
						}
					}

skip:

					if (orig_mods) {
						free(orig_mods);
					}

					str_clear(&regexp);

					state = ST_MATCHES;
					count++;
				}
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
					char *data;
					char *mods;

					mods = NULL;
					data = decode_escapes(stripws(s), &mods);

					if (mods == NULL || data_mods_supported(mods)) {
						/* XXX - handle any supported modifiers.  although we currently don't
						 * support any
						 */
						fprintf(out, "+%s\n", data);
					} else {
						fprintf(stderr, "line %5zu: unsupported data modifiers: %s\n", linenum, mods);
					}
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
					char *data;
					char *mods;

					mods = NULL;
					data = decode_escapes(stripws(s), &mods);

					if (mods == NULL || data_mods_supported(mods)) {
						/* XXX - handle any supported modifiers.  although we currently don't
						 * support any
						 */
						fprintf(out, "-%s\n", data);
					} else {
						fprintf(stderr, "line %5zu: unsupported data modifiers: %s\n", linenum, mods);
					}
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

	free(skip);
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

