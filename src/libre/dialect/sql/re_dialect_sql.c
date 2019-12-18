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
 * SQL 2003 ISO/IEC 9075-2:2003 (E) 8.6.3 q)
 *   [:SPACE:] is "the set of all character strings of length 1 (one)
 *   that are the <space> character."
 *
 * Not to be confused with [:space:] in other dialects, which is
 * written [:WHITESPACE:] in SQL.
 */
static const struct {
	const char *name;
	const struct class *class;
} classes[] = {
	{ "[:ALNUM:]", &class_alnum },
	{ "[:ALPHA:]", &class_alpha },
	{ "[:DIGIT:]", &class_digit },
	{ "[:LOWER:]", &class_lower },
	{ "[:SPACE:]", &class_spchr },
	{ "[:UPPER:]", &class_upper },
	{ "[:WHITESPACE:]", &class_space }
};

const struct class *
re_class_sql(const char *name)
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
