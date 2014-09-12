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
static void p_148(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_154(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_165(lex_state, act_state, fsm, fsm *);
static void p_pattern_C_Cbody(lex_state, act_state);
static void p_170(lex_state, act_state, zone *, fsm *, fsm *);
static void p_173(lex_state, act_state, zone *, fsm *, fsm *);
static void p_176(lex_state, act_state, zone *, fsm *, fsm *);
static void p_expr_C_Cprimary_Hexpr(lex_state, act_state, zone, fsm *);
static void p_179(lex_state, act_state, fsm *);
static void p_expr_C_Cpostfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cprefix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr(lex_state, act_state, zone, fsm *);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_197(lex_state, act_state, ast *, zone *, fsm *, string *);
static void p_expr_C_Cinfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_217(lex_state, act_state, ast *, zone *, string *, fsm *);
extern void p_lx(lex_state, act_state, ast *);
static void p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state, act_state, zone, fsm *);
static void p_106(lex_state, act_state, string *);
static void p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state, act_state, zone, fsm *);
static void p_116(lex_state, act_state);
static void p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state, act_state, zone, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI142, zone ZI143, fsm ZI144)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI145;
		zone ZI146;
		fsm ZI147;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI142, ZI143, ZI144);
		p_148 (lex_state, act_state, ZI142, ZI143, ZI144, &ZI145, &ZI146, &ZI147);
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
#line 760 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 157 "src/lx/parser.c"
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
	
#line 183 "src/lx/parser.c"
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
	
#line 216 "src/lx/parser.c"
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
	
#line 236 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 285 "src/lx/parser.act"

		const struct ast_mapping *m;

		assert((ZIt) != NULL);

		(ZIr) = re_new_empty();
		if ((ZIr) == NULL) {
			perror("re_new_empty");
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
	
#line 288 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			p_pattern_C_Cbody (lex_state, act_state);
			p_179 (lex_state, act_state, &ZIr);
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
p_148(lex_state lex_state, act_state act_state, ast ZI142, zone ZI143, fsm ZI144, ast *ZO145, zone *ZO146, fsm *ZO147)
{
	ast ZI145;
	zone ZI146;
	fsm ZI147;

ZL2_148:;
	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI142, ZI143, ZI144);
			/* BEGINNING OF INLINE: 148 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_148;
			}
			/* END OF INLINE: 148 */
		}
		/*UNREACHED*/
	default:
		{
			ZI145 = ZI142;
			ZI146 = ZI143;
			ZI147 = ZI144;
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
	*ZO145 = ZI145;
	*ZO146 = ZI146;
	*ZO147 = ZI147;
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BANG):
		{
			fsm ZI174;
			fsm ZI171;
			fsm ZI168;
			fsm ZIq;
			zone ZI186;
			fsm ZI187;
			string ZI188;
			fsm ZI189;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI174);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 636 "src/lx/parser.act"

		assert((ZI174) != NULL);

		if (!fsm_reverse((ZI174))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 393 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
			p_176 (lex_state, act_state, &ZIz, &ZI174, &ZI171);
			p_173 (lex_state, act_state, &ZIz, &ZI171, &ZI168);
			p_170 (lex_state, act_state, &ZIz, &ZI168, &ZIq);
			p_154 (lex_state, act_state, ZIz, ZIq, &ZI186, &ZI187);
			p_106 (lex_state, act_state, &ZI188);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

		assert((ZI187) != NULL);

		if ((ZIexit) == NULL) {
			(ZI189) = (ZI187);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI189) = fsm_subtract((ZI187), (ZIexit));
			if ((ZI189) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI189))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 432 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_217 (lex_state, act_state, &ZIa, &ZIz, &ZI188, &ZI189);
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
	
#line 456 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_197 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_LPAREN):
		{
			fsm ZI191;
			fsm ZI174;
			fsm ZI171;
			fsm ZI168;
			fsm ZIq;
			zone ZI192;
			fsm ZI193;
			string ZI194;
			fsm ZI195;

			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZI191);
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
			p_165 (lex_state, act_state, ZI191, &ZI174);
			p_176 (lex_state, act_state, &ZIz, &ZI174, &ZI171);
			p_173 (lex_state, act_state, &ZIz, &ZI171, &ZI168);
			p_170 (lex_state, act_state, &ZIz, &ZI168, &ZIq);
			p_154 (lex_state, act_state, ZIz, ZIq, &ZI192, &ZI193);
			p_106 (lex_state, act_state, &ZI194);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

		assert((ZI193) != NULL);

		if ((ZIexit) == NULL) {
			(ZI195) = (ZI193);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI195) = fsm_subtract((ZI193), (ZIexit));
			if ((ZI195) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI195))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 528 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_217 (lex_state, act_state, &ZIa, &ZIz, &ZI194, &ZI195);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TILDE):
		{
			fsm ZI174;
			fsm ZI171;
			fsm ZI168;
			fsm ZIq;
			zone ZI181;
			fsm ZI182;
			string ZI183;
			fsm ZI184;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI174);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 618 "src/lx/parser.act"

		assert((ZI174) != NULL);

		if (!fsm_complement((ZI174))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 566 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
			p_176 (lex_state, act_state, &ZIz, &ZI174, &ZI171);
			p_173 (lex_state, act_state, &ZIz, &ZI171, &ZI168);
			p_170 (lex_state, act_state, &ZIz, &ZI168, &ZIq);
			p_154 (lex_state, act_state, ZIz, ZIq, &ZI181, &ZI182);
			p_106 (lex_state, act_state, &ZI183);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

		assert((ZI182) != NULL);

		if ((ZIexit) == NULL) {
			(ZI184) = (ZI182);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI184) = fsm_subtract((ZI182), (ZIexit));
			if ((ZI184) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI184))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 605 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_217 (lex_state, act_state, &ZIa, &ZIz, &ZI183, &ZI184);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TOKEN):
		{
			string ZI198;
			fsm ZI199;
			fsm ZI174;
			fsm ZI171;
			fsm ZI168;
			fsm ZIq;
			zone ZI200;
			fsm ZI201;
			string ZI202;
			fsm ZI203;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 204 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZI198 = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZI198 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 639 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 285 "src/lx/parser.act"

		const struct ast_mapping *m;

		assert((ZI198) != NULL);

		(ZI199) = re_new_empty();
		if ((ZI199) == NULL) {
			perror("re_new_empty");
			goto ZL1;
		}

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			struct fsm_state *s;

			if (m->token == NULL) {
				continue;
			}

			if (0 != strcmp(m->token->s, (ZI198))) {
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

			(ZI199) = fsm_union((ZI199), fsm);
			if ((ZI199) == NULL) {
				perror("fsm_determinise_opaque");
				goto ZL1;
			}
		}
	
#line 691 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
			p_165 (lex_state, act_state, ZI199, &ZI174);
			p_176 (lex_state, act_state, &ZIz, &ZI174, &ZI171);
			p_173 (lex_state, act_state, &ZIz, &ZI171, &ZI168);
			p_170 (lex_state, act_state, &ZIz, &ZI168, &ZIq);
			p_154 (lex_state, act_state, ZIz, ZIq, &ZI200, &ZI201);
			p_106 (lex_state, act_state, &ZI202);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

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
	
#line 731 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_217 (lex_state, act_state, &ZIa, &ZIz, &ZI202, &ZI203);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			fsm ZI205;
			fsm ZI174;
			fsm ZI171;
			fsm ZI168;
			fsm ZIq;
			zone ZI206;
			fsm ZI207;
			string ZI208;
			fsm ZI209;

			p_pattern_C_Cbody (lex_state, act_state);
			p_179 (lex_state, act_state, &ZI205);
			p_165 (lex_state, act_state, ZI205, &ZI174);
			p_176 (lex_state, act_state, &ZIz, &ZI174, &ZI171);
			p_173 (lex_state, act_state, &ZIz, &ZI171, &ZI168);
			p_170 (lex_state, act_state, &ZIz, &ZI168, &ZIq);
			p_154 (lex_state, act_state, ZIz, ZIq, &ZI206, &ZI207);
			p_106 (lex_state, act_state, &ZI208);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

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
	
#line 793 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_217 (lex_state, act_state, &ZIa, &ZIz, &ZI208, &ZI209);
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
#line 756 "src/lx/parser.act"

		err_expected(lex_state, "mapping, binding or zone");
	
#line 817 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-thing */
	}
}

static void
p_154(lex_state lex_state, act_state act_state, zone ZI150, fsm ZI151, zone *ZO152, fsm *ZO153)
{
	zone ZI152;
	fsm ZI153;

ZL2_154:;
	switch (CURRENT_TERMINAL) {
	case (TOK_PIPE):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI150, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 674 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 850 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: 154 */
			ZI151 = ZIq;
			goto ZL2_154;
			/* END OF INLINE: 154 */
		}
		/*UNREACHED*/
	default:
		{
			ZI152 = ZI150;
			ZI153 = ZI151;
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
	*ZO152 = ZI152;
	*ZO153 = ZI153;
}

static void
p_165(lex_state lex_state, act_state act_state, fsm ZI159, fsm *ZO164)
{
	fsm ZI164;

ZL2_165:;
	switch (CURRENT_TERMINAL) {
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
		{
			fsm ZIq;

			ZIq = ZI159;
			/* BEGINNING OF INLINE: 211 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CROSS):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-cross */
						{
#line 533 "src/lx/parser.act"

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
	
#line 942 "src/lx/parser.c"
						}
						/* END OF ACTION: op-cross */
						/* BEGINNING OF INLINE: 165 */
						ZI159 = ZIq;
						goto ZL2_165;
						/* END OF INLINE: 165 */
					}
					/*UNREACHED*/
				case (TOK_QMARK):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-qmark */
						{
#line 578 "src/lx/parser.act"

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
	
#line 1001 "src/lx/parser.c"
						}
						/* END OF ACTION: op-qmark */
						/* BEGINNING OF INLINE: 165 */
						ZI159 = ZIq;
						goto ZL2_165;
						/* END OF INLINE: 165 */
					}
					/*UNREACHED*/
				case (TOK_STAR):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-star */
						{
#line 483 "src/lx/parser.act"

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
	
#line 1065 "src/lx/parser.c"
						}
						/* END OF ACTION: op-star */
						/* BEGINNING OF INLINE: 165 */
						ZI159 = ZIq;
						goto ZL2_165;
						/* END OF INLINE: 165 */
					}
					/*UNREACHED*/
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 211 */
		}
		/*UNREACHED*/
	default:
		{
			ZI164 = ZI159;
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
	*ZO164 = ZI164;
}

static void
p_pattern_C_Cbody(lex_state lex_state, act_state act_state)
{
ZL2_pattern_C_Cbody:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF INLINE: 79 */
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
	
#line 1125 "src/lx/parser.c"
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
	
#line 1158 "src/lx/parser.c"
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

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			err(lex_state, "hex escape %s out of range: expected \\x0..\\x%x inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			err(lex_state, "%s: %s: expected \\x0..\\x%x inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (unsigned char) u;
	
#line 1195 "src/lx/parser.c"
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

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			err(lex_state, "octal escape %s out of range: expected \\0..\\%o inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			err(lex_state, "%s: %s: expected \\0..\\%o inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (unsigned char) u;
	
#line 1232 "src/lx/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 79 */
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 234 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 1250 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: pattern::body */
			goto ZL2_pattern_C_Cbody;
			/* END OF INLINE: pattern::body */
		}
		/*UNREACHED*/
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
p_170(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI168, fsm *ZOq)
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
#line 662 "src/lx/parser.act"

		assert((*ZI168) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_subtract((*ZI168), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_subtract");
			goto ZL1;
		}
	
#line 1298 "src/lx/parser.c"
			}
			/* END OF ACTION: op-subtract */
		}
		break;
	default:
		{
			ZIq = *ZI168;
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
p_173(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI171, fsm *ZOq)
{
	fsm ZIq;

	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG):
		{
			fsm ZIb;

			p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, *ZIz, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-concat */
			{
#line 645 "src/lx/parser.act"

		assert((*ZI171) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((*ZI171), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 1349 "src/lx/parser.c"
			}
			/* END OF ACTION: op-concat */
		}
		break;
	default:
		{
			ZIq = *ZI171;
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
p_176(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI174, fsm *ZOq)
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
#line 657 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 1394 "src/lx/parser.c"
			}
			/* END OF ACTION: op-product */
		}
		break;
	default:
		{
			ZIq = *ZI174;
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
p_179(lex_state lex_state, act_state act_state, fsm *ZOr)
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
	
#line 1484 "src/lx/parser.c"
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
	
#line 1508 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 349 "src/lx/parser.act"

		struct re_err err;
		const char *s;

		assert((ZIs) != NULL);

		s = (ZIs);

		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &s, (ZIf), &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADFORM);
			/* TODO: pass filename for .lx source */
			re_perror("re_new_comp", RE_SIMPLE, &err, NULL, (ZIs));
			exit(EXIT_FAILURE);
		}
	
#line 1530 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-regex */
			/* BEGINNING OF ACTION: free */
			{
#line 714 "src/lx/parser.act"

		free((ZIs));
	
#line 1539 "src/lx/parser.c"
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
	
#line 1569 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 332 "src/lx/parser.act"

		struct re_err err;
		const char *s;

		assert((ZIs) != NULL);

		s = (ZIs);

		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &s, 0, &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADFORM);
			/* TODO: pass filename for .lx source */
			re_perror("re_new_comp", RE_LITERAL, &err, NULL, (ZIs));
			exit(EXIT_FAILURE);
		}
	
#line 1591 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-literal */
			/* BEGINNING OF ACTION: free */
			{
#line 714 "src/lx/parser.act"

		free((ZIs));
	
#line 1600 "src/lx/parser.c"
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
p_expr_C_Cpostfix_Hexpr(lex_state lex_state, act_state act_state, zone ZI156, fsm *ZO164)
{
	fsm ZI164;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_expr_C_Cprimary_Hexpr (lex_state, act_state, ZI156, &ZIq);
		p_165 (lex_state, act_state, ZIq, &ZI164);
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
	*ZO164 = ZI164;
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
#line 636 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_reverse((ZIq))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 1669 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
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
#line 618 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complement((ZIq))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 1693 "src/lx/parser.c"
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
		p_106 (lex_state, act_state, &ZIt);
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
p_197(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BIND):
		{
			fsm ZIr;
			fsm ZIq;

			/* BEGINNING OF INLINE: 115 */
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
#line 732 "src/lx/parser.act"

		err_expected(lex_state, "'='");
	
#line 1800 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-bind */
				}
			ZL2:;
			}
			/* END OF INLINE: 115 */
			p_expr (lex_state, act_state, *ZIz, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

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
	
#line 1839 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_116 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 444 "src/lx/parser.act"

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
	
#line 1866 "src/lx/parser.c"
			}
			/* END OF ACTION: add-binding */
		}
		break;
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_SEMI): case (TOK_TO): case (TOK_MAP): case (TOK_LPAREN):
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK): case (TOK_TILDE):
	case (TOK_DASH): case (TOK_BANG): case (TOK_DOT): case (TOK_PIPE):
		{
			fsm ZI212;
			fsm ZI174;
			fsm ZI171;
			fsm ZI168;
			fsm ZIq;
			zone ZI213;
			fsm ZI214;
			string ZI215;
			fsm ZI216;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 258 "src/lx/parser.act"

		struct ast_zone *z;

		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);

		for (z = (*ZIz); z != NULL; z = z->parent) {
			(ZI212) = var_find(z->vl, (*ZIn));
			if ((ZI212) != NULL) {
				break;
			}

			if ((*ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (*ZIn));
				goto ZL1;
			}
		}

		(ZI212) = fsm_clone((ZI212));
		if ((ZI212) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 1916 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
			p_165 (lex_state, act_state, ZI212, &ZI174);
			p_176 (lex_state, act_state, ZIz, &ZI174, &ZI171);
			p_173 (lex_state, act_state, ZIz, &ZI171, &ZI168);
			p_170 (lex_state, act_state, ZIz, &ZI168, &ZIq);
			p_154 (lex_state, act_state, *ZIz, ZIq, &ZI213, &ZI214);
			p_106 (lex_state, act_state, &ZI215);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 679 "src/lx/parser.act"

		assert((ZI214) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI216) = (ZI214);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI216) = fsm_subtract((ZI214), (*ZIexit));
			if ((ZI216) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI216))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 1956 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_217 (lex_state, act_state, ZIa, ZIz, &ZI215, &ZI216);
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
p_217(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI215, fsm *ZI216)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_SEMI):
		{
			zone ZIto;

			p_116 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-zone */
			{
#line 474 "src/lx/parser.act"

		(ZIto) = NULL;
	
#line 2019 "src/lx/parser.c"
			}
			/* END OF ACTION: no-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 419 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI216) != NULL);

		if ((*ZI215) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI215));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI216), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2050 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 123 */
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
#line 740 "src/lx/parser.act"

		err_expected(lex_state, "'..'");
	
#line 2081 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 123 */
			p_token_Hmapping (lex_state, act_state, *ZIz, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 406 "src/lx/parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa), (*ZIz));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 2105 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 127 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_116 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-zone */
						{
#line 474 "src/lx/parser.act"

		(ZIx) = NULL;
	
#line 2130 "src/lx/parser.c"
						}
						/* END OF ACTION: no-zone */
						/* BEGINNING OF ACTION: no-token */
						{
#line 466 "src/lx/parser.act"

		(ZIy) = NULL;
	
#line 2139 "src/lx/parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 704 "src/lx/parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 2154 "src/lx/parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 377 "src/lx/parser.act"

		struct fsm_state *e;

		(ZIw) = re_new_empty();
		if ((ZIw) == NULL) {
			perror("re_new_empty");
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
	
#line 2182 "src/lx/parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 679 "src/lx/parser.act"

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
	
#line 2212 "src/lx/parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 419 "src/lx/parser.act"

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
	
#line 2243 "src/lx/parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						/* BEGINNING OF INLINE: 134 */
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
#line 744 "src/lx/parser.act"

		err_expected(lex_state, "'{'");
	
#line 2270 "src/lx/parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 134 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						/* BEGINNING OF INLINE: 135 */
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
#line 748 "src/lx/parser.act"

		err_expected(lex_state, "'}'");
	
#line 2302 "src/lx/parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 135 */
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
#line 760 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 2323 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 127 */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 419 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI216) != NULL);

		if ((*ZI215) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI215));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI216), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2358 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 419 "src/lx/parser.act"

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
	
#line 2389 "src/lx/parser.c"
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
#line 474 "src/lx/parser.act"

		(ZIparent) = NULL;
	
#line 2424 "src/lx/parser.c"
		}
		/* END OF ACTION: no-zone */
		/* BEGINNING OF ACTION: make-ast */
		{
#line 398 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 2437 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 406 "src/lx/parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa), (ZIparent));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 2452 "src/lx/parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 470 "src/lx/parser.act"

		(ZIexit) = NULL;
	
#line 2461 "src/lx/parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 459 "src/lx/parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 2473 "src/lx/parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 140 */
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
#line 752 "src/lx/parser.act"

		err_expected(lex_state, "EOF");
	
#line 2501 "src/lx/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 140 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 398 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 2522 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 720 "src/lx/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 2532 "src/lx/parser.c"
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
		fsm ZI171;

		p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, ZIz, &ZI171);
		p_173 (lex_state, act_state, &ZIz, &ZI171, &ZIq);
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
p_106(lex_state lex_state, act_state act_state, string *ZOt)
{
	string ZIt;

	switch (CURRENT_TERMINAL) {
	case (TOK_MAP):
		{
			/* BEGINNING OF INLINE: 107 */
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
#line 728 "src/lx/parser.act"

		err_expected(lex_state, "'->'");
	
#line 2598 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-map */
				}
			ZL2:;
			}
			/* END OF INLINE: 107 */
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
	
#line 2618 "src/lx/parser.c"
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
#line 466 "src/lx/parser.act"

		(ZIt) = NULL;
	
#line 2636 "src/lx/parser.c"
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
p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state lex_state, act_state act_state, zone ZI150, fsm *ZO153)
{
	fsm ZI153;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI152;

		p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI150, &ZIq);
		p_154 (lex_state, act_state, ZI150, ZIq, &ZI152, &ZI153);
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
	*ZO153 = ZI153;
}

static void
p_116(lex_state lex_state, act_state act_state)
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
#line 736 "src/lx/parser.act"

		err_expected(lex_state, "';'");
	
#line 2703 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI174;

		p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI174);
		p_176 (lex_state, act_state, &ZIz, &ZI174, &ZIq);
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
p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI168;

		p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, ZIz, &ZI168);
		p_170 (lex_state, act_state, &ZIz, &ZI168, &ZIq);
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

#line 805 "src/lx/parser.act"


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

#line 2807 "src/lx/parser.c"

/* END OF FILE */
