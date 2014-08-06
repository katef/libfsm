/*
 * Automatically generated from the files:
 *	src/libre/form/simple/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 59 "src/libre/parser.act"


	typedef struct fsm *       fsm;
	typedef enum re_cflags     cflags;
	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	struct act_state {
		int lex_tok;
		int lex_tok_save;
		enum re_err err;

		/*
		 * XXX: This non-portably assumes all struct lex_state * pointers may
		 * store the same representation. Really it ought to be void * instead.
		 */
		int (*lex_nexttoken)(struct lex_state *);
		char (*lex_tokval)(struct lex_state *);
		unsigned (*lex_tokval_u)(struct lex_state *);
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   18
	#define ADVANCE_LEXER    do { act_state->lex_tok = act_state->lex_nexttoken(lex_state); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok;  \
	                              act_state->lex_tok = tok;                      } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save;  } while (0)

#line 42 "src/libre/form/simple/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__simple(fsm, cflags, lex_state, act_state);
/* BEGINNING OF TRAILER */

#line 364 "src/libre/parser.act"


#line 52 "src/libre/form/simple/parser.h"

/* END OF FILE */
