/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_H
#define RE_H

struct fsm;
struct fsm_options;

enum re_dialect {
	RE_LIKE,
	RE_LITERAL,
	RE_GLOB,
	RE_NATIVE,
	RE_SQL,
	RE_PCRE
};

enum re_flags {
	RE_ICASE   = 1 << 0,
	RE_TEXT    = 1 << 1,
	RE_MULTI   = 1 << 2,
	RE_REVERSE = 1 << 3,
	RE_SINGLE  = 1 << 4, /* aka PCRE_DOTALL */
	RE_ZONE    = 1 << 5,
	RE_ANCHORED = 1 << 6,
	RE_EXTENDED = 1 << 7,  /* PCRE extended mode */
	RE_END_NL  = 1 << 8, /* end anchor matches '\n' */
	RE_FLAGS_NONE = 0
};

#define RE_ANCHOR (RE_TEXT | RE_MULTI | RE_ZONE)

enum {
	RE_MISC  = 1 << (8 - 1),
	RE_GROUP = 1 << (8 - 2), /* re_err.set/.dup are populated */
	RE_ESC   = 1 << (8 - 3), /* re_err.esc is populated */
	RE_MARK  = 1 << (8 - 4)  /* re_err.u.pos.start/.end are populated */
};

enum re_errno {
	RE_ESUCCESS     =  0 | RE_MISC,
	RE_EERRNO       =  1 | RE_MISC,
	RE_EBADDIALECT  =  2 | RE_MISC,
	RE_EBADGROUP    =  3 | RE_MISC,

	RE_ENEGRANGE    =  0 | RE_MARK | RE_GROUP,
	RE_ENEGCOUNT    =  1 | RE_MARK | RE_GROUP,

	RE_EHEXRANGE    =  0 | RE_MARK | RE_ESC,
	RE_EOCTRANGE    =  1 | RE_MARK | RE_ESC,
	RE_ECOUNTRANGE  =  2 | RE_MARK | RE_ESC,

	RE_EUNSUPPORTED =  0 | RE_MARK,
	RE_EFLAG        =  1 | RE_MARK,
	RE_EBADCP       =  2 | RE_MARK,
	RE_EBADCOMMENT  =  3 | RE_MARK,

	/* the X means "Expected", these are all syntax errors */
	RE_EXSUB        =  4 | RE_MARK,
	RE_EXTERM       =  5 | RE_MARK,
	RE_EXGROUP      =  6 | RE_MARK,
	RE_EXATOM       =  7 | RE_MARK,
	RE_EXCOUNT      =  8 | RE_MARK,
	RE_EXALTS       =  9 | RE_MARK,
	RE_EXRANGE      = 10 | RE_MARK,
	RE_EXCLOSEGROUP = 11 | RE_MARK,
	RE_EXCLOSEFLAGS = 12 | RE_MARK,
	RE_EXGROUPBODY  = 13 | RE_MARK,
	RE_EXEOF        = 14 | RE_MARK,
	RE_EXESC        = 15 | RE_MARK
};

struct re_pos {
	unsigned byte;
};

struct re_err {
	enum re_errno e;
	struct re_pos start;
	struct re_pos end;

	/* XXX: these should be a union */

	/* populated for RE_ECOUNTRANGE; unused otherwise */
	unsigned m;
	unsigned n;

	/* populated for RE_ESC; unused otherwise */
	char esc[32];

	/* populated for RE_GROUP; unused otherwise */
	char set[128];

	/* populated for RE_EXBADCP; unused otherwise */
	unsigned long cp;
};

/*
 * Parse flags. Returns -1 on error.
 */
int
re_flags(const char *s, enum re_flags *f);

/* Callback to acquire each character of the input, in the spirit of fgetc().
 * opaque can be used to pass in callback-specific state, and is otherwise
 * opaque to libre. */
typedef int
re_getchar_fun(void *opaque);

/*
 * Compile a regexp of the given dialect.
 *
 * Returns NULL on error. If non-NULL, the *err struct is populated with the
 * type and 0-indexed byte offset of the error.
 *
 * libfsm provides getc callbacks suitable for use with re_comp; see <fsm/fsm.h>.
 * For example:
 *
 *     const char *s = "abc";
 *     re_comp(RE_NATIVE, fsm_sgetc, &s, NULL, 0, NULL);
 *
 * and:
 *
 *     re_comp(RE_NATIVE, fsm_fgetc, stdin, NULL, 0, NULL);
 *
 * There's nothing special about libfsm's implementation of these; they could
 * equally well be user defined.
 */
struct fsm *
re_comp(enum re_dialect dialect,
	re_getchar_fun *f, void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err);

/*
 * Return a human-readable string describing a given error code. The string
 * returned has static storage, and must not be freed.
 */
const char *
re_strerror(enum re_errno e);

/*
 * Format an error to a file.
 * re_perror() is equivalent to re_ferror(stderr, ...)
 */
void
re_ferror(FILE *f, enum re_dialect dialect, const struct re_err *err,
	const char *file, const char *s);
void
re_perror(enum re_dialect dialect, const struct re_err *err,
	const char *file, const char *s);

/* TODO: a convenience interface in the spirit of strtol() which parses between delimiters (and escapes accordingly) */

#endif

