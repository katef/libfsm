/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stddef.h>

#include "../../class.h"
#include "../../class_lookup.h"

/*
 * While this could be done via the lexer, doing it here
 * greatly reduces the surface area of the types we need to
 * declare in the parser.
 */

static const struct {
	const char *name;
	char_class_constructor *ctor;
} classes[] = {
	/*
	 * The short forms are ordered first for sake of pcre_class_name()
	 * giving these in preference when rendering out to pcre dialect.
	 */

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
	{ "\\N", class_notnl_fsm     }, /* [^\n] */

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
};

const char *
pcre_class_name(const char *name)
{
	size_t i;

	assert(name != NULL);

	/*
	 * This is expensive, but for our purposes it doesn't matter yet.
	 */
	for (i = 0; i < sizeof classes / sizeof *classes; i++) {
		if (0 == strcmp(class_name(classes[i].ctor), name)) {
			return classes[i].name;
		}
	}

	return NULL;
}

char_class_constructor *
re_char_class_pcre(const char *name)
{
	size_t i;

	assert(name != NULL);

	for (i = 0; i < sizeof classes / sizeof *classes; i++) {
		if (0 == strcmp(classes[i].name, name)) {
			assert(classes[i].ctor != NULL);

			return classes[i].ctor;
		}
	}	

	return NULL;
}

