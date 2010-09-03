/*
 * Automatically generated from the files:
 *	cli/parser.sid
 * and
 *	cli/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 71 "cli/parser.act"


	typedef struct fsm *       fsm;
	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	/*
	 * Parse .fsm input from the given file stream. The fsm passed is expected to
	 * have been created by fsm_new(); it should not have any existing states.
	 * TODO: Have this create an fsm, instead
	 */
	struct fsm *
	fsm_parse(struct fsm *fsm, FILE *f);

#line 28 "cli/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_fsm(fsm, lex_state, act_state);

/* BEGINNING OF TERMINAL DEFINITIONS */

#define lex_label (1)
#define lex_equals (9)
#define lex_comma (8)
#define lex_unknown (11)
#define lex_arrow (6)
#define lex_start (4)
#define lex_end (5)
#define lex_eof (10)
#define lex_any (3)
#define lex_sep (7)
#define lex_id (0)
#define lex_literal (2)

/* BEGINNING OF TRAILER */

#line 253 "cli/parser.act"

#line 53 "cli/parser.h"

/* END OF FILE */
