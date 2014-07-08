/* $Id$ */

#ifndef RE_H
#define RE_H

struct re;

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

enum re_cflags {
	RE_ICASE      = 1 << 0,
	RE_NEWLINE    = 1 << 1
};

enum re_eflags {
	RE_NOTBOL = 1 << 0,
	RE_NOTEOL = 1 << 1,
	RE_GREEDY = 1 << 2
};

enum re_err {
	RE_ESUCCESS,
	RE_ENOMEM,
	RE_EBADFORM,

	/* Syntax errors */
	RE_EXSUB,
	RE_EXTERM,
	RE_EXGROUP,
	RE_EXITEM,
	RE_EXCOUNT,
	RE_EXITEMS,
	RE_EXALTS,
	RE_EXEOF
};


/*
 * Create a new empty regexp.
 *
 * Returns NULL on error.
 */
struct re *
re_new_empty(void);

/*
 * Compile a regexp of the given form. The function passed acts as a callback
 * to acquire each character of the input, in the spirit of fgetc().
 *
 * Returns NULL on error.
 */
struct re *
re_new_comp(enum re_form form, int (*f)(void *opaque), void *opaque,
	enum re_cflags cflags, enum re_err *err);

void
re_free(struct re *re);

/*
 * Return a human-readable string describing a given error code. The string
 * returned has static storage, and must not be freed.
 */
const char *
re_strerror(enum re_err err);

/*
 * Merge a new regexp into an existing regexp as set union of the two languages,
 * and free new.
 *
 * Returns false on error.
 */
int
re_union(struct re *re, struct re *new);

/*
 * Merge a new regexp into an existing regexp as a concatenation of the two
 * languages, and free new.
 *
 * Returns false on error.
 */
int
re_concat(struct re *re, struct re *new);

/*
 * Match a string.
 *
 * TODO: Return the regexp that matched, or NULL for no match.
 */
void
re_exec(const struct re *re, const char *s, enum re_eflags eflags);


/* TODO: a convenience interface in the spirit of strtol() which parses between delimiters (and escapes accordingly) */


/*
 * Conveniences for re_new_comp().
 */
int re_getc_str(void *opaque);  /* expects opaque to be char ** */
int re_getc_file(void *oapque); /* expects opaque to be FILE */

#endif

