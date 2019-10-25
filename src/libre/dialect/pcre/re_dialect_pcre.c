/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../class.h"
#include "../../re_char_class.h"

/*
 * While this could be done via the lexer, doing it here
 * greatly reduces the surface area of the types we need to
 * declare in the parser.
 */

static const struct {
	const char *name;
	char_class_constructor *ctor;
} classes[] = {
	{ "[:alnum:]", class_alnum_fsm },
	{ "[:alpha:]", class_alpha_fsm },
	{ "[:ascii:]", class_ascii_fsm },
	{ "[:blank:]", class_hspace_fsm },
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
		
	{ "\\d", class_digit_fsm     },
	{ "\\h", class_hspace_fsm    },
	{ "\\s", class_space_fsm     },
	{ "\\v", class_vspace_fsm    },
	{ "\\w", class_word_fsm      },

	{ "\\D", class_notdigit_fsm  }, /* [^\d] */
	{ "\\H", class_nothspace_fsm }, /* [^\h] */
	{ "\\S", class_notspace_fsm  }, /* [^\s] */
	{ "\\V", class_notvspace_fsm }, /* [^\v] */
	{ "\\W", class_notword_fsm   }, /* [^\w] */
	{ "\\N", class_notnl_fsm     }  /* [^\n] */
};

enum re_dialect_char_class_lookup_res
re_char_class_pcre(const char *name, char_class_constructor **res)
{
	size_t i;

	assert(name != NULL);
	assert(res != NULL);

	for (i = 0; i < sizeof classes / sizeof *classes; i++) {
		if (0 == strcmp(classes[i].name, name)) {
			assert(classes[i].ctor != NULL);

			*res = classes[i].ctor;
			return RE_CLASS_FOUND;
		}
	}	

	return RE_CLASS_NOT_FOUND;
}

