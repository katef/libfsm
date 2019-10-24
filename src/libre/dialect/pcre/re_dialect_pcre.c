/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../re_char_class.h"

/*
 * While this could be done via the lexer, doing it here
 * greatly reduces the surface area of the types we need to
 * declare in the parser.
 */

#include "../../class.h"

static const struct {
	const char *name;
	char_class_constructor_fun *ctor;
} classes[] = {
	{ "[:alnum:]", class_alnum_fsm },
	{ "[:alpha:]", class_alpha_fsm },
	{ "[:ascii:]", class_ascii_fsm },
	{ "[:blank:]", class_blank_fsm },
	{ "[:cntrl:]", class_cntrl_fsm },
	{ "[:digit:]", class_digit_fsm },
	{ "[:graph:]", class_graph_fsm },
	{ "[:lower:]", class_lower_fsm },
	{ "[:print:]", class_print_fsm },
	{ "[:punct:]", class_punct_fsm },
	{ "[:space:]", class_space_fsm },
	{ "[:upper:]", class_upper_fsm },
	{ "[:word:]", class_word_fsm },
	{ "[:xdigit:]", class_xdigit_fsm },
		
	{ "\\d", class_digit_fsm },
	{ "\\h", NULL            }, /* horizontal ws: [ \t] */
	{ "\\s", class_space_fsm }, /* [\h\v] */
	{ "\\v", NULL            }, /* vertical ws: [\x0a\x0b\x0c\x0d] */
	{ "\\w", class_word_fsm  },

	{ "\\D", NULL }, /* [^\d] */
	{ "\\H", NULL }, /* [^\h] */
	{ "\\S", NULL }, /* [^\s] */
	{ "\\V", NULL }, /* [^\v] */
	{ "\\W", NULL }, /* [^\w] */
	{ "\\N", NULL }  /* [^\n] */
};

enum re_dialect_char_class_lookup_res
re_char_class_pcre(const char *name, char_class_constructor_fun **res)
{
	size_t i;

	assert(name != NULL);
	assert(res != NULL);

	for (i = 0; i < sizeof classes / sizeof *classes; i++) {
		if (0 == strcmp(classes[i].name, name)) {
			if (classes[i].ctor == NULL) {
				return RE_CLASS_UNSUPPORTED;
			}

			*res = classes[i].ctor;
			return RE_CLASS_FOUND;
		}
	}	

	return RE_CLASS_NOT_FOUND;
}

