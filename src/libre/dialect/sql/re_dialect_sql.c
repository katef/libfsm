/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../class.h"
#include "../../re_char_class.h"

static const struct {
	const char *name;
	char_class_constructor *ctor;
} classes[] = {
	{ "[:ALNUM:]", class_alnum_fsm },
	{ "[:ALPHA:]", class_alpha_fsm },
	{ "[:DIGIT:]", class_digit_fsm },
	{ "[:LOWER:]", class_lower_fsm },
	{ "[:SPACE:]", class_space_fsm },
	{ "[:UPPER:]", class_upper_fsm },
	{ "[:WHITESPACE:]", class_space_fsm }
};

char_class_constructor *
re_char_class_sql(const char *name)
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
