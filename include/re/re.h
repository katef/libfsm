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
/* TODO:
	RE_ERE,
	RE_BRE,
	RE_PLAN9,
	RE_PCRE,
	RE_JS,
	RE_PYTHON,
*/
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
	RE_SINGLE  = 1 << 4,
	RE_ZONE    = 1 << 5,
	RE_ANCHORED = 1 << 6,
	RE_FLAGS_NONE = 0
};

#define RE_ANCHOR (RE_TEXT | RE_MULTI | RE_ZONE)

enum re_pred {
	RE_SOL = 1 << 0,
	RE_SOZ = 1 << 1,
	RE_SOT = 1 << 2 | RE_SOL,

	RE_EOL = 1 << 3,
	RE_EOZ = 1 << 4,
	RE_EOT = 1 << 5 | RE_EOL
};

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
	RE_EDISTINCT    =  2 | RE_MARK | RE_GROUP,

	RE_EHEXRANGE    =  0 | RE_MARK | RE_ESC,
	RE_EOCTRANGE    =  1 | RE_MARK | RE_ESC,
	RE_ECOUNTRANGE  =  2 | RE_MARK | RE_ESC,

	RE_EXSUB        =  0 | RE_MARK,
	RE_EXTERM       =  1 | RE_MARK,
	RE_EXGROUP      =  2 | RE_MARK,
	RE_EXATOM       =  3 | RE_MARK,
	RE_EXCOUNT      =  4 | RE_MARK,
	RE_EXALTS       =  5 | RE_MARK,
	RE_EXRANGE      =  6 | RE_MARK,
	RE_EXCLOSEGROUP =  7 | RE_MARK,
	RE_EXGROUPBODY  =  8 | RE_MARK,
	RE_EXEOF        =  9 | RE_MARK,
	RE_EXESC        = 10 | RE_MARK,
	RE_EFLAG        = 11 | RE_MARK,
	RE_EXCLOSEFLAGS = 12 | RE_MARK,
	RE_EXUNSUPPORTD = 13 | RE_MARK,
	RE_EBADCP       = 14 | RE_MARK
};

struct re_pos {
	unsigned byte;
};

struct re_err {
	enum re_errno e;
	struct re_pos start;
	struct re_pos end;

	/* XXX: these should be a union */

	/* populated for RE_ECOUNTRANGE; ignored otherwise */
	unsigned m;
	unsigned n;

	/* populated for RE_ESC; ignored otherwise */
	char esc[32];

	/* populated for RE_GROUP; ignored otherwise */
	char set[128];

	/* populated for RE_EBADCP; ignored otherwise */
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
 *     re_comp(RE_NATIVE, fsm_sgetc, &s, 0, NULL);
 *
 * and:
 *
 *     re_comp(RE_NATIVE, fsm_fgetc, stdin, 0, NULL);
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
 * Format an error to stderr.
 */
void
re_perror(enum re_dialect dialect, const struct re_err *err,
	const char *file, const char *s);


/* TODO: a convenience interface in the spirit of strtol() which parses between delimiters (and escapes accordingly) */

#endif

