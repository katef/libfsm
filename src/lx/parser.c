/*
 * Automatically generated from the files:
 *	src/lx/parser.sid
 * and
 *	src/lx/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 129 "src/lx/parser.act"


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

	#include <re/re.h>

	#include "libfsm/internal.h"

	#include "lexer.h"
	#include "parser.h"
	#include "ast.h"
	#include "print.h"
	#include "var.h"

	struct arr {
		char *p;
		size_t len;
	};

	typedef struct arr *         arr;
	typedef char *               string;
	typedef enum re_flags        flags;
	typedef struct fsm *         fsm;
	typedef struct ast_zone *    zone;
	typedef struct ast_mapping * mapping;

	static unsigned int nmappings;

	struct act_state {
		enum lx_token lex_tok;
		enum lx_token lex_tok_save;
		unsigned int zn;
		const struct fsm_alloc *alloc;
	};

	struct lex_state {
		struct lx lx;
		struct lx_dynbuf buf;

		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   TOK_UNKNOWN
	#define ADVANCE_LEXER    do { act_state->lex_tok = lx_next(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static char *eof_token = "EOF"; /* morally const */

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

	static int
	act_agetc(void *opaque)
	{
		struct arr *a;

		a = opaque;

		assert(a != NULL);
		assert(a->p != NULL);

		if (a->len == 0) {
			return EOF;
		}

		return a->len--, (unsigned char) *a->p++;
	}

#line 116 "src/lx/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_255(lex_state, act_state, ast *, zone *, fsm *, string *);
static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_pattern(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_161(lex_state, act_state);
static void p_162(lex_state, act_state);
static void p_pattern_C_Cbody(lex_state, act_state);
static void p_expr_C_Cprimary_Hexpr(lex_state, act_state, zone, fsm *);
static void p_177(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_183(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_expr_C_Cpostfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cprefix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_189(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx(lex_state, act_state, ast, zone, zone, fsm *);
static void p_expr(lex_state, act_state, zone, fsm *);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_200(lex_state, act_state, fsm, fsm *);
static void p_expr_C_Cinfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings(lex_state, act_state, ast, zone, zone, fsm *);
static void p_207(lex_state, act_state, zone *, fsm *, fsm *);
static void p_211(lex_state, act_state, zone *, fsm *, fsm *);
static void p_215(lex_state, act_state, zone *, fsm *, fsm *);
static void p_218(lex_state, act_state, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx(lex_state, act_state, ast, zone, zone, fsm);
extern void p_lx(lex_state, act_state, ast *);
static void p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Czone_Hto_Hmapping(lex_state, act_state, ast, zone, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state, act_state, zone, fsm *);
static void p_114(lex_state, act_state, string *);
static void p_245(lex_state, act_state, ast *, zone *, fsm *, string *, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state, act_state, zone, fsm *);
static void p_124(lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_255(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BIND):
		{
			fsm ZIr;

			/* BEGINNING OF INLINE: 123 */
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
#line 831 "src/lx/parser.act"

		err_expected(lex_state, "'='");
	
#line 193 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-bind */
				}
			ZL2:;
			}
			/* END OF INLINE: 123 */
			p_expr (lex_state, act_state, *ZIz, &ZIr);
			p_124 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 524 "src/lx/parser.act"

		struct var *v;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((ZIr) != NULL);
		assert((*ZIn) != NULL);

		(void) (*ZIa);

		v = var_bind(&(*ZIz)->vl, (*ZIn), (ZIr));
		if (v == NULL) {
			perror("var_bind");
			goto ZL1;
		}
	
#line 225 "src/lx/parser.c"
			}
			/* END OF ACTION: add-binding */
		}
		break;
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_SEMI): case (TOK_TO): case (TOK_MAP): case (TOK_OPEN):
	case (TOK_LPAREN): case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
	case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT): case (TOK_DASH):
	case (TOK_DOT): case (TOK_PIPE): case (TOK_AND): case (TOK_COMMA):
		{
			fsm ZI276;
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI277;
			zone ZI278;
			fsm ZI279;
			zone ZI280;
			fsm ZI281;
			string ZI282;
			fsm ZI283;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 282 "src/lx/parser.act"

		struct ast_zone *z;

		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);

		for (z = (*ZIz); z != NULL; z = z->parent) {
			(ZI276) = var_find(z->vl, (*ZIn));
			if ((ZI276) != NULL) {
				break;
			}

			if ((*ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (*ZIn));
				goto ZL1;
			}
		}

		(ZI276) = fsm_clone((ZI276));
		if ((ZI276) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 278 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
			p_200 (lex_state, act_state, ZI276, &ZI213);
			p_215 (lex_state, act_state, ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, ZIz, &ZI205, &ZI277);
			p_189 (lex_state, act_state, *ZIz, ZI277, &ZI278, &ZI279);
			p_183 (lex_state, act_state, *ZIz, ZI279, &ZI280, &ZI281);
			p_114 (lex_state, act_state, &ZI282);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI281) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI283) = (ZI281);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI283) = fsm_subtract((ZI281), (*ZIexit));
			if ((ZI283) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 314 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, ZIa, ZIz, ZIexit, &ZI282, &ZI283);
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
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI171, zone ZI172, fsm ZI173)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI174;
		zone ZI175;
		fsm ZI176;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI171, ZI172, ZI173);
		p_177 (lex_state, act_state, ZI171, ZI172, ZI173, &ZI174, &ZI175, &ZI176);
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
#line 859 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 362 "src/lx/parser.c"
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
#line 230 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
	
#line 384 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-var */
			{
#line 282 "src/lx/parser.act"

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
	
#line 417 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
		}
		break;
	case (TOK_TOKEN):
		{
			string ZIt;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 226 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
	
#line 433 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 310 "src/lx/parser.act"

		const struct ast_mapping *m;
		fsm_state_t start;

		assert((ZIt) != NULL);
		assert((ZIt) != NULL);

		(ZIr) = fsm_new(act_state->alloc);
		if ((ZIr) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		if (!fsm_addstate((ZIr), &start)) {
			perror("fsm_addstate");
			goto ZL1;
		}

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			fsm_state_t ms;
			struct fsm_combine_info combine_info;

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

			(void) fsm_getstart(fsm, &ms);

			(ZIr) = fsm_merge((ZIr), fsm, &combine_info);
			if ((ZIr) == NULL) {
				perror("fsm_union");
				goto ZL1;
			}

			ms    += combine_info.base_b;
			start += combine_info.base_a;

			if (!fsm_addedge_epsilon((ZIr), start, ms)) {
				perror("fsm_addedge_epsilon");
				goto ZL1;
			}
		}

		fsm_setstart((ZIr), start);
	
#line 496 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			p_pattern_C_Cbody (lex_state, act_state);
			p_218 (lex_state, act_state, &ZIr);
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
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI230;
			zone ZI231;
			fsm ZI232;
			zone ZI233;
			fsm ZI234;
			string ZI235;
			fsm ZI236;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI213);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 720 "src/lx/parser.act"

		assert((ZI213) != NULL);

		if (!fsm_reverse((ZI213))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 559 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
			p_215 (lex_state, act_state, &ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, &ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, &ZIz, &ZI205, &ZI230);
			p_189 (lex_state, act_state, ZIz, ZI230, &ZI231, &ZI232);
			p_183 (lex_state, act_state, ZIz, ZI232, &ZI233, &ZI234);
			p_114 (lex_state, act_state, &ZI235);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI234) != NULL);

		if ((ZIexit) == NULL) {
			(ZI236) = (ZI234);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI236) = fsm_subtract((ZI234), (ZIexit));
			if ((ZI236) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 594 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI235, &ZI236);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HAT):
		{
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI238;
			zone ZI239;
			fsm ZI240;
			zone ZI241;
			fsm ZI242;
			string ZI243;
			fsm ZI244;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI213);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complete */
			{
#line 711 "src/lx/parser.act"

		assert((ZI213) != NULL);

		if (!fsm_complete((ZI213), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 634 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complete */
			p_215 (lex_state, act_state, &ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, &ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, &ZIz, &ZI205, &ZI238);
			p_189 (lex_state, act_state, ZIz, ZI238, &ZI239, &ZI240);
			p_183 (lex_state, act_state, ZIz, ZI240, &ZI241, &ZI242);
			p_114 (lex_state, act_state, &ZI243);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI242) != NULL);

		if ((ZIexit) == NULL) {
			(ZI244) = (ZI242);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI244) = fsm_subtract((ZI242), (ZIexit));
			if ((ZI244) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 669 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI243, &ZI244);
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
#line 230 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
	
#line 689 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_255 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_LPAREN):
		{
			fsm ZI246;
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI247;
			zone ZI248;
			fsm ZI249;
			zone ZI250;
			fsm ZI251;
			string ZI252;
			fsm ZI253;

			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZI246);
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
			p_200 (lex_state, act_state, ZI246, &ZI213);
			p_215 (lex_state, act_state, &ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, &ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, &ZIz, &ZI205, &ZI247);
			p_189 (lex_state, act_state, ZIz, ZI247, &ZI248, &ZI249);
			p_183 (lex_state, act_state, ZIz, ZI249, &ZI250, &ZI251);
			p_114 (lex_state, act_state, &ZI252);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI251) != NULL);

		if ((ZIexit) == NULL) {
			(ZI253) = (ZI251);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI253) = fsm_subtract((ZI251), (ZIexit));
			if ((ZI253) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 759 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI252, &ZI253);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TILDE):
		{
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI222;
			zone ZI223;
			fsm ZI224;
			zone ZI225;
			fsm ZI226;
			string ZI227;
			fsm ZI228;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI213);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 702 "src/lx/parser.act"

		assert((ZI213) != NULL);

		if (!fsm_complement((ZI213))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 799 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
			p_215 (lex_state, act_state, &ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, &ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, &ZIz, &ZI205, &ZI222);
			p_189 (lex_state, act_state, ZIz, ZI222, &ZI223, &ZI224);
			p_183 (lex_state, act_state, ZIz, ZI224, &ZI225, &ZI226);
			p_114 (lex_state, act_state, &ZI227);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI226) != NULL);

		if ((ZIexit) == NULL) {
			(ZI228) = (ZI226);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI228) = fsm_subtract((ZI226), (ZIexit));
			if ((ZI228) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 834 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI227, &ZI228);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TOKEN):
		{
			string ZI256;
			fsm ZI257;
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI258;
			zone ZI259;
			fsm ZI260;
			zone ZI261;
			fsm ZI262;
			string ZI263;
			fsm ZI264;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 226 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZI256 = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
	
#line 866 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 310 "src/lx/parser.act"

		const struct ast_mapping *m;
		fsm_state_t start;

		assert((ZI256) != NULL);
		assert((ZI256) != NULL);

		(ZI257) = fsm_new(act_state->alloc);
		if ((ZI257) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		if (!fsm_addstate((ZI257), &start)) {
			perror("fsm_addstate");
			goto ZL1;
		}

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			fsm_state_t ms;
			struct fsm_combine_info combine_info;

			if (m->token == NULL) {
				continue;
			}

			if (0 != strcmp(m->token->s, (ZI256))) {
				continue;
			}

			fsm = fsm_clone(m->fsm);
			if (fsm == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(void) fsm_getstart(fsm, &ms);

			(ZI257) = fsm_merge((ZI257), fsm, &combine_info);
			if ((ZI257) == NULL) {
				perror("fsm_union");
				goto ZL1;
			}

			ms    += combine_info.base_b;
			start += combine_info.base_a;

			if (!fsm_addedge_epsilon((ZI257), start, ms)) {
				perror("fsm_addedge_epsilon");
				goto ZL1;
			}
		}

		fsm_setstart((ZI257), start);
	
#line 929 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
			p_200 (lex_state, act_state, ZI257, &ZI213);
			p_215 (lex_state, act_state, &ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, &ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, &ZIz, &ZI205, &ZI258);
			p_189 (lex_state, act_state, ZIz, ZI258, &ZI259, &ZI260);
			p_183 (lex_state, act_state, ZIz, ZI260, &ZI261, &ZI262);
			p_114 (lex_state, act_state, &ZI263);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI262) != NULL);

		if ((ZIexit) == NULL) {
			(ZI264) = (ZI262);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI264) = fsm_subtract((ZI262), (ZIexit));
			if ((ZI264) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 965 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI263, &ZI264);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			fsm ZI266;
			fsm ZI213;
			fsm ZI209;
			fsm ZI205;
			fsm ZI267;
			zone ZI268;
			fsm ZI269;
			zone ZI270;
			fsm ZI271;
			string ZI272;
			fsm ZI273;

			p_pattern_C_Cbody (lex_state, act_state);
			p_218 (lex_state, act_state, &ZI266);
			p_200 (lex_state, act_state, ZI266, &ZI213);
			p_215 (lex_state, act_state, &ZIz, &ZI213, &ZI209);
			p_211 (lex_state, act_state, &ZIz, &ZI209, &ZI205);
			p_207 (lex_state, act_state, &ZIz, &ZI205, &ZI267);
			p_189 (lex_state, act_state, ZIz, ZI267, &ZI268, &ZI269);
			p_183 (lex_state, act_state, ZIz, ZI269, &ZI270, &ZI271);
			p_114 (lex_state, act_state, &ZI272);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZI271) != NULL);

		if ((ZIexit) == NULL) {
			(ZI273) = (ZI271);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI273) = fsm_subtract((ZI271), (ZIexit));
			if ((ZI273) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 1025 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_245 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI272, &ZI273);
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
#line 855 "src/lx/parser.act"

		err_expected(lex_state, "mapping, binding or zone");
	
#line 1049 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-thing */
	}
}

static void
p_161(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_OPEN):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-open */
		{
#line 843 "src/lx/parser.act"

		err_expected(lex_state, "'{'");
	
#line 1079 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-open */
	}
}

static void
p_162(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSE):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-close */
		{
#line 847 "src/lx/parser.act"

		err_expected(lex_state, "'}'");
	
#line 1109 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-close */
	}
}

static void
p_pattern_C_Cbody(lex_state lex_state, act_state act_state)
{
ZL2_pattern_C_Cbody:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF INLINE: 86 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 221 "src/lx/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 1138 "src/lx/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_ESC):
					{
						/* BEGINNING OF EXTRACT: ESC */
						{
#line 151 "src/lx/parser.act"

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
	
#line 1166 "src/lx/parser.c"
						}
						/* END OF EXTRACT: ESC */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						/* BEGINNING OF EXTRACT: HEX */
						{
#line 214 "src/lx/parser.act"

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
	
#line 1203 "src/lx/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						/* BEGINNING OF EXTRACT: OCT */
						{
#line 187 "src/lx/parser.act"

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
	
#line 1240 "src/lx/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 86 */
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 251 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 1258 "src/lx/parser.c"
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
p_177(lex_state lex_state, act_state act_state, ast ZI171, zone ZI172, fsm ZI173, ast *ZO174, zone *ZO175, fsm *ZO176)
{
	ast ZI174;
	zone ZI175;
	fsm ZI176;

ZL2_177:;
	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI171, ZI172, ZI173);
			/* BEGINNING OF INLINE: 177 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_177;
			}
			/* END OF INLINE: 177 */
		}
		/*UNREACHED*/
	default:
		{
			ZI174 = ZI171;
			ZI175 = ZI172;
			ZI176 = ZI173;
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
	*ZO175 = ZI175;
	*ZO176 = ZI176;
}

static void
p_183(lex_state lex_state, act_state act_state, zone ZI179, fsm ZI180, zone *ZO181, fsm *ZO182)
{
	zone ZI181;
	fsm ZI182;

ZL2_183:;
	switch (CURRENT_TERMINAL) {
	case (TOK_PIPE):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI179, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 768 "src/lx/parser.act"

		assert((ZI180) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_union((ZI180), (ZIb), NULL);
		if ((ZIq) == NULL) {
			perror("fsm_union");
			goto ZL1;
		}
	
#line 1398 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: 183 */
			ZI180 = ZIq;
			goto ZL2_183;
			/* END OF INLINE: 183 */
		}
		/*UNREACHED*/
	default:
		{
			ZI181 = ZI179;
			ZI182 = ZI180;
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
	*ZO181 = ZI181;
	*ZO182 = ZI182;
}

static void
p_expr_C_Cpostfix_Hexpr(lex_state lex_state, act_state act_state, zone ZI191, fsm *ZO199)
{
	fsm ZI199;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_expr_C_Cprimary_Hexpr (lex_state, act_state, ZI191, &ZIq);
		p_200 (lex_state, act_state, ZIq, &ZI199);
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
	*ZO199 = ZI199;
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
#line 720 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_reverse((ZIq))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 1476 "src/lx/parser.c"
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
#line 711 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complete((ZIq), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 1500 "src/lx/parser.c"
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
#line 702 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complement((ZIq))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 1524 "src/lx/parser.c"
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
p_189(lex_state lex_state, act_state act_state, zone ZI185, fsm ZI186, zone *ZO187, fsm *ZO188)
{
	zone ZI187;
	fsm ZI188;

ZL2_189:;
	switch (CURRENT_TERMINAL) {
	case (TOK_AND):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI185, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-intersect */
			{
#line 757 "src/lx/parser.act"

		assert((ZI186) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_intersect((ZI186), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_intersect");
			goto ZL1;
		}
	
#line 1585 "src/lx/parser.c"
			}
			/* END OF ACTION: op-intersect */
			/* BEGINNING OF INLINE: 189 */
			ZI186 = ZIq;
			goto ZL2_189;
			/* END OF INLINE: 189 */
		}
		/*UNREACHED*/
	default:
		{
			ZI187 = ZI185;
			ZI188 = ZI186;
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
	*ZO187 = ZI187;
	*ZO188 = ZI188;
}

static void
p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx(lex_state lex_state, act_state act_state, ast ZIa, zone ZIparent, zone ZIchild, fsm *ZIexit)
{
ZL2_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx:;
	switch (CURRENT_TERMINAL) {
	case (TOK_COMMA):
		{
			fsm ZIold_Hexit;
			fsm ZInew_Hexit;

			ADVANCE_LEXER;
			ZIold_Hexit = *ZIexit;
			p_list_Hof_Hthings_C_Czone_Hthing_C_Czone_Hto_Hmapping (lex_state, act_state, ZIa, ZIparent, ZIchild, &ZInew_Hexit);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 768 "src/lx/parser.act"

		assert((ZIold_Hexit) != NULL);
		assert((ZInew_Hexit) != NULL);

		(*ZIexit) = fsm_union((ZIold_Hexit), (ZInew_Hexit), NULL);
		if ((*ZIexit) == NULL) {
			perror("fsm_union");
			goto ZL1;
		}
	
#line 1642 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: list-of-things::zone-thing::list-of-zone-to-mappings::list-of-zone-to-mappings-x */
			goto ZL2_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx;
			/* END OF INLINE: list-of-things::zone-thing::list-of-zone-to-mappings::list-of-zone-to-mappings-x */
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
		p_114 (lex_state, act_state, &ZIt);
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
p_200(lex_state lex_state, act_state act_state, fsm ZI194, fsm *ZO199)
{
	fsm ZI199;

ZL2_200:;
	switch (CURRENT_TERMINAL) {
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
		{
			fsm ZIq;

			ZIq = ZI194;
			/* BEGINNING OF INLINE: 275 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CROSS):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-cross */
						{
#line 618 "src/lx/parser.act"

		fsm_state_t start, end;
		fsm_state_t old;
		fsm_state_t rs;

		/* TODO: centralise with libre */

		if (!fsm_addstate((ZIq), &start)) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		if (!fsm_addstate((ZIq), &end)) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		if (!fsm_collate((ZIq), &old, fsm_isend)) {
			perror("fsm_collate");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), old, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setend((ZIq), old, 0);
		fsm_setend((ZIq), end, 1);

		(void) fsm_getstart((ZIq), &rs);

		if (!fsm_addedge_epsilon((ZIq), start, rs)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), end, start)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setstart((ZIq), start);
	
#line 1775 "src/lx/parser.c"
						}
						/* END OF ACTION: op-cross */
						/* BEGINNING OF INLINE: 200 */
						ZI194 = ZIq;
						goto ZL2_200;
						/* END OF INLINE: 200 */
					}
					/*UNREACHED*/
				case (TOK_QMARK):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-qmark */
						{
#line 663 "src/lx/parser.act"

		fsm_state_t start, end;
		fsm_state_t old;
		fsm_state_t rs;

		/* TODO: centralise with libre */

		if (!fsm_addstate((ZIq), &start)) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		if (!fsm_addstate((ZIq), &end)) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		if (!fsm_collate((ZIq), &old, fsm_isend)) {
			perror("fsm_collate");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), old, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setend((ZIq), old, 0);
		fsm_setend((ZIq), end, 1);

		(void) fsm_getstart((ZIq), &rs);

		if (!fsm_addedge_epsilon((ZIq), start, rs)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), start, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setstart((ZIq), start);
	
#line 1834 "src/lx/parser.c"
						}
						/* END OF ACTION: op-qmark */
						/* BEGINNING OF INLINE: 200 */
						ZI194 = ZIq;
						goto ZL2_200;
						/* END OF INLINE: 200 */
					}
					/*UNREACHED*/
				case (TOK_STAR):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-star */
						{
#line 568 "src/lx/parser.act"

		fsm_state_t start, end;
		fsm_state_t old;
		fsm_state_t rs;

		/* TODO: centralise with libre */

		if (!fsm_addstate((ZIq), &start)) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		if (!fsm_addstate((ZIq), &end)) {
			perror("fsm_addtstate");
			goto ZL1;
		}

		if (!fsm_collate((ZIq), &old, fsm_isend)) {
			perror("fsm_collate");
			goto ZL1;
		}

		if (!fsm_addedge_epsilon((ZIq), old, end)) {
			perror("fsm_addedge_epsilon");
			goto ZL1;
		}

		fsm_setend((ZIq), old, 0);
		fsm_setend((ZIq), end, 1);

		(void) fsm_getstart((ZIq), &rs);

		if (!fsm_addedge_epsilon((ZIq), start, rs)) {
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
	
#line 1898 "src/lx/parser.c"
						}
						/* END OF ACTION: op-star */
						/* BEGINNING OF INLINE: 200 */
						ZI194 = ZIq;
						goto ZL2_200;
						/* END OF INLINE: 200 */
					}
					/*UNREACHED*/
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 275 */
		}
		/*UNREACHED*/
	default:
		{
			ZI199 = ZI194;
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
	*ZO199 = ZI199;
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
p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings(lex_state lex_state, act_state act_state, ast ZIa, zone ZIparent, zone ZIchild, fsm *ZOexit)
{
	fsm ZIexit;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_list_Hof_Hthings_C_Czone_Hthing_C_Czone_Hto_Hmapping (lex_state, act_state, ZIa, ZIparent, ZIchild, &ZIexit);
		p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx (lex_state, act_state, ZIa, ZIparent, ZIchild, &ZIexit);
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
	*ZOexit = ZIexit;
}

static void
p_207(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI205, fsm *ZOq)
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
#line 746 "src/lx/parser.act"

		assert((*ZI205) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_subtract((*ZI205), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_subtract");
			goto ZL1;
		}
	
#line 2006 "src/lx/parser.c"
			}
			/* END OF ACTION: op-subtract */
		}
		break;
	default:
		{
			ZIq = *ZI205;
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
p_211(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI209, fsm *ZOq)
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
#line 729 "src/lx/parser.act"

		assert((*ZI209) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((*ZI209), (ZIb), NULL);
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 2057 "src/lx/parser.c"
			}
			/* END OF ACTION: op-concat */
		}
		break;
	default:
		{
			ZIq = *ZI209;
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
p_215(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI213, fsm *ZOq)
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
#line 741 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 2102 "src/lx/parser.c"
			}
			/* END OF ACTION: op-product */
		}
		break;
	default:
		{
			ZIq = *ZI213;
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
p_218(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RE):
		{
			flags ZIf;
			arr ZIa;

			/* BEGINNING OF EXTRACT: RE */
			{
#line 238 "src/lx/parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */

		if (-1 == re_flags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			err(lex_state, "/%s: Unrecognised regexp flags", lex_state->buf.a + 1);
			exit(EXIT_FAILURE);
		}

		ZIf |= RE_ANCHORED; /* regexps in lx are implicitly anchored */
		ZIf |= RE_SINGLE;   /* we can't assume we're lexing text */
	
#line 2150 "src/lx/parser.c"
			}
			/* END OF EXTRACT: RE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 265 "src/lx/parser.act"

		size_t len;

		/*
		 * Note we copy the string here because the grammar permits adjacent
		 * patterns; the pattern buffer will be overwritten by the LL(1) one-token
		 * lookahead.
		 */

		len = lex_state->p - lex_state->a;

		(ZIa) = malloc(sizeof *(ZIa) + len);
		if ((ZIa) == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		(ZIa)->len = len;
		(ZIa)->p   = (char *) (ZIa) + sizeof *(ZIa);

		memcpy((ZIa)->p, lex_state->a, (ZIa)->len);

		lex_state->p = lex_state->a;
	
#line 2181 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 381 "src/lx/parser.act"

		struct re_err err;

		assert((ZIa) != NULL);
		assert((ZIa)->p != NULL);

		(ZIr) = re_comp(RE_NATIVE, act_agetc, (ZIa), act_state->alloc, (ZIf), &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_NATIVE, &err, NULL, (ZIa)->p);
			exit(EXIT_FAILURE);
		}
	
#line 2201 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-regex */
			/* BEGINNING OF ACTION: free-arr */
			{
#line 809 "src/lx/parser.act"

		free((ZIa));
	
#line 2210 "src/lx/parser.c"
			}
			/* END OF ACTION: free-arr */
		}
		break;
	case (TOK_STR):
		{
			arr ZIa;

			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 265 "src/lx/parser.act"

		size_t len;

		/*
		 * Note we copy the string here because the grammar permits adjacent
		 * patterns; the pattern buffer will be overwritten by the LL(1) one-token
		 * lookahead.
		 */

		len = lex_state->p - lex_state->a;

		(ZIa) = malloc(sizeof *(ZIa) + len);
		if ((ZIa) == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		(ZIa)->len = len;
		(ZIa)->p   = (char *) (ZIa) + sizeof *(ZIa);

		memcpy((ZIa)->p, lex_state->a, (ZIa)->len);

		lex_state->p = lex_state->a;
	
#line 2247 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 366 "src/lx/parser.act"

		struct re_err err;

		assert((ZIa) != NULL);
		assert((ZIa)->p != NULL);

		(ZIr) = re_comp(RE_LITERAL, act_agetc, (ZIa), act_state->alloc, 0, &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_LITERAL, &err, NULL, (ZIa)->p); /* XXX */
			exit(EXIT_FAILURE);
		}
	
#line 2267 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-literal */
			/* BEGINNING OF ACTION: free-arr */
			{
#line 809 "src/lx/parser.act"

		free((ZIa));
	
#line 2276 "src/lx/parser.c"
			}
			/* END OF ACTION: free-arr */
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
p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state lex_state, act_state act_state, zone ZI185, fsm *ZO188)
{
	fsm ZI188;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI187;

		p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI185, &ZIq);
		p_189 (lex_state, act_state, ZI185, ZIq, &ZI187, &ZI188);
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
	*ZO188 = ZI188;
}

static void
p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx(lex_state lex_state, act_state act_state, ast ZIa, zone ZIparent, zone ZIchild, fsm ZIexit)
{
ZL2_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx:;
	switch (CURRENT_TERMINAL) {
	case (TOK_COMMA):
		{
			fsm ZIr;
			string ZIt;
			fsm ZIq;

			ADVANCE_LEXER;
			p_token_Hmapping (lex_state, act_state, ZIparent, &ZIr, &ZIt);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 779 "src/lx/parser.act"

		assert((ZIr) != NULL);

		if ((ZIexit) == NULL) {
			(ZIq) = (ZIr);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZIq) = fsm_subtract((ZIr), (ZIexit));
			if ((ZIq) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 2360 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 487 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((ZIa) != NULL);
		assert((ZIparent) != NULL);
		assert((ZIparent) != (ZIchild));
		assert((ZIq) != NULL);

		if ((ZIt) != NULL) {
			t = ast_addtoken((ZIa), (ZIt));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIparent), (ZIq), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((ZIt) == eof_token) {
			(ZIa)->eof_mapping = m;
		}
	
#line 2403 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: list-of-things::zone-thing::list-of-zone-from-mappings::list-of-zone-from-mappings-x */
			goto ZL2_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx;
			/* END OF INLINE: list-of-things::zone-thing::list-of-zone-from-mappings::list-of-zone-from-mappings-x */
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
		string ZIeof;
		fsm ZIr;

		/* BEGINNING OF ACTION: no-zone */
		{
#line 558 "src/lx/parser.act"

		(ZIparent) = NULL;
	
#line 2443 "src/lx/parser.c"
		}
		/* END OF ACTION: no-zone */
		/* BEGINNING OF ACTION: make-ast */
		{
#line 450 "src/lx/parser.act"

		const char *builtins[] = { "EOF", "UNKNOWN", "ERROR" };
		struct ast_token *t;
		size_t i;

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}

		for (i = 0; i < sizeof builtins / sizeof *builtins; i++) {
			t = ast_addtoken((ZIa), builtins[i]);
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		}
	
#line 2468 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 466 "src/lx/parser.act"

		assert((ZIa) != NULL);

		if (print_progress) {
			nmappings = 0;
			if (important(act_state->zn)) {
				fprintf(stderr, " z%u", act_state->zn);
			}
			act_state->zn++;
		}

		(ZIz) = ast_addzone((ZIa), (ZIparent));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 2491 "src/lx/parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 554 "src/lx/parser.act"

		(ZIexit) = NULL;
	
#line 2500 "src/lx/parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 539 "src/lx/parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 2512 "src/lx/parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 167 */
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
#line 851 "src/lx/parser.act"

		err_expected(lex_state, "EOF");
	
#line 2540 "src/lx/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 167 */
		/* BEGINNING OF ACTION: eof-token */
		{
#line 546 "src/lx/parser.act"

		(ZIeof) = eof_token;
	
#line 2553 "src/lx/parser.c"
		}
		/* END OF ACTION: eof-token */
		/* BEGINNING OF ACTION: regex-empty */
		{
#line 429 "src/lx/parser.act"

		fsm_state_t state;

		(ZIr) = fsm_new(act_state->alloc);
		if ((ZIr) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		if (!fsm_addstate((ZIr), &state)) {
			perror("fsm_addstate");
			fsm_free((ZIr));
			goto ZL1;
		}

		fsm_setstart((ZIr), state);
		fsm_setend((ZIr), state, 1);
	
#line 2577 "src/lx/parser.c"
		}
		/* END OF ACTION: regex-empty */
		/* BEGINNING OF ACTION: add-mapping */
		{
#line 487 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);
		assert((ZIz) != (ZIparent));
		assert((ZIr) != NULL);

		if ((ZIeof) != NULL) {
			t = ast_addtoken((ZIa), (ZIeof));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIz), (ZIr), t, (ZIparent));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((ZIeof) == eof_token) {
			(ZIa)->eof_mapping = m;
		}
	
#line 2620 "src/lx/parser.c"
		}
		/* END OF ACTION: add-mapping */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 450 "src/lx/parser.act"

		const char *builtins[] = { "EOF", "UNKNOWN", "ERROR" };
		struct ast_token *t;
		size_t i;

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}

		for (i = 0; i < sizeof builtins / sizeof *builtins; i++) {
			t = ast_addtoken((ZIa), builtins[i]);
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL4;
			}
		}
	
#line 2649 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 819 "src/lx/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 2659 "src/lx/parser.c"
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
		fsm ZI209;

		p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, ZIz, &ZI209);
		p_211 (lex_state, act_state, &ZIz, &ZI209, &ZIq);
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
p_list_Hof_Hthings_C_Czone_Hthing_C_Czone_Hto_Hmapping(lex_state lex_state, act_state act_state, ast ZIa, zone ZIparent, zone ZIchild, fsm *ZOexit)
{
	fsm ZIexit;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIr;
		string ZIt;

		p_token_Hmapping (lex_state, act_state, ZIparent, &ZIr, &ZIt);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: add-mapping */
		{
#line 487 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((ZIa) != NULL);
		assert((ZIchild) != NULL);
		assert((ZIchild) != (ZIparent));
		assert((ZIr) != NULL);

		if ((ZIt) != NULL) {
			t = ast_addtoken((ZIa), (ZIt));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIchild), (ZIr), t, (ZIparent));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((ZIt) == eof_token) {
			(ZIa)->eof_mapping = m;
		}
	
#line 2754 "src/lx/parser.c"
		}
		/* END OF ACTION: add-mapping */
		/* BEGINNING OF ACTION: clone */
		{
#line 799 "src/lx/parser.act"

		assert((ZIr) != NULL);

		(ZIexit) = fsm_clone((ZIr));
		if ((ZIexit) == NULL) {
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 2769 "src/lx/parser.c"
		}
		/* END OF ACTION: clone */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOexit = ZIexit;
}

static void
p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state lex_state, act_state act_state, zone ZI179, fsm *ZO182)
{
	fsm ZI182;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI181;

		p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI179, &ZIq);
		p_183 (lex_state, act_state, ZI179, ZIq, &ZI181, &ZI182);
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
	*ZO182 = ZI182;
}

static void
p_114(lex_state lex_state, act_state act_state, string *ZOt)
{
	string ZIt;

	switch (CURRENT_TERMINAL) {
	case (TOK_MAP):
		{
			/* BEGINNING OF INLINE: 115 */
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
#line 827 "src/lx/parser.act"

		err_expected(lex_state, "'->'");
	
#line 2836 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-map */
				}
			ZL2:;
			}
			/* END OF INLINE: 115 */
			switch (CURRENT_TERMINAL) {
			case (TOK_TOKEN):
				/* BEGINNING OF EXTRACT: TOKEN */
				{
#line 226 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
	
#line 2852 "src/lx/parser.c"
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
#line 550 "src/lx/parser.act"

		(ZIt) = NULL;
	
#line 2870 "src/lx/parser.c"
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
p_245(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZI243, fsm *ZI244)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_SEMI):
		{
			zone ZIto;

			p_124 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-zone */
			{
#line 558 "src/lx/parser.act"

		(ZIto) = NULL;
	
#line 2905 "src/lx/parser.c"
			}
			/* END OF ACTION: no-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 487 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI244) != NULL);

		if ((*ZI243) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI243));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI244), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((*ZI243) == eof_token) {
			(*ZIa)->eof_mapping = m;
		}
	
#line 2948 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_TO): case (TOK_COMMA):
		{
			zone ZIchild;
			fsm ZIchild_Hexit;

			/* BEGINNING OF ACTION: make-zone */
			{
#line 466 "src/lx/parser.act"

		assert((*ZIa) != NULL);

		if (print_progress) {
			nmappings = 0;
			if (important(act_state->zn)) {
				fprintf(stderr, " z%u", act_state->zn);
			}
			act_state->zn++;
		}

		(ZIchild) = ast_addzone((*ZIa), (*ZIz));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 2978 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 487 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI244) != NULL);

		if ((*ZI243) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI243));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI244), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((*ZI243) == eof_token) {
			(*ZIa)->eof_mapping = m;
		}
	
#line 3021 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx (lex_state, act_state, *ZIa, *ZIz, ZIchild, *ZIexit);
			/* BEGINNING OF INLINE: 153 */
			{
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
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
#line 839 "src/lx/parser.act"

		err_expected(lex_state, "'..'");
	
#line 3049 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 153 */
			p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings (lex_state, act_state, *ZIa, *ZIz, ZIchild, &ZIchild_Hexit);
			/* BEGINNING OF INLINE: 155 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIw;
						fsm ZIv;

						p_124 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-zone */
						{
#line 558 "src/lx/parser.act"

		(ZIx) = NULL;
	
#line 3078 "src/lx/parser.c"
						}
						/* END OF ACTION: no-zone */
						/* BEGINNING OF ACTION: no-token */
						{
#line 550 "src/lx/parser.act"

		(ZIy) = NULL;
	
#line 3087 "src/lx/parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 397 "src/lx/parser.act"

		fsm_state_t start, end;

		(ZIw) = fsm_new(act_state->alloc);
		if ((ZIw) == NULL) {
			perror("fsm_new");
			goto ZL5;
		}

		if (!fsm_addstate((ZIw), &start)) {
			perror("fsm_addstate");
			fsm_free((ZIw));
			goto ZL5;
		}

		if (!fsm_addstate((ZIw), &end)) {
			perror("fsm_addstate");
			fsm_free((ZIw));
			goto ZL5;
		}

		if (!fsm_addedge_any((ZIw), start, end)) {
			perror("fsm_addedge_any");
			fsm_free((ZIw));
			goto ZL5;
		}

		fsm_setstart((ZIw), start);
		fsm_setend((ZIw), end, 1);
	
#line 3123 "src/lx/parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 779 "src/lx/parser.act"

		assert((ZIw) != NULL);

		if ((ZIchild_Hexit) == NULL) {
			(ZIv) = (ZIw);
		} else {
			(ZIchild_Hexit) = fsm_clone((ZIchild_Hexit));
			if ((ZIchild_Hexit) == NULL) {
				perror("fsm_clone");
				goto ZL5;
			}

			(ZIv) = fsm_subtract((ZIw), (ZIchild_Hexit));
			if ((ZIv) == NULL) {
				perror("fsm_subtract");
				goto ZL5;
			}
		}
	
#line 3148 "src/lx/parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 487 "src/lx/parser.act"

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

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((ZIy) == eof_token) {
			(*ZIa)->eof_mapping = m;
		}
	
#line 3191 "src/lx/parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						p_161 (lex_state, act_state);
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIchild_Hexit);
						p_162 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
					}
					break;
				case (ERROR_TERMINAL):
					RESTORE_LEXER;
					goto ZL5;
				default:
					goto ZL5;
				}
				goto ZL4;
			ZL5:;
				{
					/* BEGINNING OF ACTION: err-expected-list */
					{
#line 859 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3222 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 155 */
		}
		break;
	case (TOK_OPEN):
		{
			fsm ZIr2;
			zone ZIchild;

			/* BEGINNING OF ACTION: no-exit */
			{
#line 554 "src/lx/parser.act"

		(ZIr2) = NULL;
	
#line 3242 "src/lx/parser.c"
			}
			/* END OF ACTION: no-exit */
			/* BEGINNING OF ACTION: make-zone */
			{
#line 466 "src/lx/parser.act"

		assert((*ZIa) != NULL);

		if (print_progress) {
			nmappings = 0;
			if (important(act_state->zn)) {
				fprintf(stderr, " z%u", act_state->zn);
			}
			act_state->zn++;
		}

		(ZIchild) = ast_addzone((*ZIa), (*ZIz));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 3265 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 487 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI244) != NULL);

		if ((*ZI243) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI243));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI244), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}

		/* purposefully shallow pointer comparison */
		if ((*ZI243) == eof_token) {
			(*ZIa)->eof_mapping = m;
		}
	
#line 3308 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: 133 */
			{
				{
					p_161 (lex_state, act_state);
					p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
					p_162 (lex_state, act_state);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL7;
					}
				}
				goto ZL6;
			ZL7:;
				{
					/* BEGINNING OF ACTION: err-expected-list */
					{
#line 859 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3331 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL6:;
			}
			/* END OF INLINE: 133 */
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
		fsm ZI213;

		p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI213);
		p_215 (lex_state, act_state, &ZIz, &ZI213, &ZIq);
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
		fsm ZI205;

		p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, ZIz, &ZI205);
		p_207 (lex_state, act_state, &ZIz, &ZI205, &ZIq);
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
p_124(lex_state lex_state, act_state act_state)
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
#line 835 "src/lx/parser.act"

		err_expected(lex_state, "';'");
	
#line 3427 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

/* BEGINNING OF TRAILER */

#line 923 "src/lx/parser.act"


	struct ast *lx_parse(FILE *f, const struct fsm_alloc *alloc) {
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

		lx->lgetc       = lx_fgetc;
		lx->getc_opaque = f;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		lx->buf_opaque = &lex_state->buf;
		lx->push       = lx_dynpush;
		lx->clear      = lx_dynclear;
		lx->free       = lx_dynfree;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->alloc = alloc;

		if (print_progress) {
			act_state->zn = 0;
		}

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state, &ast);

		lx->free(lx->buf_opaque);

		/*
		 * This can happen when the first token is TOK_UNKNOWN.
		 * SID's generated parser bails out immediately.
		 * So we never reach <make-ast>, and never <err-syntax> about it.
		 *
		 * Really I wanted this handled along with the usual syntax error
		 * case, from ## alts inside parser.sid, instead of reproducing
		 * the same error here.
		 */
		if (ast == NULL) {
			err(lex_state, "Syntax error");
			exit(EXIT_FAILURE);
		}

		return ast;
	}

#line 3498 "src/lx/parser.c"

/* END OF FILE */
