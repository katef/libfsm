/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../class.h"
#include "../../re_char_class.h"

enum re_dialect_char_class_lookup_res
re_char_class_like(const char *name, char_class_constructor **res)
{
	(void) name;
	(void) res;

	/* no character classes in this dialect */
	return RE_CLASS_UNSUPPORTED;
}
