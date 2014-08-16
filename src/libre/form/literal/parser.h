/*
 * Automatically generated from the files:
 *	src/libre/form/literal/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 67 "src/libre/parser.act"


	#include "lexer.h"

	typedef struct fsm *       fsm;
	typedef enum re_cflags     cflags;
	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	struct act_state {
		enum lx_token lex_tok;
		enum lx_token lex_tok_save;
		enum re_errno e;

		enum lx_token (*lex_next)(struct lx *);
	};

	struct lex_state {
		struct lx lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		int (*lgetc)(void *opaque);
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { act_state->lex_tok = act_state->lex_next(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok;  \
	                              act_state->lex_tok = tok;                      } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save;  } while (0)

#line 50 "src/libre/form/literal/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__literal(fsm, cflags, lex_state, act_state);
/* BEGINNING OF TRAILER */

#line 405 "src/libre/parser.act"


#line 60 "src/libre/form/literal/parser.h"

/* END OF FILE */
