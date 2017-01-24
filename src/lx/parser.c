/*
 * Automatically generated from the files:
 *	src/lx/parser.sid
 * and
 *	src/lx/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 98 "src/lx/parser.act"


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
	#include "out.h"
	#include "var.h"

	typedef char *               string;
	typedef enum re_flags        flags;
	typedef struct fsm *         fsm;
	typedef struct ast_zone *    zone;
	typedef struct ast_mapping * mapping;

	struct act_state {
		enum lx_token lex_tok;
		enum lx_token lex_tok_save;
		unsigned int zn;
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

#line 90 "src/lx/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_pattern(lex_state, act_state, zone, fsm *);
static void p_143(lex_state, act_state);
static void p_144(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_157(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_163(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_169(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_pattern_C_Cbody(lex_state, act_state);
static void p_expr_C_Cprimary_Hexpr(lex_state, act_state, zone, fsm *);
static void p_180(lex_state, act_state, fsm, fsm *);
static void p_expr_C_Cpostfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cprefix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_188(lex_state, act_state, zone *, fsm *, fsm *);
static void p_191(lex_state, act_state, zone *, fsm *, fsm *);
static void p_expr(lex_state, act_state, zone, fsm *);
static void p_194(lex_state, act_state, zone *, fsm *, fsm *);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_198(lex_state, act_state, fsm *);
static void p_expr_C_Cinfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_225(lex_state, act_state, ast *, zone *, fsm *, string *);
static void p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state, act_state, zone, fsm *);
extern void p_lx(lex_state, act_state, ast *);
static void p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state, act_state, zone, fsm *);
static void p_110(lex_state, act_state, string *);
static void p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state, act_state, zone, fsm *);
static void p_120(lex_state, act_state);
static void p_248(lex_state, act_state, ast *, zone *, string *, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state, act_state, zone, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI151, zone ZI152, fsm ZI153)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI154;
		zone ZI155;
		fsm ZI156;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI151, ZI152, ZI153);
		p_157 (lex_state, act_state, ZI151, ZI152, ZI153, &ZI154, &ZI155, &ZI156);
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
#line 787 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 162 "src/lx/parser.c"
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
#line 213 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 188 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-var */
			{
#line 259 "src/lx/parser.act"

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
	
#line 221 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
		}
		break;
	case (TOK_TOKEN):
		{
			string ZIt;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 205 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 241 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 286 "src/lx/parser.act"

		const struct ast_mapping *m;

		assert((ZIt) != NULL);

		(ZIr) = fsm_new();
		if ((ZIr) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		(ZIr)->tidy = 0;

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
	
#line 295 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			p_pattern_C_Cbody (lex_state, act_state);
			p_198 (lex_state, act_state, &ZIr);
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
p_143(lex_state lex_state, act_state act_state)
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
#line 771 "src/lx/parser.act"

		err_expected(lex_state, "'{'");
	
#line 348 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-open */
	}
}

static void
p_144(lex_state lex_state, act_state act_state)
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
#line 775 "src/lx/parser.act"

		err_expected(lex_state, "'}'");
	
#line 378 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-close */
	}
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BANG):
		{
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI206;
			zone ZI167;
			fsm ZIq;
			zone ZI207;
			fsm ZI208;
			string ZI209;
			fsm ZI210;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI192);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 645 "src/lx/parser.act"

		assert((ZI192) != NULL);

		if (!fsm_reverse((ZI192))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 418 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
			p_194 (lex_state, act_state, &ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, &ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI206);
			p_169 (lex_state, act_state, ZIz, ZI206, &ZI167, &ZIq);
			p_163 (lex_state, act_state, ZIz, ZIq, &ZI207, &ZI208);
			p_110 (lex_state, act_state, &ZI209);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

		assert((ZI208) != NULL);

		if ((ZIexit) == NULL) {
			(ZI210) = (ZI208);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI210) = fsm_subtract((ZI208), (ZIexit));
			if ((ZI210) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if ((ZI210)->tidy) {
				if (!fsm_trim((ZI210))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 460 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, &ZIa, &ZIz, &ZI209, &ZI210);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HAT):
		{
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI212;
			zone ZI167;
			fsm ZIq;
			zone ZI213;
			fsm ZI214;
			string ZI215;
			fsm ZI216;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI192);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complete */
			{
#line 636 "src/lx/parser.act"

		assert((ZI192) != NULL);

		if (!fsm_complete((ZI192), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 500 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complete */
			p_194 (lex_state, act_state, &ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, &ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI212);
			p_169 (lex_state, act_state, ZIz, ZI212, &ZI167, &ZIq);
			p_163 (lex_state, act_state, ZIz, ZIq, &ZI213, &ZI214);
			p_110 (lex_state, act_state, &ZI215);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

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

			if ((ZI216)->tidy) {
				if (!fsm_trim((ZI216))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 542 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, &ZIa, &ZIz, &ZI215, &ZI216);
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
#line 213 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 566 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_225 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_LPAREN):
		{
			fsm ZI218;
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI219;
			zone ZI167;
			fsm ZIq;
			zone ZI220;
			fsm ZI221;
			string ZI222;
			fsm ZI223;

			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZI218);
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
			p_180 (lex_state, act_state, ZI218, &ZI192);
			p_194 (lex_state, act_state, &ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, &ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI219);
			p_169 (lex_state, act_state, ZIz, ZI219, &ZI167, &ZIq);
			p_163 (lex_state, act_state, ZIz, ZIq, &ZI220, &ZI221);
			p_110 (lex_state, act_state, &ZI222);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

		assert((ZI221) != NULL);

		if ((ZIexit) == NULL) {
			(ZI223) = (ZI221);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI223) = fsm_subtract((ZI221), (ZIexit));
			if ((ZI223) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if ((ZI223)->tidy) {
				if (!fsm_trim((ZI223))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 643 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, &ZIa, &ZIz, &ZI222, &ZI223);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TILDE):
		{
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI200;
			zone ZI167;
			fsm ZIq;
			zone ZI201;
			fsm ZI202;
			string ZI203;
			fsm ZI204;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI192);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 627 "src/lx/parser.act"

		assert((ZI192) != NULL);

		if (!fsm_complement((ZI192))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 683 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
			p_194 (lex_state, act_state, &ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, &ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI200);
			p_169 (lex_state, act_state, ZIz, ZI200, &ZI167, &ZIq);
			p_163 (lex_state, act_state, ZIz, ZIq, &ZI201, &ZI202);
			p_110 (lex_state, act_state, &ZI203);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

		assert((ZI202) != NULL);

		if ((ZIexit) == NULL) {
			(ZI204) = (ZI202);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI204) = fsm_subtract((ZI202), (ZIexit));
			if ((ZI204) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if ((ZI204)->tidy) {
				if (!fsm_trim((ZI204))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 725 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, &ZIa, &ZIz, &ZI203, &ZI204);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TOKEN):
		{
			string ZI226;
			fsm ZI227;
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI228;
			zone ZI167;
			fsm ZIq;
			zone ZI229;
			fsm ZI230;
			string ZI231;
			fsm ZI232;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 205 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZI226 = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZI226 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 761 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 286 "src/lx/parser.act"

		const struct ast_mapping *m;

		assert((ZI226) != NULL);

		(ZI227) = fsm_new();
		if ((ZI227) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		(ZI227)->tidy = 0;

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			struct fsm_state *s;

			if (m->token == NULL) {
				continue;
			}

			if (0 != strcmp(m->token->s, (ZI226))) {
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

			(ZI227) = fsm_union((ZI227), fsm);
			if ((ZI227) == NULL) {
				perror("fsm_determinise_opaque");
				goto ZL1;
			}
		}
	
#line 815 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
			p_180 (lex_state, act_state, ZI227, &ZI192);
			p_194 (lex_state, act_state, &ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, &ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI228);
			p_169 (lex_state, act_state, ZIz, ZI228, &ZI167, &ZIq);
			p_163 (lex_state, act_state, ZIz, ZIq, &ZI229, &ZI230);
			p_110 (lex_state, act_state, &ZI231);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

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

			if ((ZI232)->tidy) {
				if (!fsm_trim((ZI232))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 858 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, &ZIa, &ZIz, &ZI231, &ZI232);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			fsm ZI234;
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI235;
			zone ZI167;
			fsm ZIq;
			zone ZI236;
			fsm ZI237;
			string ZI238;
			fsm ZI239;

			p_pattern_C_Cbody (lex_state, act_state);
			p_198 (lex_state, act_state, &ZI234);
			p_180 (lex_state, act_state, ZI234, &ZI192);
			p_194 (lex_state, act_state, &ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, &ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, &ZIz, &ZI186, &ZI235);
			p_169 (lex_state, act_state, ZIz, ZI235, &ZI167, &ZIq);
			p_163 (lex_state, act_state, ZIz, ZIq, &ZI236, &ZI237);
			p_110 (lex_state, act_state, &ZI238);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

		assert((ZI237) != NULL);

		if ((ZIexit) == NULL) {
			(ZI239) = (ZI237);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI239) = fsm_subtract((ZI237), (ZIexit));
			if ((ZI239) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if ((ZI239)->tidy) {
				if (!fsm_trim((ZI239))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 925 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, &ZIa, &ZIz, &ZI238, &ZI239);
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
#line 783 "src/lx/parser.act"

		err_expected(lex_state, "mapping, binding or zone");
	
#line 949 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-thing */
	}
}

static void
p_157(lex_state lex_state, act_state act_state, ast ZI151, zone ZI152, fsm ZI153, ast *ZO154, zone *ZO155, fsm *ZO156)
{
	ast ZI154;
	zone ZI155;
	fsm ZI156;

ZL2_157:;
	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI151, ZI152, ZI153);
			/* BEGINNING OF INLINE: 157 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_157;
			}
			/* END OF INLINE: 157 */
		}
		/* UNREACHED */
	default:
		{
			ZI154 = ZI151;
			ZI155 = ZI152;
			ZI156 = ZI153;
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
	*ZO154 = ZI154;
	*ZO155 = ZI155;
	*ZO156 = ZI156;
}

static void
p_163(lex_state lex_state, act_state act_state, zone ZI159, fsm ZI160, zone *ZO161, fsm *ZO162)
{
	zone ZI161;
	fsm ZI162;

ZL2_163:;
	switch (CURRENT_TERMINAL) {
	case (TOK_PIPE):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI159, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 693 "src/lx/parser.act"

		assert((ZI160) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_union((ZI160), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_union");
			goto ZL1;
		}
	
#line 1031 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: 163 */
			ZI160 = ZIq;
			goto ZL2_163;
			/* END OF INLINE: 163 */
		}
		/* UNREACHED */
	default:
		{
			ZI161 = ZI159;
			ZI162 = ZI160;
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
	*ZO161 = ZI161;
	*ZO162 = ZI162;
}

static void
p_169(lex_state lex_state, act_state act_state, zone ZI165, fsm ZI166, zone *ZO167, fsm *ZO168)
{
	zone ZI167;
	fsm ZI168;

ZL2_169:;
	switch (CURRENT_TERMINAL) {
	case (TOK_AND):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI165, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-intersect */
			{
#line 682 "src/lx/parser.act"

		assert((ZI166) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_intersect((ZI166), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_intersect");
			goto ZL1;
		}
	
#line 1090 "src/lx/parser.c"
			}
			/* END OF ACTION: op-intersect */
			/* BEGINNING OF INLINE: 169 */
			ZI166 = ZIq;
			goto ZL2_169;
			/* END OF INLINE: 169 */
		}
		/* UNREACHED */
	default:
		{
			ZI167 = ZI165;
			ZI168 = ZI166;
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
	*ZO167 = ZI167;
	*ZO168 = ZI168;
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
#line 195 "src/lx/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];

		/* position of first character */
		if (lex_state->p == lex_state->a) {
			lex_state->pstart = lex_state->lx.start;
		}
	
#line 1145 "src/lx/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_ESC):
					{
						/* BEGINNING OF EXTRACT: ESC */
						{
#line 120 "src/lx/parser.act"

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
	
#line 1178 "src/lx/parser.c"
						}
						/* END OF EXTRACT: ESC */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						/* BEGINNING OF EXTRACT: HEX */
						{
#line 188 "src/lx/parser.act"

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
	
#line 1215 "src/lx/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						/* BEGINNING OF EXTRACT: OCT */
						{
#line 161 "src/lx/parser.act"

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
	
#line 1252 "src/lx/parser.c"
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
#line 235 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 1270 "src/lx/parser.c"
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
p_180(lex_state lex_state, act_state act_state, fsm ZI174, fsm *ZO179)
{
	fsm ZI179;

ZL2_180:;
	switch (CURRENT_TERMINAL) {
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
		{
			fsm ZIq;

			ZIq = ZI174;
			/* BEGINNING OF INLINE: 241 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CROSS):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-cross */
						{
#line 542 "src/lx/parser.act"

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
	
#line 1399 "src/lx/parser.c"
						}
						/* END OF ACTION: op-cross */
						/* BEGINNING OF INLINE: 180 */
						ZI174 = ZIq;
						goto ZL2_180;
						/* END OF INLINE: 180 */
					}
					/* UNREACHED */
				case (TOK_QMARK):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-qmark */
						{
#line 587 "src/lx/parser.act"

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
	
#line 1458 "src/lx/parser.c"
						}
						/* END OF ACTION: op-qmark */
						/* BEGINNING OF INLINE: 180 */
						ZI174 = ZIq;
						goto ZL2_180;
						/* END OF INLINE: 180 */
					}
					/* UNREACHED */
				case (TOK_STAR):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-star */
						{
#line 492 "src/lx/parser.act"

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
	
#line 1522 "src/lx/parser.c"
						}
						/* END OF ACTION: op-star */
						/* BEGINNING OF INLINE: 180 */
						ZI174 = ZIq;
						goto ZL2_180;
						/* END OF INLINE: 180 */
					}
					/* UNREACHED */
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 241 */
		}
		/* UNREACHED */
	default:
		{
			ZI179 = ZI174;
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
	*ZO179 = ZI179;
}

static void
p_expr_C_Cpostfix_Hexpr(lex_state lex_state, act_state act_state, zone ZI171, fsm *ZO179)
{
	fsm ZI179;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_expr_C_Cprimary_Hexpr (lex_state, act_state, ZI171, &ZIq);
		p_180 (lex_state, act_state, ZIq, &ZI179);
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
#line 645 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_reverse((ZIq))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 1605 "src/lx/parser.c"
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
#line 636 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complete((ZIq), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 1629 "src/lx/parser.c"
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
#line 627 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complement((ZIq))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 1653 "src/lx/parser.c"
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
#line 671 "src/lx/parser.act"

		assert((*ZI186) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_subtract((*ZI186), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_subtract");
			goto ZL1;
		}
	
#line 1711 "src/lx/parser.c"
			}
			/* END OF ACTION: op-subtract */
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
p_191(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI189, fsm *ZOq)
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
#line 654 "src/lx/parser.act"

		assert((*ZI189) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((*ZI189), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 1762 "src/lx/parser.c"
			}
			/* END OF ACTION: op-concat */
		}
		break;
	default:
		{
			ZIq = *ZI189;
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
p_194(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI192, fsm *ZOq)
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
#line 666 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 1830 "src/lx/parser.c"
			}
			/* END OF ACTION: op-product */
		}
		break;
	default:
		{
			ZIq = *ZI192;
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
p_198(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RE):
		{
			flags ZIf;
			string ZIs;

			/* BEGINNING OF EXTRACT: RE */
			{
#line 225 "src/lx/parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */

		if (-1 == re_flags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			err(lex_state, "/%s: Unrecognised regexp flags", lex_state->buf.a + 1);
			exit(EXIT_FAILURE);
		}
	
#line 1901 "src/lx/parser.c"
			}
			/* END OF EXTRACT: RE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 247 "src/lx/parser.act"

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
	
#line 1925 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 354 "src/lx/parser.act"

		struct re_err err;
		const char *s;

		assert((ZIs) != NULL);

		s = (ZIs);

		(ZIr) = re_comp(RE_NATIVE, re_sgetc, &s, (ZIf), &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_NATIVE, &err, NULL, (ZIs));
			exit(EXIT_FAILURE);
		}

		(ZIr)->tidy = 0;
	
#line 1949 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-regex */
			/* BEGINNING OF ACTION: free */
			{
#line 741 "src/lx/parser.act"

		free((ZIs));
	
#line 1958 "src/lx/parser.c"
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
#line 247 "src/lx/parser.act"

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
	
#line 1988 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 335 "src/lx/parser.act"

		struct re_err err;
		const char *s;

		assert((ZIs) != NULL);

		s = (ZIs);

		(ZIr) = re_comp(RE_LITERAL, re_sgetc, &s, 0, &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_LITERAL, &err, NULL, (ZIs));
			exit(EXIT_FAILURE);
		}

		(ZIr)->tidy = 0;
	
#line 2012 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-literal */
			/* BEGINNING OF ACTION: free */
			{
#line 741 "src/lx/parser.act"

		free((ZIs));
	
#line 2021 "src/lx/parser.c"
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
p_225(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
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
#line 759 "src/lx/parser.act"

		err_expected(lex_state, "'='");
	
#line 2091 "src/lx/parser.c"
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
#line 704 "src/lx/parser.act"

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

			if ((ZIq)->tidy) {
				if (!fsm_trim((ZIq))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 2132 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_120 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 453 "src/lx/parser.act"

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
	
#line 2159 "src/lx/parser.c"
			}
			/* END OF ACTION: add-binding */
		}
		break;
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_SEMI): case (TOK_TO): case (TOK_MAP): case (TOK_OPEN):
	case (TOK_LPAREN): case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
	case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT): case (TOK_DASH):
	case (TOK_DOT): case (TOK_PIPE): case (TOK_AND):
		{
			fsm ZI242;
			fsm ZI192;
			fsm ZI189;
			fsm ZI186;
			fsm ZI243;
			zone ZI167;
			fsm ZIq;
			zone ZI244;
			fsm ZI245;
			string ZI246;
			fsm ZI247;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 259 "src/lx/parser.act"

		struct ast_zone *z;

		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);

		for (z = (*ZIz); z != NULL; z = z->parent) {
			(ZI242) = var_find(z->vl, (*ZIn));
			if ((ZI242) != NULL) {
				break;
			}

			if ((*ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (*ZIn));
				goto ZL1;
			}
		}

		(ZI242) = fsm_clone((ZI242));
		if ((ZI242) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 2212 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
			p_180 (lex_state, act_state, ZI242, &ZI192);
			p_194 (lex_state, act_state, ZIz, &ZI192, &ZI189);
			p_191 (lex_state, act_state, ZIz, &ZI189, &ZI186);
			p_188 (lex_state, act_state, ZIz, &ZI186, &ZI243);
			p_169 (lex_state, act_state, *ZIz, ZI243, &ZI167, &ZIq);
			p_163 (lex_state, act_state, *ZIz, ZIq, &ZI244, &ZI245);
			p_110 (lex_state, act_state, &ZI246);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 704 "src/lx/parser.act"

		assert((ZI245) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI247) = (ZI245);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI247) = fsm_subtract((ZI245), (*ZIexit));
			if ((ZI247) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if ((ZI247)->tidy) {
				if (!fsm_trim((ZI247))) {
					perror("fsm_trim");
					goto ZL1;
				}
			}
		}
	
#line 2255 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_248 (lex_state, act_state, ZIa, ZIz, &ZI246, &ZI247);
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
p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state lex_state, act_state act_state, zone ZI165, fsm *ZO168)
{
	fsm ZI168;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI167;

		p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI165, &ZIq);
		p_169 (lex_state, act_state, ZI165, ZIq, &ZI167, &ZI168);
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
	*ZO168 = ZI168;
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
#line 483 "src/lx/parser.act"

		(ZIparent) = NULL;
	
#line 2322 "src/lx/parser.c"
		}
		/* END OF ACTION: no-zone */
		/* BEGINNING OF ACTION: make-ast */
		{
#line 400 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 2335 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 408 "src/lx/parser.act"

		assert((ZIa) != NULL);

		if (print_progress) {
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
	
#line 2357 "src/lx/parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 479 "src/lx/parser.act"

		(ZIexit) = NULL;
	
#line 2366 "src/lx/parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 468 "src/lx/parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 2378 "src/lx/parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 149 */
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
#line 779 "src/lx/parser.act"

		err_expected(lex_state, "EOF");
	
#line 2406 "src/lx/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 149 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 400 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 2427 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 747 "src/lx/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 2437 "src/lx/parser.c"
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
		fsm ZI189;

		p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, ZIz, &ZI189);
		p_191 (lex_state, act_state, &ZIz, &ZI189, &ZIq);
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
#line 755 "src/lx/parser.act"

		err_expected(lex_state, "'->'");
	
#line 2503 "src/lx/parser.c"
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
#line 205 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 2523 "src/lx/parser.c"
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
#line 475 "src/lx/parser.act"

		(ZIt) = NULL;
	
#line 2541 "src/lx/parser.c"
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
p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state lex_state, act_state act_state, zone ZI159, fsm *ZO162)
{
	fsm ZI162;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI161;

		p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI159, &ZIq);
		p_163 (lex_state, act_state, ZI159, ZIq, &ZI161, &ZI162);
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
	*ZO162 = ZI162;
}

static void
p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI192;

		p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI192);
		p_194 (lex_state, act_state, &ZIz, &ZI192, &ZIq);
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
#line 763 "src/lx/parser.act"

		err_expected(lex_state, "';'");
	
#line 2634 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_248(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI246, fsm *ZI247)
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
#line 483 "src/lx/parser.act"

		(ZIto) = NULL;
	
#line 2659 "src/lx/parser.c"
			}
			/* END OF ACTION: no-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 428 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI247) != NULL);

		if ((*ZI246) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI246));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI247), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2690 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 134 */
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
#line 767 "src/lx/parser.act"

		err_expected(lex_state, "'..'");
	
#line 2721 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 134 */
			p_token_Hmapping (lex_state, act_state, *ZIz, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 408 "src/lx/parser.act"

		assert((*ZIa) != NULL);

		if (print_progress) {
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
	
#line 2752 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 428 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI247) != NULL);

		if ((*ZI246) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI246));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI247), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 2783 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 428 "src/lx/parser.act"

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
	
#line 2814 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: 136 */
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
#line 483 "src/lx/parser.act"

		(ZIx) = NULL;
	
#line 2839 "src/lx/parser.c"
						}
						/* END OF ACTION: no-zone */
						/* BEGINNING OF ACTION: no-token */
						{
#line 475 "src/lx/parser.act"

		(ZIy) = NULL;
	
#line 2848 "src/lx/parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 731 "src/lx/parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 2863 "src/lx/parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 373 "src/lx/parser.act"

		struct fsm_state *start, *end;

		(ZIw) = fsm_new();
		if ((ZIw) == NULL) {
			perror("fsm_new");
			goto ZL5;
		}

		(ZIw)->tidy = 0;

		start = fsm_addstate((ZIw));
		end   = fsm_addstate((ZIw));
		if (start == NULL || end == NULL) {
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
	
#line 2897 "src/lx/parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 704 "src/lx/parser.act"

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

			if ((ZIv)->tidy) {
				if (!fsm_trim((ZIv))) {
					perror("fsm_trim");
					goto ZL5;
				}
			}
		}
	
#line 2929 "src/lx/parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 428 "src/lx/parser.act"

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
	
#line 2960 "src/lx/parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						p_143 (lex_state, act_state);
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						p_144 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
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
#line 787 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 2988 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 136 */
		}
		break;
	case (TOK_OPEN):
		{
			fsm ZIr2;
			zone ZIchild;

			/* BEGINNING OF ACTION: no-exit */
			{
#line 479 "src/lx/parser.act"

		(ZIr2) = NULL;
	
#line 3008 "src/lx/parser.c"
			}
			/* END OF ACTION: no-exit */
			/* BEGINNING OF ACTION: make-zone */
			{
#line 408 "src/lx/parser.act"

		assert((*ZIa) != NULL);

		if (print_progress) {
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
	
#line 3030 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 428 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI247) != NULL);

		if ((*ZI246) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI246));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI247), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 3061 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: 129 */
			{
				{
					p_143 (lex_state, act_state);
					p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
					p_144 (lex_state, act_state);
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
#line 787 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3084 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL6:;
			}
			/* END OF INLINE: 129 */
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
p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI186;

		p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, ZIz, &ZI186);
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

/* BEGINNING OF TRAILER */

#line 836 "src/lx/parser.act"


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

		if (print_progress) {
			act_state->zn = 0;
		}

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state, &ast);

		assert(ast != NULL);

		return ast;
	}

#line 3180 "src/lx/parser.c"

/* END OF FILE */
