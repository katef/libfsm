/* $Id$ */

/* TODO: placeholder until a real parser exists */
enum lex_token {
	lex_flags,

	lex_pattern__regex,
	lex_pattern__literal,
	lex_pattern__str__r,
	lex_pattern__str__t,
	lex_pattern__str__n,

	lex_map,
	lex_to,
	lex_semi,
	lex_alt,
	lex_token,

	lex_open,
	lex_close,

	lex_unknown,
	lex_eof
};

