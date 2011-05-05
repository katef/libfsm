/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 76 "parser.act"


	typedef struct fsm *       fsm;
	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	/*
	 * Parse .fsm input from the given file stream.
	 */
	struct fsm *
	fsm_parse(FILE *f);

#line 26 "parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_fsm(fsm, lex_state, act_state);

/* BEGINNING OF TERMINAL DEFINITIONS */

#define lex_label (1)
#define lex_equals (8)
#define lex_comma (7)
#define lex_unknown (10)
#define lex_ident (0)
#define lex_arrow (5)
#define lex_start (3)
#define lex_end (4)
#define lex_eof (9)
#define lex_any (2)
#define lex_sep (6)

/* BEGINNING OF TRAILER */

#line 286 "parser.act"

#line 50 "parser.h"

/* END OF FILE */
