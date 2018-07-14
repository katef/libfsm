/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../re_char_class.h"

/* While this could be done via the lexer, doing it here
 * greatly reduces the surface area of the types we need to
 * declare in the parser. */
#include "../../class.h"

struct pairs {
	const char *s;
	char_class_constructor_fun *ctor;
};
static const struct pairs class_table[] = {
	{ "alnum", class_alnum_fsm },
	{ "alpha", class_alpha_fsm },
	{ "ascii", class_ascii_fsm },
	{ "blank", class_blank_fsm },
	{ "cntrl", class_cntrl_fsm },
	{ "digit", class_digit_fsm },
	{ "graph", class_graph_fsm },
	{ "lower", class_lower_fsm },
	{ "print", class_print_fsm },
	{ "punct", class_punct_fsm },
	{ "space", class_space_fsm },
	{ "upper", class_upper_fsm },
	{ "word", class_word_fsm },
	{ "xdigit", class_xdigit_fsm },
	{ NULL, NULL },
};
		
static const struct pairs char_type_table[] = {
	{ "d", class_digit_fsm },
	{ "h", NULL },		  /* horizontal ws: [ \t] */
	{ "s", class_space_fsm }, /* [\h\v] */
	{ "v", NULL },		  /* vertical ws: [\x0a\x0b\x0c\x0d] */
	{ "w", class_word_fsm },

	{ "D", NULL },		/* [^\d] */
	{ "H", NULL },		/* [^\h] */
	{ "S", NULL },		/* [^\s] */
	{ "V", NULL },		/* [^\v] */
	{ "W", NULL },		/* [^\w] */
	{ "N", NULL },		/* [^\n] */

	{ NULL, NULL },
};
	

enum re_dialect_char_class_lookup_res
re_char_class_pcre(const char *name, char_class_constructor_fun **res)
{
	const struct pairs *t = NULL;
	size_t i;
	assert(res != NULL);
	assert(name != NULL);

	if (name[0] == '\\') {
		name += 1;
		t = char_type_table;
	} else if (0 == strncmp("[:", name, 2)) {
		name += 2;
		t = class_table;
	}

	for (i = 0; t && t[i].s != NULL; i++) {
		if (0 == strncmp(t[i].s, name, strlen(t[i].s))) {
			if (t[i].ctor == NULL) { return RE_CLASS_UNSUPPORTED; }
			*res = t[i].ctor;
			return RE_CLASS_FOUND;
		}
	}	

	return RE_CLASS_NOT_FOUND;
}
