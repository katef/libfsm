#include "../../re_char_class.h"

/* While this could be done via the lexer, doing it here
 * greatly reduces the surface area of the types we need to
 * declare in the parser. */

int
re_char_class_pcre(const char *name, enum ast_char_class_id *id)
{
	struct pairs {
		const char *s;
		enum ast_char_type_id id;
	};
	const struct pairs table[] = {
		{ "alnum", AST_CHAR_CLASS_ALNUM },
		{ "alpha", AST_CHAR_CLASS_ALPHA },
		{ "ascii", AST_CHAR_CLASS_ASCII },
		{ "blank", AST_CHAR_CLASS_BLANK },
		{ "cntrl", AST_CHAR_CLASS_CNTRL },
		{ "digit", AST_CHAR_CLASS_DIGIT },
		{ "graph", AST_CHAR_CLASS_GRAPH },
		{ "lower", AST_CHAR_CLASS_LOWER },
		{ "print", AST_CHAR_CLASS_PRINT },
		{ "punct", AST_CHAR_CLASS_PUNCT },
		{ "space", AST_CHAR_CLASS_SPACE },
		{ "upper", AST_CHAR_CLASS_UPPER },
		{ "word", AST_CHAR_CLASS_WORD },
		{ "xdigit", AST_CHAR_CLASS_XDIGIT },
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
re_char_type_pcre(const char *name, enum ast_char_type_id *id)
{
	assert(name != NULL);
	assert(id != NULL);

	if (name[0] != '\\') { return 0; }
	if (name[1] == '\0' || name[2] != '\0') { return 0; }

	switch (name[1]) {
	case 'd': *id = AST_CHAR_TYPE_DECIMAL; break;
	case 'h': *id = AST_CHAR_TYPE_HORIZ_WS; break;
	case 's': *id = AST_CHAR_TYPE_WS; break;
	case 'v': *id = AST_CHAR_TYPE_VERT_WS; break;
	case 'w': *id = AST_CHAR_TYPE_WORD; break;
	case 'D': *id = AST_CHAR_TYPE_NON_DECIMAL; break;
	case 'H': *id = AST_CHAR_TYPE_NON_HORIZ_WS; break;
	case 'S': *id = AST_CHAR_TYPE_NON_WS; break;
	case 'V': *id = AST_CHAR_TYPE_NON_VERT_WS; break;
	case 'W': *id = AST_CHAR_TYPE_NON_WORD; break;
	case 'N': *id = AST_CHAR_TYPE_NON_NL; break;
	default:
		return 0;
	}
	return 1;
}
