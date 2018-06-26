#include "../../re_char_class.h"

int
re_char_class_sql(const char *name, enum ast_char_class_id *id)
{
	struct pairs {
		const char *s;
		enum ast_char_type_id id;
	};
	const struct pairs table[] = {
		{ "ALNUM", AST_CHAR_CLASS_ALNUM },
		{ "ALPHA", AST_CHAR_CLASS_ALPHA },
		{ "DIGIT", AST_CHAR_CLASS_DIGIT },
		{ "LOWER", AST_CHAR_CLASS_LOWER },
		{ "SPACE", AST_CHAR_CLASS_SPACE },
		{ "UPPER", AST_CHAR_CLASS_UPPER },
		{ "WHITESPACE", AST_CHAR_CLASS_SPACE },
	};

	unsigned i;

	if (name == NULL || name[0] != '[' || name[1] != ':') {
		return 0;
	}
	name += 2;

	for (i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
		if (0 == strncmp(table[i].s, name, strlen(table[i].s))) {
			*id = table[i].id;
			return 1;
		}
	}
	return 0;
}

int
re_char_type_sql(const char *name, enum ast_char_type_id *id)
{
	(void)name;
	(void)id;
	return 0;
}
