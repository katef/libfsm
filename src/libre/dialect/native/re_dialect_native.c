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
	class_constructor *ctor;
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
	{ "[:xdigit:]", class_xdigit_fsm }
};

class_constructor *
re_class_native(const char *name)
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
