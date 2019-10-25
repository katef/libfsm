/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../class.h"
#include "../../re_char_class.h"

char_class_constructor *
re_char_class_literal(const char *name)
{
	(void) name;

	/* no character classes in this dialect */
	return NULL;
}
