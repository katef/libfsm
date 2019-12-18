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

static const struct {
	const char *name;
	const struct class *class;
} classes[] = {
	{ "[:alnum:]",  &class_alnum  },
	{ "[:alpha:]",  &class_alpha  },
	{ "[:ascii:]",  &class_ascii  },
	{ "[:hspace:]", &class_hspace },
	{ "[:cntrl:]",  &class_cntrl  },
	{ "[:digit:]",  &class_digit  },
	{ "[:graph:]",  &class_graph  },
	{ "[:lower:]",  &class_lower  },
	{ "[:print:]",  &class_print  },
	{ "[:punct:]",  &class_punct  },
	{ "[:space:]",  &class_space  },
	{ "[:upper:]",  &class_upper  },
	{ "[:vspace:]", &class_vspace },
	{ "[:word:]",   &class_word   },
	{ "[:xdigit:]", &class_xdigit }
};

const struct class *
re_class_native(const char *name)
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
