/*
 * Automatically generated from the files:
 *	src/lx/parser.sid
 * and
 *	src/lx/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 97 "src/lx/parser.act"


	#include <assert.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <stdio.h>
	#include <errno.h>

	#include <adt/xalloc.h>

	#include <fsm/fsm.h>
	#include <fsm/bool.h>
	#include <fsm/pred.h>
	#include <fsm/graph.h>

	#include <re/re.h>

	#include "libfsm/internal.h"

	#include "lexer.h"
	#include "parser.h"
	#include "ast.h"
	#include "var.h"

	typedef char *               string;
	typedef enum re_flags        flags;
	typedef struct fsm *         fsm;
	typedef struct ast_zone *    zone;
	typedef struct ast_mapping * mapping;

	struct act_state {
		enum lx_token lex_tok;
		enum lx_token lex_tok_save;
	};

	struct lex_state {
		struct lx lx;
		struct lx_dynbuf buf;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		struct lx_pos pstart;
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   TOK_UNKNOWN
	#define ADVANCE_LEXER    do { act_state->lex_tok = lx_next(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void
	err(const struct lex_state *lex_state, const char *fmt, ...)
	{
		va_list ap;

		assert(lex_state != NULL);

		va_start(ap, fmt);
		fprintf(stderr, "%u:%u: ",
			lex_state->lx.start.line, lex_state->lx.start.col);
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
		va_end(ap);
	}

	static void
	err_expected(const struct lex_state *lex_state, const char *token)
	{
		err(lex_state, "Syntax error: expected %s", token);
		exit(EXIT_FAILURE);
	}

#line 89 "src/lx/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_pattern(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_152(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_158(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_164(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_pattern_C_Cbody(lex_state, act_state);
static void p_175(lex_state, act_state, fsm, fsm *);
static void p_expr_C_Cprimary_Hexpr(lex_state, act_state, zone, fsm *);
static void p_182(lex_state, act_state, zone *, fsm *, fsm *);
static void p_185(lex_state, act_state, zone *, fsm *, fsm *);
static void p_expr_C_Cpostfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cprefix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_188(lex_state, act_state, zone *, fsm *, fsm *);
static void p_191(lex_state, act_state, fsm *);
static void p_expr(lex_state, act_state, zone, fsm *);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_expr_C_Cinfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_218(lex_state, act_state, ast *, zone *, fsm *, string *);
static void p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state, act_state, zone, fsm *);
extern void p_lx(lex_state, act_state, ast *);
static void p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state, act_state, zone, fsm *);
static void p_110(lex_state, act_state, string *);
static void p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state, act_state, zone, fsm *);
static void p_241(lex_state, act_state, ast *, zone *, string *, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state, act_state, zone, fsm *);
static void p_120(lex_state, act_state);
static void p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state, act_state, zone, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI146, zone ZI147, fsm ZI148)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI149;
		zone ZI150;
		fsm ZI151;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI146, ZI147, ZI148);
		p_152 (lex_state, act_state, ZI146, ZI147, ZI148, &ZI149, &ZI150, &ZI151);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-list */
		{
#line 755 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 159 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_pattern(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT):
		{
			string ZIn;

			/* BEGINNING OF EXTRACT: IDENT */
			{
#line 212 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 185 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-var */
			{
#line 258 "src/lx/parser.act"

		struct ast_zone *z;

		assert((ZIz) != NULL);
		assert((ZIn) != NULL);

		for (z = (ZIz); z != NULL; z = z->parent) {
			(ZIr) = var_find(z->vl, (ZIn));
			if ((ZIr) != NULL) {
				break;
			}

			if ((ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (ZIn));
				goto ZL1;
			}
		}

		(ZIr) = fsm_clone((ZIr));
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 218 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
		}
		break;
	case (TOK_TOKEN):
		{
			string ZIt;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 204 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 238 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 285 "src/lx/parser.act"

		const struct ast_mapping *m;

		assert((ZIt) != NULL);

		(ZIr) = fsm_new();
		if ((ZIr) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			struct fsm_state *s;

			if (m->token == NULL) {
				continue;
			}

			if (0 != strcmp(m->token->s, (ZIt))) {
				continue;
			}

			fsm = fsm_clone(m->fsm);
			if (fsm == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			/*
			 * We don't want to take these with us, because the output here is a
			 * general-purpose FSM for which no mapping has yet been made,
			 * so that it can be used in expressions as for any pattern.
			 */
			for (s = fsm->sl; s != NULL; s = s->next) {
				s->opaque = NULL;
			}

			(ZIr) = fsm_union((ZIr), fsm);
			if ((ZIr) == NULL) {
				perror("fsm_determinise_opaque");
				goto ZL1;
			}
		}
	
#line 290 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			p_pattern_C_Cbody (lex_state, act_state);
			p_191 (lex_state, act_state, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BANG):
		{
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI199;
			zone ZI162;
			fsm ZIq;
			zone ZI200;
			fsm ZI201;
			string ZI202;
			fsm ZI203;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI186);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 625 "src/lx/parser.act"

		assert((ZI186) != NULL);

		if (!fsm_reverse((ZI186))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 353 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, &ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, &ZIz, &ZI180, &ZI199);
			p_164 (lex_state, act_state, ZIz, ZI199, &ZI162, &ZIq);
			p_158 (lex_state, act_state, ZIz, ZIq, &ZI200, &ZI201);
			p_110 (lex_state, act_state, &ZI202);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI201) != NULL);

		if ((ZIexit) == NULL) {
			(ZI203) = (ZI201);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI203) = fsm_subtract((ZI201), (ZIexit));
			if ((ZI203) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI203))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 393 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, &ZIa, &ZIz, &ZI202, &ZI203);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HAT):
		{
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI205;
			zone ZI162;
			fsm ZIq;
			zone ZI206;
			fsm ZI207;
			string ZI208;
			fsm ZI209;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI186);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complete */
			{
#line 616 "src/lx/parser.act"

		assert((ZI186) != NULL);

		if (!fsm_complete((ZI186), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 433 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complete */
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, &ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, &ZIz, &ZI180, &ZI205);
			p_164 (lex_state, act_state, ZIz, ZI205, &ZI162, &ZIq);
			p_158 (lex_state, act_state, ZIz, ZIq, &ZI206, &ZI207);
			p_110 (lex_state, act_state, &ZI208);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI207) != NULL);

		if ((ZIexit) == NULL) {
			(ZI209) = (ZI207);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI209) = fsm_subtract((ZI207), (ZIexit));
			if ((ZI209) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI209))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 473 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, &ZIa, &ZIz, &ZI208, &ZI209);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_IDENT):
		{
			string ZIn;

			/* BEGINNING OF EXTRACT: IDENT */
			{
#line 212 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 497 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_218 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_LPAREN):
		{
			fsm ZI211;
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI212;
			zone ZI162;
			fsm ZIq;
			zone ZI213;
			fsm ZI214;
			string ZI215;
			fsm ZI216;

			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZI211);
			switch (CURRENT_TERMINAL) {
			case (TOK_RPAREN):
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_175 (lex_state, act_state, ZI211, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, &ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, &ZIz, &ZI180, &ZI212);
			p_164 (lex_state, act_state, ZIz, ZI212, &ZI162, &ZIq);
			p_158 (lex_state, act_state, ZIz, ZIq, &ZI213, &ZI214);
			p_110 (lex_state, act_state, &ZI215);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI214) != NULL);

		if ((ZIexit) == NULL) {
			(ZI216) = (ZI214);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI216) = fsm_subtract((ZI214), (ZIexit));
			if ((ZI216) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI216))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 572 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, &ZIa, &ZIz, &ZI215, &ZI216);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TILDE):
		{
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI193;
			zone ZI162;
			fsm ZIq;
			zone ZI194;
			fsm ZI195;
			string ZI196;
			fsm ZI197;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI186);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 607 "src/lx/parser.act"

		assert((ZI186) != NULL);

		if (!fsm_complement((ZI186))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 612 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, &ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, &ZIz, &ZI180, &ZI193);
			p_164 (lex_state, act_state, ZIz, ZI193, &ZI162, &ZIq);
			p_158 (lex_state, act_state, ZIz, ZIq, &ZI194, &ZI195);
			p_110 (lex_state, act_state, &ZI196);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI195) != NULL);

		if ((ZIexit) == NULL) {
			(ZI197) = (ZI195);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI197) = fsm_subtract((ZI195), (ZIexit));
			if ((ZI197) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI197))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 652 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, &ZIa, &ZIz, &ZI196, &ZI197);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TOKEN):
		{
			string ZI219;
			fsm ZI220;
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI221;
			zone ZI162;
			fsm ZIq;
			zone ZI222;
			fsm ZI223;
			string ZI224;
			fsm ZI225;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 204 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZI219 = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZI219 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 688 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 285 "src/lx/parser.act"

		const struct ast_mapping *m;

		assert((ZI219) != NULL);

		(ZI220) = fsm_new();
		if ((ZI220) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			struct fsm_state *s;

			if (m->token == NULL) {
				continue;
			}

			if (0 != strcmp(m->token->s, (ZI219))) {
				continue;
			}

			fsm = fsm_clone(m->fsm);
			if (fsm == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			/*
			 * We don't want to take these with us, because the output here is a
			 * general-purpose FSM for which no mapping has yet been made,
			 * so that it can be used in expressions as for any pattern.
			 */
			for (s = fsm->sl; s != NULL; s = s->next) {
				s->opaque = NULL;
			}

			(ZI220) = fsm_union((ZI220), fsm);
			if ((ZI220) == NULL) {
				perror("fsm_determinise_opaque");
				goto ZL1;
			}
		}
	
#line 740 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
			p_175 (lex_state, act_state, ZI220, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, &ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, &ZIz, &ZI180, &ZI221);
			p_164 (lex_state, act_state, ZIz, ZI221, &ZI162, &ZIq);
			p_158 (lex_state, act_state, ZIz, ZIq, &ZI222, &ZI223);
			p_110 (lex_state, act_state, &ZI224);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI223) != NULL);

		if ((ZIexit) == NULL) {
			(ZI225) = (ZI223);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI225) = fsm_subtract((ZI223), (ZIexit));
			if ((ZI225) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI225))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 781 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, &ZIa, &ZIz, &ZI224, &ZI225);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			fsm ZI227;
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI228;
			zone ZI162;
			fsm ZIq;
			zone ZI229;
			fsm ZI230;
			string ZI231;
			fsm ZI232;

			p_pattern_C_Cbody (lex_state, act_state);
			p_191 (lex_state, act_state, &ZI227);
			p_175 (lex_state, act_state, ZI227, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, &ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, &ZIz, &ZI180, &ZI228);
			p_164 (lex_state, act_state, ZIz, ZI228, &ZI162, &ZIq);
			p_158 (lex_state, act_state, ZIz, ZIq, &ZI229, &ZI230);
			p_110 (lex_state, act_state, &ZI231);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI230) != NULL);

		if ((ZIexit) == NULL) {
			(ZI232) = (ZI230);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI232) = fsm_subtract((ZI230), (ZIexit));
			if ((ZI232) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI232))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 846 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, &ZIa, &ZIz, &ZI231, &ZI232);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-thing */
		{
#line 751 "src/lx/parser.act"

		err_expected(lex_state, "mapping, binding or zone");
	
#line 870 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-thing */
	}
}

static void
p_152(lex_state lex_state, act_state act_state, ast ZI146, zone ZI147, fsm ZI148, ast *ZO149, zone *ZO150, fsm *ZO151)
{
	ast ZI149;
	zone ZI150;
	fsm ZI151;

ZL2_152:;
	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI146, ZI147, ZI148);
			/* BEGINNING OF INLINE: 152 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_152;
			}
			/* END OF INLINE: 152 */
		}
		/* UNREACHED */
	default:
		{
			ZI149 = ZI146;
			ZI150 = ZI147;
			ZI151 = ZI148;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO149 = ZI149;
	*ZO150 = ZI150;
	*ZO151 = ZI151;
}

static void
p_158(lex_state lex_state, act_state act_state, zone ZI154, fsm ZI155, zone *ZO156, fsm *ZO157)
{
	zone ZI156;
	fsm ZI157;

ZL2_158:;
	switch (CURRENT_TERMINAL) {
	case (TOK_PIPE):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI154, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 669 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 947 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: 158 */
			ZI155 = ZIq;
			goto ZL2_158;
			/* END OF INLINE: 158 */
		}
		/* UNREACHED */
	default:
		{
			ZI156 = ZI154;
			ZI157 = ZI155;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO156 = ZI156;
	*ZO157 = ZI157;
}

static void
p_164(lex_state lex_state, act_state act_state, zone ZI160, fsm ZI161, zone *ZO162, fsm *ZO163)
{
	zone ZI162;
	fsm ZI163;

ZL2_164:;
	switch (CURRENT_TERMINAL) {
	case (TOK_AND):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI160, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-intersect */
			{
#line 663 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 1001 "src/lx/parser.c"
			}
			/* END OF ACTION: op-intersect */
			/* BEGINNING OF INLINE: 164 */
			ZI161 = ZIq;
			goto ZL2_164;
			/* END OF INLINE: 164 */
		}
		/* UNREACHED */
	default:
		{
			ZI162 = ZI160;
			ZI163 = ZI161;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO162 = ZI162;
	*ZO163 = ZI163;
}

static void
p_pattern_C_Cbody(lex_state lex_state, act_state act_state)
{
ZL2_pattern_C_Cbody:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF INLINE: 81 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 194 "src/lx/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];

		/* position of first character */
		if (lex_state->p == lex_state->a) {
			lex_state->pstart = lex_state->lx.start;
		}
	
#line 1056 "src/lx/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_ESC):
					{
						/* BEGINNING OF EXTRACT: ESC */
						{
#line 119 "src/lx/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		switch (lex_state->buf.a[1]) {
		case '\\': ZIc = '\\'; break;
		case '\"': ZIc = '\"'; break;
		case 'f':  ZIc = '\f'; break;
		case 'n':  ZIc = '\n'; break;
		case 'r':  ZIc = '\r'; break;
		case 't':  ZIc = '\t'; break;
		case 'v':  ZIc = '\v'; break;

		default:   ZIc = '\0'; break; /* TODO: handle error */
		}

		/* position of first character */
		if (lex_state->p == lex_state->a) {
			lex_state->pstart = lex_state->lx.start;
		}
	
#line 1089 "src/lx/parser.c"
						}
						/* END OF EXTRACT: ESC */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						/* BEGINNING OF EXTRACT: HEX */
						{
#line 187 "src/lx/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);
		assert(*e == '\0');

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err(lex_state, "hex escape %s out of range: expected \\x0..\\x%x inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err(lex_state, "%s: %s: expected \\x0..\\x%x inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (char) (unsigned char) u;
	
#line 1126 "src/lx/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						/* BEGINNING OF EXTRACT: OCT */
						{
#line 160 "src/lx/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);
		assert(*e == '\0');

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err(lex_state, "octal escape %s out of range: expected \\0..\\%o inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err(lex_state, "%s: %s: expected \\0..\\%o inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (char) (unsigned char) u;
	
#line 1163 "src/lx/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 81 */
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 234 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 1181 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: pattern::body */
			goto ZL2_pattern_C_Cbody;
			/* END OF INLINE: pattern::body */
		}
		/* UNREACHED */
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_175(lex_state lex_state, act_state act_state, fsm ZI169, fsm *ZO174)
{
	fsm ZI174;

ZL2_175:;
	switch (CURRENT_TERMINAL) {
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
		{
			fsm ZIq;

			ZIq = ZI169;
			/* BEGINNING OF INLINE: 234 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CROSS):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-cross */
						{
#line 522 "src/lx/parser.act"

		struct fsm_state *start, *end;
		struct fsm_state *old;

		/* TODO: centralise with libre */

		start = fsm_addstate((ZIq));
		if (start == NULL) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		end = fsm_addstate((ZIq));
		if (end == NULL) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		old = fsm_collate((ZIq), fsm_isend);
		if (old == NULL) {
			perror("fsm_collate");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), old, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setend((ZIq), old, 0);
		fsm_setend((ZIq), end, 1);

		if (!fsm_addedge_epsilon((ZIq), start, fsm_getstart((ZIq)))) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), end, start)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setstart((ZIq), start);
	
#line 1265 "src/lx/parser.c"
						}
						/* END OF ACTION: op-cross */
						/* BEGINNING OF INLINE: 175 */
						ZI169 = ZIq;
						goto ZL2_175;
						/* END OF INLINE: 175 */
					}
					/* UNREACHED */
				case (TOK_QMARK):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-qmark */
						{
#line 567 "src/lx/parser.act"

		struct fsm_state *start, *end;
		struct fsm_state *old;

		/* TODO: centralise with libre */

		start = fsm_addstate((ZIq));
		if (start == NULL) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		end = fsm_addstate((ZIq));
		if (end == NULL) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		old = fsm_collate((ZIq), fsm_isend);
		if (old == NULL) {
			perror("fsm_collate");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), old, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setend((ZIq), old, 0);
		fsm_setend((ZIq), end, 1);

		if (!fsm_addedge_epsilon((ZIq), start, fsm_getstart((ZIq)))) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), start, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setstart((ZIq), start);
	
#line 1324 "src/lx/parser.c"
						}
						/* END OF ACTION: op-qmark */
						/* BEGINNING OF INLINE: 175 */
						ZI169 = ZIq;
						goto ZL2_175;
						/* END OF INLINE: 175 */
					}
					/* UNREACHED */
				case (TOK_STAR):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-star */
						{
#line 472 "src/lx/parser.act"

		struct fsm_state *start, *end;
		struct fsm_state *old;

		/* TODO: centralise with libre */

		start = fsm_addstate((ZIq));
		if (start == NULL) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		end = fsm_addstate((ZIq));
		if (end == NULL) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		old = fsm_collate((ZIq), fsm_isend);
		if (old == NULL) {
			perror("fsm_collate");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), old, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setend((ZIq), old, 0);
		fsm_setend((ZIq), end, 1);

		if (!fsm_addedge_epsilon((ZIq), start, fsm_getstart((ZIq)))) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), start, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), end, start)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setstart((ZIq), start);
	
#line 1388 "src/lx/parser.c"
						}
						/* END OF ACTION: op-star */
						/* BEGINNING OF INLINE: 175 */
						ZI169 = ZIq;
						goto ZL2_175;
						/* END OF INLINE: 175 */
					}
					/* UNREACHED */
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 234 */
		}
		/* UNREACHED */
	default:
		{
			ZI174 = ZI169;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO174 = ZI174;
}

static void
p_expr_C_Cprimary_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	switch (CURRENT_TERMINAL) {
	case (TOK_LPAREN):
		{
			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZIq);
			switch (CURRENT_TERMINAL) {
			case (TOK_RPAREN):
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
		}
		break;
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
		{
			p_pattern (lex_state, act_state, ZIz, &ZIq);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_182(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI180, fsm *ZOq)
{
	fsm ZIq;

	switch (CURRENT_TERMINAL) {
	case (TOK_DASH):
		{
			fsm ZIb;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, *ZIz, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-subtract */
			{
#line 651 "src/lx/parser.act"

		assert((*ZI180) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_subtract((*ZI180), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_subtract");
			goto ZL1;
		}
	
#line 1494 "src/lx/parser.c"
			}
			/* END OF ACTION: op-subtract */
		}
		break;
	default:
		{
			ZIq = *ZI180;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_185(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI183, fsm *ZOq)
{
	fsm ZIq;

	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT):
		{
			fsm ZIb;

			p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, *ZIz, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-concat */
			{
#line 634 "src/lx/parser.act"

		assert((*ZI183) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((*ZI183), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 1545 "src/lx/parser.c"
			}
			/* END OF ACTION: op-concat */
		}
		break;
	default:
		{
			ZIq = *ZI183;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_expr_C_Cpostfix_Hexpr(lex_state lex_state, act_state act_state, zone ZI166, fsm *ZO174)
{
	fsm ZI174;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_expr_C_Cprimary_Hexpr (lex_state, act_state, ZI166, &ZIq);
		p_175 (lex_state, act_state, ZIq, &ZI174);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO174 = ZI174;
}

static void
p_expr_C_Cprefix_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	switch (CURRENT_TERMINAL) {
	case (TOK_BANG):
		{
			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZIq);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 625 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_reverse((ZIq))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 1617 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
		}
		break;
	case (TOK_HAT):
		{
			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZIq);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complete */
			{
#line 616 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complete((ZIq), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 1641 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complete */
		}
		break;
	case (TOK_TILDE):
		{
			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZIq);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 607 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complement((ZIq))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 1665 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
		}
		break;
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN):
		{
			p_expr_C_Cpostfix_Hexpr (lex_state, act_state, ZIz, &ZIq);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_188(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI186, fsm *ZOq)
{
	fsm ZIq;

	switch (CURRENT_TERMINAL) {
	case (TOK_DOT):
		{
			fsm ZIb;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, *ZIz, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-product */
			{
#line 646 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 1718 "src/lx/parser.c"
			}
			/* END OF ACTION: op-product */
		}
		break;
	default:
		{
			ZIq = *ZI186;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_191(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RE):
		{
			flags ZIf;
			string ZIs;

			/* BEGINNING OF EXTRACT: RE */
			{
#line 224 "src/lx/parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */

		if (-1 == re_flags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			err(lex_state, "/%s: Unrecognised regexp flags", lex_state->buf.a + 1);
			exit(EXIT_FAILURE);
		}
	
#line 1763 "src/lx/parser.c"
			}
			/* END OF EXTRACT: RE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 246 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		/*
		 * Note we strdup() here because the grammar permits adjacent patterns,
		 * and so the pattern buffer will be overwritten by the LL(1) one-token
		 * lookahead.
		 */
		(ZIs) = xstrdup(lex_state->a);
		if ((ZIs) == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}

		lex_state->p = lex_state->a;
	
#line 1787 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 349 "src/lx/parser.act"

		struct re_err err;
		const char *s;

		assert((ZIs) != NULL);

		s = (ZIs);

		(ZIr) = re_comp(RE_SIMPLE, re_sgetc, &s, (ZIf), &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADFORM);
			/* TODO: pass filename for .lx source */
			re_perror("re_comp", RE_SIMPLE, &err, NULL, (ZIs));
			exit(EXIT_FAILURE);
		}
	
#line 1809 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-regex */
			/* BEGINNING OF ACTION: free */
			{
#line 709 "src/lx/parser.act"

		free((ZIs));
	
#line 1818 "src/lx/parser.c"
			}
			/* END OF ACTION: free */
		}
		break;
	case (TOK_STR):
		{
			string ZIs;

			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 246 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		/*
		 * Note we strdup() here because the grammar permits adjacent patterns,
		 * and so the pattern buffer will be overwritten by the LL(1) one-token
		 * lookahead.
		 */
		(ZIs) = xstrdup(lex_state->a);
		if ((ZIs) == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}

		lex_state->p = lex_state->a;
	
#line 1848 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 332 "src/lx/parser.act"

		struct re_err err;
		const char *s;

		assert((ZIs) != NULL);

		s = (ZIs);

		(ZIr) = re_comp(RE_LITERAL, re_sgetc, &s, 0, &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADFORM);
			/* TODO: pass filename for .lx source */
			re_perror("re_comp", RE_LITERAL, &err, NULL, (ZIs));
			exit(EXIT_FAILURE);
		}
	
#line 1870 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-literal */
			/* BEGINNING OF ACTION: free */
			{
#line 709 "src/lx/parser.act"

		free((ZIs));
	
#line 1879 "src/lx/parser.c"
			}
			/* END OF ACTION: free */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
}

static void
p_expr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Cinfix_Hexpr (lex_state, act_state, ZIz, &ZIq);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_token_Hmapping(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOr, string *ZOt)
{
	fsm ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr (lex_state, act_state, ZIz, &ZIr);
		p_110 (lex_state, act_state, &ZIt);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOt = ZIt;
}

static void
p_expr_C_Cinfix_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr (lex_state, act_state, ZIz, &ZIq);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_218(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BIND):
		{
			fsm ZIr;
			fsm ZIq;

			/* BEGINNING OF INLINE: 119 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_BIND):
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-bind */
					{
#line 727 "src/lx/parser.act"

		err_expected(lex_state, "'='");
	
#line 1998 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-bind */
				}
			ZL2:;
			}
			/* END OF INLINE: 119 */
			p_expr (lex_state, act_state, *ZIz, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZIr) != NULL);

		if ((*ZIexit) == NULL) {
			(ZIq) = (ZIr);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZIq) = fsm_subtract((ZIr), (*ZIexit));
			if ((ZIq) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZIq))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 2037 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_120 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 433 "src/lx/parser.act"

		struct var *v;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);
		assert((ZIq) != NULL);

		(void) (*ZIa);

		v = var_bind(&(*ZIz)->vl, (*ZIn), (ZIq));
		if (v == NULL) {
			perror("var_bind");
			goto ZL1;
		}
	
#line 2064 "src/lx/parser.c"
			}
			/* END OF ACTION: add-binding */
		}
		break;
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_SEMI): case (TOK_TO): case (TOK_MAP): case (TOK_LPAREN):
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK): case (TOK_TILDE):
	case (TOK_BANG): case (TOK_HAT): case (TOK_DASH): case (TOK_DOT):
	case (TOK_PIPE): case (TOK_AND):
		{
			fsm ZI235;
			fsm ZI186;
			fsm ZI183;
			fsm ZI180;
			fsm ZI236;
			zone ZI162;
			fsm ZIq;
			zone ZI237;
			fsm ZI238;
			string ZI239;
			fsm ZI240;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 258 "src/lx/parser.act"

		struct ast_zone *z;

		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);

		for (z = (*ZIz); z != NULL; z = z->parent) {
			(ZI235) = var_find(z->vl, (*ZIn));
			if ((ZI235) != NULL) {
				break;
			}

			if ((*ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (*ZIn));
				goto ZL1;
			}
		}

		(ZI235) = fsm_clone((ZI235));
		if ((ZI235) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 2117 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
			p_175 (lex_state, act_state, ZI235, &ZI186);
			p_188 (lex_state, act_state, ZIz, &ZI186, &ZI183);
			p_185 (lex_state, act_state, ZIz, &ZI183, &ZI180);
			p_182 (lex_state, act_state, ZIz, &ZI180, &ZI236);
			p_164 (lex_state, act_state, *ZIz, ZI236, &ZI162, &ZIq);
			p_158 (lex_state, act_state, *ZIz, ZIq, &ZI237, &ZI238);
			p_110 (lex_state, act_state, &ZI239);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 674 "src/lx/parser.act"

		assert((ZI238) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI240) = (ZI238);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI240) = fsm_subtract((ZI238), (*ZIexit));
			if ((ZI240) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI240))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 2158 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_241 (lex_state, act_state, ZIa, ZIz, &ZI239, &ZI240);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state lex_state, act_state act_state, zone ZI160, fsm *ZO163)
{
	fsm ZI163;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI162;

		p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI160, &ZIq);
		p_164 (lex_state, act_state, ZI160, ZIq, &ZI162, &ZI163);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO163 = ZI163;
}

void
p_lx(lex_state lex_state, act_state act_state, ast *ZOa)
{
	ast ZIa;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		zone ZIparent;
		zone ZIz;
		fsm ZIexit;

		/* BEGINNING OF ACTION: no-zone */
		{
#line 463 "src/lx/parser.act"

		(ZIparent) = NULL;
	
#line 2225 "src/lx/parser.c"
		}
		/* END OF ACTION: no-zone */
		/* BEGINNING OF ACTION: make-ast */
		{
#line 387 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 2238 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 395 "src/lx/parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa), (ZIparent));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 2253 "src/lx/parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 459 "src/lx/parser.act"

		(ZIexit) = NULL;
	
#line 2262 "src/lx/parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 448 "src/lx/parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 2274 "src/lx/parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 144 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 747 "src/lx/parser.act"

		err_expected(lex_state, "EOF");
	
#line 2302 "src/lx/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 144 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 387 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 2323 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 715 "src/lx/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 2333 "src/lx/parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
	goto ZL0;
ZL4:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOa = ZIa;
}

static void
p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI183;

		p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, ZIz, &ZI183);
		p_185 (lex_state, act_state, &ZIz, &ZI183, &ZIq);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_110(lex_state lex_state, act_state act_state, string *ZOt)
{
	string ZIt;

	switch (CURRENT_TERMINAL) {
	case (TOK_MAP):
		{
			/* BEGINNING OF INLINE: 111 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_MAP):
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-map */
					{
#line 723 "src/lx/parser.act"

		err_expected(lex_state, "'->'");
	
#line 2399 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-map */
				}
			ZL2:;
			}
			/* END OF INLINE: 111 */
			switch (CURRENT_TERMINAL) {
			case (TOK_TOKEN):
				/* BEGINNING OF EXTRACT: TOKEN */
				{
#line 204 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 2419 "src/lx/parser.c"
				}
				/* END OF EXTRACT: TOKEN */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: no-token */
			{
#line 455 "src/lx/parser.act"

		(ZIt) = NULL;
	
#line 2437 "src/lx/parser.c"
			}
			/* END OF ACTION: no-token */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOt = ZIt;
}

static void
p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state lex_state, act_state act_state, zone ZI154, fsm *ZO157)
{
	fsm ZI157;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI156;

		p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI154, &ZIq);
		p_158 (lex_state, act_state, ZI154, ZIq, &ZI156, &ZI157);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO157 = ZI157;
}

static void
p_241(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI239, fsm *ZI240)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_SEMI):
		{
			zone ZIto;

			p_120 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-zone */
			{
#line 463 "src/lx/parser.act"

		(ZIto) = NULL;
	
#line 2499 "src/lx/parser.c"
			}
			/* END OF ACTION: no-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 408 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI240) != NULL);

		if ((*ZI239) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI239));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI240), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2530 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 127 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_TO):
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-to */
					{
#line 735 "src/lx/parser.act"

		err_expected(lex_state, "'..'");
	
#line 2561 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 127 */
			p_token_Hmapping (lex_state, act_state, *ZIz, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 395 "src/lx/parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa), (*ZIz));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 2585 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 131 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_120 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-zone */
						{
#line 463 "src/lx/parser.act"

		(ZIx) = NULL;
	
#line 2610 "src/lx/parser.c"
						}
						/* END OF ACTION: no-zone */
						/* BEGINNING OF ACTION: no-token */
						{
#line 455 "src/lx/parser.act"

		(ZIy) = NULL;
	
#line 2619 "src/lx/parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 699 "src/lx/parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 2634 "src/lx/parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 366 "src/lx/parser.act"

		struct fsm_state *e;

		(ZIw) = fsm_new();
		if ((ZIw) == NULL) {
			perror("fsm_new");
			goto ZL5;
		}

		e = fsm_addstate((ZIw));
		if (e == NULL) {
			perror("fsm_addstate");
			goto ZL5;
		}

		if (!fsm_addedge_any((ZIw), fsm_getstart((ZIw)), e)) {
			perror("fsm_addedge_any");
			goto ZL5;
		}

		fsm_setend((ZIw), e, 1);
	
#line 2662 "src/lx/parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 674 "src/lx/parser.act"

		assert((ZIw) != NULL);

		if ((ZIu) == NULL) {
			(ZIv) = (ZIw);
		} else {
			(ZIu) = fsm_clone((ZIu));
			if ((ZIu) == NULL) {
				perror("fsm_clone");
				goto ZL5;
			}

			(ZIv) = fsm_subtract((ZIw), (ZIu));
			if ((ZIv) == NULL) {
				perror("fsm_subtract");
				goto ZL5;
			}

			if (!fsm_trim((ZIv))) {
				perror("fsm_trim");
				goto ZL5;
			}
		}
	
#line 2692 "src/lx/parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 408 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((ZIchild) != NULL);
		assert((ZIchild) != (ZIx));
		assert((ZIv) != NULL);

		if ((ZIy) != NULL) {
			t = ast_addtoken((*ZIa), (ZIy));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL5;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIchild), (ZIv), t, (ZIx));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL5;
		}
	
#line 2723 "src/lx/parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						/* BEGINNING OF INLINE: 138 */
						{
							{
								switch (CURRENT_TERMINAL) {
								case (TOK_OPEN):
									break;
								default:
									goto ZL7;
								}
								ADVANCE_LEXER;
							}
							goto ZL6;
						ZL7:;
							{
								/* BEGINNING OF ACTION: err-expected-open */
								{
#line 739 "src/lx/parser.act"

		err_expected(lex_state, "'{'");
	
#line 2750 "src/lx/parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 138 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						/* BEGINNING OF INLINE: 139 */
						{
							if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
								RESTORE_LEXER;
								goto ZL5;
							}
							{
								switch (CURRENT_TERMINAL) {
								case (TOK_CLOSE):
									break;
								default:
									goto ZL9;
								}
								ADVANCE_LEXER;
							}
							goto ZL8;
						ZL9:;
							{
								/* BEGINNING OF ACTION: err-expected-close */
								{
#line 743 "src/lx/parser.act"

		err_expected(lex_state, "'}'");
	
#line 2782 "src/lx/parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 139 */
					}
					break;
				default:
					goto ZL5;
				}
				goto ZL4;
			ZL5:;
				{
					/* BEGINNING OF ACTION: err-expected-list */
					{
#line 755 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 2803 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 131 */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 408 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI240) != NULL);

		if ((*ZI239) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI239));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI240), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2838 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 408 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((ZIchild) != NULL);
		assert((ZIchild) != (*ZIz));
		assert((ZIr2) != NULL);

		if ((ZIt2) != NULL) {
			t = ast_addtoken((*ZIa), (ZIt2));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIchild), (ZIr2), t, (*ZIz));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2869 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI186;

		p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI186);
		p_188 (lex_state, act_state, &ZIz, &ZI186, &ZIq);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

static void
p_120(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_SEMI):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-semi */
		{
#line 731 "src/lx/parser.act"

		err_expected(lex_state, "';'");
	
#line 2935 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI180;

		p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, ZIz, &ZI180);
		p_182 (lex_state, act_state, &ZIz, &ZI180, &ZIq);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOq = ZIq;
}

/* BEGINNING OF TRAILER */

#line 800 "src/lx/parser.act"


	struct ast *lx_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state lex_state_s;
		struct lex_state *lex_state;
		struct ast *ast;
		struct lx *lx;

		assert(f != NULL);

		lex_state = &lex_state_s;

		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		lx_init(lx);

		lx->lgetc  = lx_fgetc;
		lx->opaque = f;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		lx->buf   = &lex_state->buf;
		lx->push  = lx_dynpush;
		lx->pop   = lx_dynpop;
		lx->clear = lx_dynclear;
		lx->free  = lx_dynfree;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state, &ast);

		assert(ast != NULL);

		return ast;
	}

#line 3013 "src/lx/parser.c"

/* END OF FILE */
