/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "../../re_char_class.h"

#include "../../class.h"

struct pairs {
	const char *s;
	char_class_constructor_fun *ctor;
};
static const struct pairs class_table[] = {
	{ "ALNUM", class_alnum_fsm },
	{ "ALPHA", class_alpha_fsm },
	{ "DIGIT", class_digit_fsm },
	{ "LOWER", class_lower_fsm },
	{ "SPACE", class_space_fsm },
	{ "UPPER", class_upper_fsm },
	{ "WHITESPACE", class_space_fsm },
	{ NULL, NULL },
};

enum re_dialect_char_class_lookup_res
re_char_class_sql(const char *name, char_class_constructor_fun **res)
{
	const struct pairs *t = NULL;
	size_t i;
	assert(res != NULL);
	assert(name != NULL);

	if (0 == strncmp("[:", name, 2)) {
		name += 2;
		t = class_table;
	}

	for (i = 0; t && t[i].s != NULL; i++) {
		if (0 == strncmp(t[i].s, name, strlen(t[i].s))) {
			if (t[i].ctor == NULL) { return RE_CLASS_UNSUPPORTED; }
			*res = t[i].ctor;
			return RE_CLASS_FOUND;
		}
	}	

	return RE_CLASS_NOT_FOUND;
}
