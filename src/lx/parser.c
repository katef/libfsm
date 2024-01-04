/*
 * Automatically generated from the files:
 *	src/lx/parser.sid
 * and
 *	src/lx/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 27 "src/lx/parser.act"


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
	#include <fsm/options.h>

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
		const struct fsm_options *opt;
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

#line 115 "src/lx/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_pattern(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_159(lex_state, act_state);
static void p_160(lex_state, act_state);
static void p_pattern_C_Cbody(lex_state, act_state);
static void p_174(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_expr_C_Cprimary_Hexpr(lex_state, act_state, zone, fsm *);
static void p_180(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_expr_C_Cpostfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cprefix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_186(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx(lex_state, act_state, ast, zone, zone, fsm *);
static void p_expr(lex_state, act_state, zone, fsm *);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_197(lex_state, act_state, fsm, fsm *);
static void p_expr_C_Cinfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_204(lex_state, act_state, zone *, fsm *, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings(lex_state, act_state, ast, zone, zone, fsm *);
static void p_208(lex_state, act_state, zone *, fsm *, fsm *);
static void p_212(lex_state, act_state, zone *, fsm *, fsm *);
static void p_215(lex_state, act_state, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx(lex_state, act_state, ast, zone, zone, fsm);
extern void p_lx(lex_state, act_state, ast *);
static void p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state, act_state, zone, fsm *);
static void p_list_Hof_Hthings_C_Czone_Hthing_C_Czone_Hto_Hmapping(lex_state, act_state, ast, zone, zone, fsm *);
static void p_112(lex_state, act_state, string *);
static void p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state, act_state, zone, fsm *);
static void p_122(lex_state, act_state);
static void p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state, act_state, zone, fsm *);
static void p_251(lex_state, act_state, ast *, zone *, fsm *, string *, fsm *);
static void p_252(lex_state, act_state, ast *, zone *, fsm *, string *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI168, zone ZI169, fsm ZI170)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI171;
		zone ZI172;
		fsm ZI173;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI168, ZI169, ZI170);
		p_174 (lex_state, act_state, ZI168, ZI169, ZI170, &ZI171, &ZI172, &ZI173);
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
#line 823 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 191 "src/lx/parser.c"
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
#line 232 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 217 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-var */
			{
#line 286 "src/lx/parser.act"

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
	
#line 250 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
		}
		break;
	case (TOK_TOKEN):
		{
			string ZIt;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 223 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 270 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 313 "src/lx/parser.act"

		const struct ast_mapping *m;
		fsm_state_t start;

		assert((ZIt) != NULL);
		assert((ZIt) != NULL);

		(ZIr) = fsm_new(act_state->opt);
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
	
#line 333 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			p_pattern_C_Cbody (lex_state, act_state);
			p_215 (lex_state, act_state, &ZIr);
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
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI227;
			zone ZI228;
			fsm ZI229;
			zone ZI230;
			fsm ZI231;
			string ZI232;
			fsm ZI233;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI210);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 685 "src/lx/parser.act"

		assert((ZI210) != NULL);

		if (!fsm_reverse((ZI210))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 396 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
			p_212 (lex_state, act_state, &ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, &ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, &ZIz, &ZI202, &ZI227);
			p_186 (lex_state, act_state, ZIz, ZI227, &ZI228, &ZI229);
			p_180 (lex_state, act_state, ZIz, ZI229, &ZI230, &ZI231);
			p_112 (lex_state, act_state, &ZI232);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

		assert((ZI231) != NULL);

		if ((ZIexit) == NULL) {
			(ZI233) = (ZI231);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI233) = fsm_subtract((ZI231), (ZIexit));
			if ((ZI233) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 431 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI232, &ZI233);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HAT):
		{
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI235;
			zone ZI236;
			fsm ZI237;
			zone ZI238;
			fsm ZI239;
			string ZI240;
			fsm ZI241;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI210);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complete */
			{
#line 676 "src/lx/parser.act"

		assert((ZI210) != NULL);

		if (!fsm_complete((ZI210), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 471 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complete */
			p_212 (lex_state, act_state, &ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, &ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, &ZIz, &ZI202, &ZI235);
			p_186 (lex_state, act_state, ZIz, ZI235, &ZI236, &ZI237);
			p_180 (lex_state, act_state, ZIz, ZI237, &ZI238, &ZI239);
			p_112 (lex_state, act_state, &ZI240);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

		assert((ZI239) != NULL);

		if ((ZIexit) == NULL) {
			(ZI241) = (ZI239);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI241) = fsm_subtract((ZI239), (ZIexit));
			if ((ZI241) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 506 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI240, &ZI241);
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
#line 232 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 530 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_252 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_LPAREN):
		{
			fsm ZI243;
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI244;
			zone ZI245;
			fsm ZI246;
			zone ZI247;
			fsm ZI248;
			string ZI249;
			fsm ZI250;

			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZI243);
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
			p_197 (lex_state, act_state, ZI243, &ZI210);
			p_212 (lex_state, act_state, &ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, &ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, &ZIz, &ZI202, &ZI244);
			p_186 (lex_state, act_state, ZIz, ZI244, &ZI245, &ZI246);
			p_180 (lex_state, act_state, ZIz, ZI246, &ZI247, &ZI248);
			p_112 (lex_state, act_state, &ZI249);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

		assert((ZI248) != NULL);

		if ((ZIexit) == NULL) {
			(ZI250) = (ZI248);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI250) = fsm_subtract((ZI248), (ZIexit));
			if ((ZI250) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 600 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI249, &ZI250);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TILDE):
		{
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI219;
			zone ZI220;
			fsm ZI221;
			zone ZI222;
			fsm ZI223;
			string ZI224;
			fsm ZI225;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI210);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 667 "src/lx/parser.act"

		assert((ZI210) != NULL);

		if (!fsm_complement((ZI210))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 640 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
			p_212 (lex_state, act_state, &ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, &ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, &ZIz, &ZI202, &ZI219);
			p_186 (lex_state, act_state, ZIz, ZI219, &ZI220, &ZI221);
			p_180 (lex_state, act_state, ZIz, ZI221, &ZI222, &ZI223);
			p_112 (lex_state, act_state, &ZI224);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

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
		}
	
#line 675 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI224, &ZI225);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TOKEN):
		{
			string ZI253;
			fsm ZI254;
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI255;
			zone ZI256;
			fsm ZI257;
			zone ZI258;
			fsm ZI259;
			string ZI260;
			fsm ZI261;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 223 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZI253 = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZI253 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 711 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 313 "src/lx/parser.act"

		const struct ast_mapping *m;
		fsm_state_t start;

		assert((ZI253) != NULL);
		assert((ZI253) != NULL);

		(ZI254) = fsm_new(act_state->opt);
		if ((ZI254) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		if (!fsm_addstate((ZI254), &start)) {
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

			if (0 != strcmp(m->token->s, (ZI253))) {
				continue;
			}

			fsm = fsm_clone(m->fsm);
			if (fsm == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(void) fsm_getstart(fsm, &ms);

			(ZI254) = fsm_merge((ZI254), fsm, &combine_info);
			if ((ZI254) == NULL) {
				perror("fsm_union");
				goto ZL1;
			}

			ms    += combine_info.base_b;
			start += combine_info.base_a;

			if (!fsm_addedge_epsilon((ZI254), start, ms)) {
				perror("fsm_addedge_epsilon");
				goto ZL1;
			}
		}

		fsm_setstart((ZI254), start);
	
#line 774 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
			p_197 (lex_state, act_state, ZI254, &ZI210);
			p_212 (lex_state, act_state, &ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, &ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, &ZIz, &ZI202, &ZI255);
			p_186 (lex_state, act_state, ZIz, ZI255, &ZI256, &ZI257);
			p_180 (lex_state, act_state, ZIz, ZI257, &ZI258, &ZI259);
			p_112 (lex_state, act_state, &ZI260);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

		assert((ZI259) != NULL);

		if ((ZIexit) == NULL) {
			(ZI261) = (ZI259);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI261) = fsm_subtract((ZI259), (ZIexit));
			if ((ZI261) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 810 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI260, &ZI261);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			fsm ZI263;
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI264;
			zone ZI265;
			fsm ZI266;
			zone ZI267;
			fsm ZI268;
			string ZI269;
			fsm ZI270;

			p_pattern_C_Cbody (lex_state, act_state);
			p_215 (lex_state, act_state, &ZI263);
			p_197 (lex_state, act_state, ZI263, &ZI210);
			p_212 (lex_state, act_state, &ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, &ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, &ZIz, &ZI202, &ZI264);
			p_186 (lex_state, act_state, ZIz, ZI264, &ZI265, &ZI266);
			p_180 (lex_state, act_state, ZIz, ZI266, &ZI267, &ZI268);
			p_112 (lex_state, act_state, &ZI269);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

		assert((ZI268) != NULL);

		if ((ZIexit) == NULL) {
			(ZI270) = (ZI268);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI270) = fsm_subtract((ZI268), (ZIexit));
			if ((ZI270) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 870 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZI269, &ZI270);
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
#line 819 "src/lx/parser.act"

		err_expected(lex_state, "mapping, binding or zone");
	
#line 894 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-thing */
	}
}

static void
p_159(lex_state lex_state, act_state act_state)
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
#line 807 "src/lx/parser.act"

		err_expected(lex_state, "'{'");
	
#line 924 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-open */
	}
}

static void
p_160(lex_state lex_state, act_state act_state)
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
#line 811 "src/lx/parser.act"

		err_expected(lex_state, "'}'");
	
#line 954 "src/lx/parser.c"
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

			/* BEGINNING OF INLINE: 84 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 216 "src/lx/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 983 "src/lx/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_ESC):
					{
						/* BEGINNING OF EXTRACT: ESC */
						{
#line 144 "src/lx/parser.act"

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
	
#line 1011 "src/lx/parser.c"
						}
						/* END OF EXTRACT: ESC */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						/* BEGINNING OF EXTRACT: HEX */
						{
#line 189 "src/lx/parser.act"

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
	
#line 1048 "src/lx/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						/* BEGINNING OF EXTRACT: OCT */
						{
#line 162 "src/lx/parser.act"

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
	
#line 1085 "src/lx/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 84 */
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 256 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 1103 "src/lx/parser.c"
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
p_174(lex_state lex_state, act_state act_state, ast ZI168, zone ZI169, fsm ZI170, ast *ZO171, zone *ZO172, fsm *ZO173)
{
	ast ZI171;
	zone ZI172;
	fsm ZI173;

ZL2_174:;
	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI168, ZI169, ZI170);
			/* BEGINNING OF INLINE: 174 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_174;
			}
			/* END OF INLINE: 174 */
		}
		/* UNREACHED */
	default:
		{
			ZI171 = ZI168;
			ZI172 = ZI169;
			ZI173 = ZI170;
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
	*ZO171 = ZI171;
	*ZO172 = ZI172;
	*ZO173 = ZI173;
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
p_180(lex_state lex_state, act_state act_state, zone ZI176, fsm ZI177, zone *ZO178, fsm *ZO179)
{
	zone ZI178;
	fsm ZI179;

ZL2_180:;
	switch (CURRENT_TERMINAL) {
	case (TOK_PIPE):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI176, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 733 "src/lx/parser.act"

		assert((ZI177) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_union((ZI177), (ZIb), NULL);
		if ((ZIq) == NULL) {
			perror("fsm_union");
			goto ZL1;
		}
	
#line 1243 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: 180 */
			ZI177 = ZIq;
			goto ZL2_180;
			/* END OF INLINE: 180 */
		}
		/* UNREACHED */
	default:
		{
			ZI178 = ZI176;
			ZI179 = ZI177;
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
	*ZO178 = ZI178;
	*ZO179 = ZI179;
}

static void
p_expr_C_Cpostfix_Hexpr(lex_state lex_state, act_state act_state, zone ZI188, fsm *ZO196)
{
	fsm ZI196;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_expr_C_Cprimary_Hexpr (lex_state, act_state, ZI188, &ZIq);
		p_197 (lex_state, act_state, ZIq, &ZI196);
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
	*ZO196 = ZI196;
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
#line 685 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_reverse((ZIq))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 1321 "src/lx/parser.c"
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
#line 676 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complete((ZIq), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 1345 "src/lx/parser.c"
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
#line 667 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complement((ZIq))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 1369 "src/lx/parser.c"
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
p_186(lex_state lex_state, act_state act_state, zone ZI182, fsm ZI183, zone *ZO184, fsm *ZO185)
{
	zone ZI184;
	fsm ZI185;

ZL2_186:;
	switch (CURRENT_TERMINAL) {
	case (TOK_AND):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI182, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-intersect */
			{
#line 722 "src/lx/parser.act"

		assert((ZI183) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_intersect((ZI183), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_intersect");
			goto ZL1;
		}
	
#line 1430 "src/lx/parser.c"
			}
			/* END OF ACTION: op-intersect */
			/* BEGINNING OF INLINE: 186 */
			ZI183 = ZIq;
			goto ZL2_186;
			/* END OF INLINE: 186 */
		}
		/* UNREACHED */
	default:
		{
			ZI184 = ZI182;
			ZI185 = ZI183;
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
	*ZO184 = ZI184;
	*ZO185 = ZI185;
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
#line 733 "src/lx/parser.act"

		assert((ZIold_Hexit) != NULL);
		assert((ZInew_Hexit) != NULL);

		(*ZIexit) = fsm_union((ZIold_Hexit), (ZInew_Hexit), NULL);
		if ((*ZIexit) == NULL) {
			perror("fsm_union");
			goto ZL1;
		}
	
#line 1487 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: list-of-things::zone-thing::list-of-zone-to-mappings::list-of-zone-to-mappings-x */
			goto ZL2_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings_C_Clist_Hof_Hzone_Hto_Hmappings_Hx;
			/* END OF INLINE: list-of-things::zone-thing::list-of-zone-to-mappings::list-of-zone-to-mappings-x */
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
		p_112 (lex_state, act_state, &ZIt);
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
p_197(lex_state lex_state, act_state act_state, fsm ZI191, fsm *ZO196)
{
	fsm ZI196;

ZL2_197:;
	switch (CURRENT_TERMINAL) {
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
		{
			fsm ZIq;

			ZIq = ZI191;
			/* BEGINNING OF INLINE: 272 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CROSS):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-cross */
						{
#line 577 "src/lx/parser.act"

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
	
#line 1620 "src/lx/parser.c"
						}
						/* END OF ACTION: op-cross */
						/* BEGINNING OF INLINE: 197 */
						ZI191 = ZIq;
						goto ZL2_197;
						/* END OF INLINE: 197 */
					}
					/* UNREACHED */
				case (TOK_QMARK):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-qmark */
						{
#line 622 "src/lx/parser.act"

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
	
#line 1679 "src/lx/parser.c"
						}
						/* END OF ACTION: op-qmark */
						/* BEGINNING OF INLINE: 197 */
						ZI191 = ZIq;
						goto ZL2_197;
						/* END OF INLINE: 197 */
					}
					/* UNREACHED */
				case (TOK_STAR):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-star */
						{
#line 527 "src/lx/parser.act"

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
	
#line 1743 "src/lx/parser.c"
						}
						/* END OF ACTION: op-star */
						/* BEGINNING OF INLINE: 197 */
						ZI191 = ZIq;
						goto ZL2_197;
						/* END OF INLINE: 197 */
					}
					/* UNREACHED */
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 272 */
		}
		/* UNREACHED */
	default:
		{
			ZI196 = ZI191;
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
	*ZO196 = ZI196;
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
p_204(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI202, fsm *ZOq)
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
#line 711 "src/lx/parser.act"

		assert((*ZI202) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_subtract((*ZI202), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_subtract");
			goto ZL1;
		}
	
#line 1827 "src/lx/parser.c"
			}
			/* END OF ACTION: op-subtract */
		}
		break;
	default:
		{
			ZIq = *ZI202;
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
p_208(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI206, fsm *ZOq)
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
#line 694 "src/lx/parser.act"

		assert((*ZI206) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((*ZI206), (ZIb), NULL);
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 1902 "src/lx/parser.c"
			}
			/* END OF ACTION: op-concat */
		}
		break;
	default:
		{
			ZIq = *ZI206;
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
p_212(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI210, fsm *ZOq)
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
#line 705 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 1947 "src/lx/parser.c"
			}
			/* END OF ACTION: op-product */
		}
		break;
	default:
		{
			ZIq = *ZI210;
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
p_215(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RE):
		{
			flags ZIf;
			arr ZIa;

			/* BEGINNING OF EXTRACT: RE */
			{
#line 240 "src/lx/parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */

		if (-1 == re_flags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			err(lex_state, "/%s: Unrecognised regexp flags", lex_state->buf.a + 1);
			exit(EXIT_FAILURE);
		}

		ZIf |= RE_ANCHORED; /* regexps in lx are implicitly anchored */
		ZIf |= RE_SINGLE;   /* we can't assume we're lexing text */
	
#line 1995 "src/lx/parser.c"
			}
			/* END OF EXTRACT: RE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 261 "src/lx/parser.act"

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
	
#line 2026 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 385 "src/lx/parser.act"

		struct re_err err;

		assert((ZIa) != NULL);
		assert((ZIa)->p != NULL);

		(ZIr) = re_comp(RE_NATIVE, act_agetc, (ZIa), act_state->opt, (ZIf), &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_NATIVE, &err, NULL, (ZIa)->p);
			exit(EXIT_FAILURE);
		}
	
#line 2046 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-regex */
			/* BEGINNING OF ACTION: free-arr */
			{
#line 774 "src/lx/parser.act"

		free((ZIa));
	
#line 2055 "src/lx/parser.c"
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
#line 261 "src/lx/parser.act"

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
	
#line 2092 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 370 "src/lx/parser.act"

		struct re_err err;

		assert((ZIa) != NULL);
		assert((ZIa)->p != NULL);

		(ZIr) = re_comp(RE_LITERAL, act_agetc, (ZIa), act_state->opt, 0, &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_LITERAL, &err, NULL, (ZIa)->p); /* XXX */
			exit(EXIT_FAILURE);
		}
	
#line 2112 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-literal */
			/* BEGINNING OF ACTION: free-arr */
			{
#line 774 "src/lx/parser.act"

		free((ZIa));
	
#line 2121 "src/lx/parser.c"
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
p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state lex_state, act_state act_state, zone ZI182, fsm *ZO185)
{
	fsm ZI185;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI184;

		p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI182, &ZIq);
		p_186 (lex_state, act_state, ZI182, ZIq, &ZI184, &ZI185);
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
	*ZO185 = ZI185;
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
#line 744 "src/lx/parser.act"

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
	
#line 2205 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 458 "src/lx/parser.act"

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
	
#line 2243 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: list-of-things::zone-thing::list-of-zone-from-mappings::list-of-zone-from-mappings-x */
			goto ZL2_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx;
			/* END OF INLINE: list-of-things::zone-thing::list-of-zone-from-mappings::list-of-zone-from-mappings-x */
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
#line 523 "src/lx/parser.act"

		(ZIparent) = NULL;
	
#line 2281 "src/lx/parser.c"
		}
		/* END OF ACTION: no-zone */
		/* BEGINNING OF ACTION: make-ast */
		{
#line 432 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 2294 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 440 "src/lx/parser.act"

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
	
#line 2317 "src/lx/parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 519 "src/lx/parser.act"

		(ZIexit) = NULL;
	
#line 2326 "src/lx/parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 508 "src/lx/parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 2338 "src/lx/parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 165 */
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
#line 815 "src/lx/parser.act"

		err_expected(lex_state, "EOF");
	
#line 2366 "src/lx/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 165 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 432 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 2387 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 782 "src/lx/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 2397 "src/lx/parser.c"
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
		fsm ZI206;

		p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, ZIz, &ZI206);
		p_208 (lex_state, act_state, &ZIz, &ZI206, &ZIq);
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
#line 458 "src/lx/parser.act"

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
	
#line 2487 "src/lx/parser.c"
		}
		/* END OF ACTION: add-mapping */
		/* BEGINNING OF ACTION: clone */
		{
#line 764 "src/lx/parser.act"

		assert((ZIr) != NULL);

		(ZIexit) = fsm_clone((ZIr));
		if ((ZIexit) == NULL) {
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 2502 "src/lx/parser.c"
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
p_112(lex_state lex_state, act_state act_state, string *ZOt)
{
	string ZIt;

	switch (CURRENT_TERMINAL) {
	case (TOK_MAP):
		{
			/* BEGINNING OF INLINE: 113 */
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
#line 791 "src/lx/parser.act"

		err_expected(lex_state, "'->'");
	
#line 2542 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-map */
				}
			ZL2:;
			}
			/* END OF INLINE: 113 */
			switch (CURRENT_TERMINAL) {
			case (TOK_TOKEN):
				/* BEGINNING OF EXTRACT: TOKEN */
				{
#line 223 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 2562 "src/lx/parser.c"
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
#line 515 "src/lx/parser.act"

		(ZIt) = NULL;
	
#line 2580 "src/lx/parser.c"
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
p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state lex_state, act_state act_state, zone ZI176, fsm *ZO179)
{
	fsm ZI179;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI178;

		p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI176, &ZIq);
		p_180 (lex_state, act_state, ZI176, ZIq, &ZI178, &ZI179);
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
	*ZO179 = ZI179;
}

static void
p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI210;

		p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI210);
		p_212 (lex_state, act_state, &ZIz, &ZI210, &ZIq);
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
p_122(lex_state lex_state, act_state act_state)
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
#line 799 "src/lx/parser.act"

		err_expected(lex_state, "';'");
	
#line 2673 "src/lx/parser.c"
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
		fsm ZI202;

		p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, ZIz, &ZI202);
		p_204 (lex_state, act_state, &ZIz, &ZI202, &ZIq);
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
p_251(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZI249, fsm *ZI250)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_SEMI):
		{
			zone ZIto;

			p_122 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-zone */
			{
#line 523 "src/lx/parser.act"

		(ZIto) = NULL;
	
#line 2724 "src/lx/parser.c"
			}
			/* END OF ACTION: no-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 458 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI250) != NULL);

		if ((*ZI249) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI249));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI250), t, (ZIto));
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
	
#line 2762 "src/lx/parser.c"
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
#line 440 "src/lx/parser.act"

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
	
#line 2792 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 458 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI250) != NULL);

		if ((*ZI249) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI249));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI250), t, (ZIchild));
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
	
#line 2830 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hfrom_Hmappings_C_Clist_Hof_Hzone_Hfrom_Hmappings_Hx (lex_state, act_state, *ZIa, *ZIz, ZIchild, *ZIexit);
			/* BEGINNING OF INLINE: 151 */
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
#line 803 "src/lx/parser.act"

		err_expected(lex_state, "'..'");
	
#line 2858 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 151 */
			p_list_Hof_Hthings_C_Czone_Hthing_C_Clist_Hof_Hzone_Hto_Hmappings (lex_state, act_state, *ZIa, *ZIz, ZIchild, &ZIchild_Hexit);
			/* BEGINNING OF INLINE: 153 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIw;
						fsm ZIv;

						p_122 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-zone */
						{
#line 523 "src/lx/parser.act"

		(ZIx) = NULL;
	
#line 2887 "src/lx/parser.c"
						}
						/* END OF ACTION: no-zone */
						/* BEGINNING OF ACTION: no-token */
						{
#line 515 "src/lx/parser.act"

		(ZIy) = NULL;
	
#line 2896 "src/lx/parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 401 "src/lx/parser.act"

		fsm_state_t start, end;

		(ZIw) = fsm_new(act_state->opt);
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
	
#line 2932 "src/lx/parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 744 "src/lx/parser.act"

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
	
#line 2957 "src/lx/parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 458 "src/lx/parser.act"

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
	
#line 2995 "src/lx/parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						p_159 (lex_state, act_state);
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIchild_Hexit);
						p_160 (lex_state, act_state);
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
#line 823 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3026 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 153 */
		}
		break;
	case (TOK_OPEN):
		{
			fsm ZIr2;
			zone ZIchild;

			/* BEGINNING OF ACTION: no-exit */
			{
#line 519 "src/lx/parser.act"

		(ZIr2) = NULL;
	
#line 3046 "src/lx/parser.c"
			}
			/* END OF ACTION: no-exit */
			/* BEGINNING OF ACTION: make-zone */
			{
#line 440 "src/lx/parser.act"

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
	
#line 3069 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 458 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI250) != NULL);

		if ((*ZI249) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI249));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI250), t, (ZIchild));
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
	
#line 3107 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: 131 */
			{
				{
					p_159 (lex_state, act_state);
					p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
					p_160 (lex_state, act_state);
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
#line 823 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3130 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL6:;
			}
			/* END OF INLINE: 131 */
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
p_252(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BIND):
		{
			fsm ZIr;

			/* BEGINNING OF INLINE: 121 */
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
#line 795 "src/lx/parser.act"

		err_expected(lex_state, "'='");
	
#line 3178 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-bind */
				}
			ZL2:;
			}
			/* END OF INLINE: 121 */
			p_expr (lex_state, act_state, *ZIz, &ZIr);
			p_122 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 491 "src/lx/parser.act"

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
	
#line 3210 "src/lx/parser.c"
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
			fsm ZI273;
			fsm ZI210;
			fsm ZI206;
			fsm ZI202;
			fsm ZI274;
			zone ZI275;
			fsm ZI276;
			zone ZI277;
			fsm ZI278;
			string ZI279;
			fsm ZI280;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 286 "src/lx/parser.act"

		struct ast_zone *z;

		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);

		for (z = (*ZIz); z != NULL; z = z->parent) {
			(ZI273) = var_find(z->vl, (*ZIn));
			if ((ZI273) != NULL) {
				break;
			}

			if ((*ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (*ZIn));
				goto ZL1;
			}
		}

		(ZI273) = fsm_clone((ZI273));
		if ((ZI273) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 3263 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
			p_197 (lex_state, act_state, ZI273, &ZI210);
			p_212 (lex_state, act_state, ZIz, &ZI210, &ZI206);
			p_208 (lex_state, act_state, ZIz, &ZI206, &ZI202);
			p_204 (lex_state, act_state, ZIz, &ZI202, &ZI274);
			p_186 (lex_state, act_state, *ZIz, ZI274, &ZI275, &ZI276);
			p_180 (lex_state, act_state, *ZIz, ZI276, &ZI277, &ZI278);
			p_112 (lex_state, act_state, &ZI279);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 744 "src/lx/parser.act"

		assert((ZI278) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI280) = (ZI278);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI280) = fsm_subtract((ZI278), (*ZIexit));
			if ((ZI280) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 3299 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_251 (lex_state, act_state, ZIa, ZIz, ZIexit, &ZI279, &ZI280);
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

/* BEGINNING OF TRAILER */

#line 827 "src/lx/parser.act"


	struct ast *lx_parse(FILE *f, const struct fsm_options *opt) {
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

		act_state->opt = opt;

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

#line 3385 "src/lx/parser.c"

/* END OF FILE */
