/* $Id$ */

#ifndef RE_H
#define RE_H

struct fsm;

enum re_form {
/* TODO:
	RE_ERE,
	RE_BRE,
	RE_PLAN9,
	RE_PCRE,
*/
	RE_LITERAL,
	RE_GLOB,
	RE_SIMPLE
};

enum re_flags {
	RE_ICASE   = 1 << 0,
	RE_TEXT    = 1 << 1,
	RE_MULTI   = 1 << 2,
	RE_REVERSE = 1 << 3,
	RE_SINGLE  = 1 << 4,
	RE_ZONE    = 1 << 5
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
	RE_MISC   = 1 << (8 - 1),
	RE_SYNTAX = 1 << (8 - 2)  /* syntax errors; re_err.byte is populated */
};

enum re_errno {
	RE_ESUCCESS = 0 | RE_MISC,
	RE_EERRNO   = 1 | RE_MISC,
	RE_EBADFORM = 2 | RE_MISC,

	RE_EXSUB    = 0 | RE_SYNTAX,
	RE_EXTERM   = 1 | RE_SYNTAX,
	RE_EXGROUP  = 2 | RE_SYNTAX,
	RE_EXATOM   = 3 | RE_SYNTAX,
	RE_EXCOUNT  = 4 | RE_SYNTAX,
	RE_EXATOMS  = 5 | RE_SYNTAX,
	RE_EXALTS   = 6 | RE_SYNTAX,
	RE_EXRANGE  = 7 | RE_SYNTAX,
	RE_EXEOF    = 8 | RE_SYNTAX
};

struct re_err {
	enum re_errno e;
	unsigned byte;
};

/*
 * Parse flags. Returns -1 on error.
 */
int
re_flags(const char *s, enum re_flags *f);

/*
 * Create a new empty regexp.
 *
 * Returns NULL on error.
 */
struct fsm *
re_new_empty(void);

/*
 * Compile a regexp of the given form. The function passed acts as a callback
 * to acquire each character of the input, in the spirit of fgetc().
 *
 * Returns NULL on error. If non-NULL, the *err struct is populated with the
 * type and 0-indexed byte offset of the error.
 */
struct fsm *
re_new_comp(enum re_form form, int (*f)(void *opaque), void *opaque,
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
re_perror(const char *func, enum re_form form, const struct re_err *err,
	const char *file, const char *s);


/* TODO: a convenience interface in the spirit of strtol() which parses between delimiters (and escapes accordingly) */


/*
 * Conveniences for re_new_comp().
 */
int re_sgetc(void *opaque); /* expects opaque to be char ** */
int re_fgetc(void *oapque); /* expects opaque to be FILE *  */

#endif

