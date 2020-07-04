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
	const struct class *class;
} classes[] = {
	{ "\\d", &class_digit     },
	{ "\\h", &class_hspace_pcre },
	{ "\\s", &class_space     },
	{ "\\v", &class_vspace_pcre },
	{ "\\w", &class_word      },

	{ "\\D", &class_notdigit  }, /* [^\d] */
	{ "\\H", &class_nothspace_pcre }, /* [^\h] */
	{ "\\S", &class_notspace  }, /* [^\s] */
	{ "\\V", &class_notvspace_pcre }, /* [^\v] */
	{ "\\W", &class_notword   }, /* [^\w] */
	{ "\\N", &class_notnl     }, /* [^\n] */

	{ "[:alnum:]",  &class_alnum  },
	{ "[:alpha:]",  &class_alpha  },
	{ "[:ascii:]",  &class_ascii  },
	{ "[:blank:]",  &class_hspace },  /* space or tab only, not the same as \h */
	{ "[:cntrl:]",  &class_cntrl  },
	{ "[:digit:]",  &class_digit  },
	{ "[:graph:]",  &class_graph  },
	{ "[:lower:]",  &class_lower  },
	{ "[:print:]",  &class_print  },
	{ "[:punct:]",  &class_punct  },
	{ "[:space:]",  &class_space  },
	{ "[:upper:]",  &class_upper  },
	{ "[:word:]",   &class_word   },
	{ "[:xdigit:]", &class_xdigit },
};

const struct class *
re_class_pcre(const char *name)
{
	size_t i;

	assert(name != NULL);

	for (i = 0; i < sizeof classes / sizeof *classes; i++) {
		if (0 == strcmp(classes[i].name, name)) {
			return classes[i].class;
		}
	}	

	return NULL;
}

