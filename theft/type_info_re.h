/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TYPE_INFO_RE_H
#define TYPE_INFO_RE_H

#include "theft_libfsm.h"
#include "buf.h"

/* Property idea: (a|b) should be the same as union[a, b]  */

struct test_re_info {
	char tag; /* 'r' */
	enum re_dialect dialect;

	size_t size;
	uint8_t *string;

	union {
		struct {
			int unused;
		} literal;
		struct {
			struct pcre_node *head;
		} pcre;
	} u;

	/* Positive pairs: these strings should match */
	size_t pos_count;
	struct string_pair *pos_pairs;

	/* Negative pairs: these strings should NOT match;
	 * skip any NULL string pointers -- no error got
	 * injected in the string, so they were discarded. */
	size_t neg_count;
	struct string_pair *neg_pairs;
};

enum pcre_node_type {
	PN_DOT,      /* . */
	PN_LITERAL,  /* x */
	PN_QUESTION, /* (x)? */
	PN_KLEENE,   /* (x)* */
	PN_PLUS,     /* (x)+ */
	PN_BRACKET,  /* [x] or [^x]*/
	PN_ALT,      /* (x|y|z) */
	PN_ANCHOR,   /* ^ or $ */
	PCRE_NODE_TYPE_COUNT
};

enum pcre_bracket_class {
	BRACKET_CLASS_ALNUM  = 0x0001, /* alphanumeric */
	BRACKET_CLASS_ALPHA  = 0x0002, /* alphabetic */
	BRACKET_CLASS_ASCII  = 0x0004, /* 0-127 */
	BRACKET_CLASS_BLANK  = 0x0008, /* space or tab */
	BRACKET_CLASS_CNTRL  = 0x0010, /* control character */
	BRACKET_CLASS_DIGIT  = 0x0020, /* decimal digit */
	BRACKET_CLASS_GRAPH  = 0x0040, /* printing, excluding space */
	BRACKET_CLASS_LOWER  = 0x0080, /* lower case letter */
	BRACKET_CLASS_PRINT  = 0x0100, /* printing, including space */
	BRACKET_CLASS_PUNCT  = 0x0200, /* printing, excluding alphanumeric */
	BRACKET_CLASS_SPACE  = 0x0400, /* white space */
	BRACKET_CLASS_UPPER  = 0x0800, /* upper case letter */
	BRACKET_CLASS_WORD   = 0x1000, /* same as \w */
	BRACKET_CLASS_XDIGIT = 0x2000, /* hexadecimal digit */
	BRACKET_CLASS_MASK   = 0x3FFF,
	BRACKET_CLASS_COUNT  = 14
};

/* Tagged union for a DAG of pcre nodes */
struct pcre_node {
	enum pcre_node_type t;
	bool has_capture;
	size_t capture_id;
	union {
		struct {
			size_t size;
			char *string;
		} literal;
		struct {
			int unused;
		} dot;
		struct {
			struct pcre_node *inner;
		} question;
		struct {
			struct pcre_node *inner;
		} kleene;
		struct {
			struct pcre_node *inner;
		} plus;
		struct { /* bit-set of chars in group */
			bool negated;
			uint64_t set[256 / (8 * sizeof (uint64_t))];
			enum pcre_bracket_class class_flags;
		} bracket;
		struct {
			size_t count;
			struct pcre_node **alts;
		} alt;
		struct {
			enum pcre_anchor_type {
				PCRE_ANCHOR_NONE, /* shrunk away */
				PCRE_ANCHOR_START,
				PCRE_ANCHOR_END,
				PCRE_ANCHOR_TYPE_COUNT,
			} type;
			struct pcre_node *inner;
		} anchor;
	} u;
};

struct flatten_env {
	struct test_env *env;
	uint8_t verbosity;
	struct buf *b;
};

extern const struct theft_type_info type_info_re;

enum theft_alloc_res
type_info_re_literal_build_info(struct theft *t,
    struct test_re_info *info);

enum theft_alloc_res
type_info_re_pcre_build_info(struct theft *t,
    struct test_re_info *info);

void type_info_re_pcre_free(struct pcre_node *node);

#endif
