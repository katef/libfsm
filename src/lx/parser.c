/*
 * Automatically generated from the files:
 *	src/lx/parser.sid
 * and
 *	src/lx/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 128 "src/lx/parser.act"


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
static void p_144(lex_state, act_state);
static void p_145(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_159(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_165(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_pattern_C_Cbody(lex_state, act_state);
static void p_171(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_expr_C_Cprimary_Hexpr(lex_state, act_state, zone, fsm *);
static void p_182(lex_state, act_state, fsm, fsm *);
static void p_expr_C_Cpostfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cprefix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_190(lex_state, act_state, zone *, fsm *, fsm *);
static void p_193(lex_state, act_state, zone *, fsm *, fsm *);
static void p_expr(lex_state, act_state, zone, fsm *);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_196(lex_state, act_state, zone *, fsm *, fsm *);
static void p_200(lex_state, act_state, fsm *);
static void p_expr_C_Cinfix_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state, act_state, zone, fsm *);
static void p_227(lex_state, act_state, ast *, zone *, fsm *, string *);
extern void p_lx(lex_state, act_state, ast *);
static void p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr(lex_state, act_state, zone, fsm *);
static void p_111(lex_state, act_state, string *);
static void p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state, act_state, zone, fsm *);
static void p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state, act_state, zone, fsm *);
static void p_121(lex_state, act_state);
static void p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr(lex_state, act_state, zone, fsm *);
static void p_250(lex_state, act_state, ast *, zone *, string *, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI153, zone ZI154, fsm ZI155)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI156;
		zone ZI157;
		fsm ZI158;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI153, ZI154, ZI155);
		p_159 (lex_state, act_state, ZI153, ZI154, ZI155, &ZI156, &ZI157, &ZI158);
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
#line 831 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 187 "src/lx/parser.c"
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
#line 233 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 213 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-var */
			{
#line 288 "src/lx/parser.act"

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
	
#line 246 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
		}
		break;
	case (TOK_TOKEN):
		{
			string ZIt;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 225 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 266 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 316 "src/lx/parser.act"

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
			fsm_state_t base_r, base_fsm;
			fsm_state_t ms;

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

			/*
			 * We don't want to take these with us, because the output here is a
			 * general-purpose FSM for which no mapping has yet been made,
			 * so that it can be used in expressions as for any pattern.
			 */
			fsm_setendopaque(fsm, NULL);

			(ZIr) = fsm_merge((ZIr), fsm, &base_r, &base_fsm);
			if ((ZIr) == NULL) {
				perror("fsm_union");
				goto ZL1;
			}

			ms    += base_fsm;
			start += base_r;

			if (!fsm_addedge_epsilon((ZIr), start, ms)) {
				perror("fsm_addedge_epsilon");
				goto ZL1;
			}
		}

		fsm_setstart((ZIr), start);
	
#line 336 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			p_pattern_C_Cbody (lex_state, act_state);
			p_200 (lex_state, act_state, &ZIr);
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
p_144(lex_state lex_state, act_state act_state)
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
#line 815 "src/lx/parser.act"

		err_expected(lex_state, "'{'");
	
#line 389 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-open */
	}
}

static void
p_145(lex_state lex_state, act_state act_state)
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
#line 819 "src/lx/parser.act"

		err_expected(lex_state, "'}'");
	
#line 419 "src/lx/parser.c"
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
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI208;
			zone ZI169;
			fsm ZIq;
			zone ZI209;
			fsm ZI210;
			string ZI211;
			fsm ZI212;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI194);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-reverse */
			{
#line 692 "src/lx/parser.act"

		assert((ZI194) != NULL);

		if (!fsm_reverse((ZI194))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 459 "src/lx/parser.c"
			}
			/* END OF ACTION: op-reverse */
			p_196 (lex_state, act_state, &ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, &ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, &ZIz, &ZI188, &ZI208);
			p_171 (lex_state, act_state, ZIz, ZI208, &ZI169, &ZIq);
			p_165 (lex_state, act_state, ZIz, ZIq, &ZI209, &ZI210);
			p_111 (lex_state, act_state, &ZI211);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

		assert((ZI210) != NULL);

		if ((ZIexit) == NULL) {
			(ZI212) = (ZI210);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI212) = fsm_subtract((ZI210), (ZIexit));
			if ((ZI212) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 494 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, &ZIa, &ZIz, &ZI211, &ZI212);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HAT):
		{
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI214;
			zone ZI169;
			fsm ZIq;
			zone ZI215;
			fsm ZI216;
			string ZI217;
			fsm ZI218;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI194);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complete */
			{
#line 683 "src/lx/parser.act"

		assert((ZI194) != NULL);

		if (!fsm_complete((ZI194), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 534 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complete */
			p_196 (lex_state, act_state, &ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, &ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, &ZIz, &ZI188, &ZI214);
			p_171 (lex_state, act_state, ZIz, ZI214, &ZI169, &ZIq);
			p_165 (lex_state, act_state, ZIz, ZIq, &ZI215, &ZI216);
			p_111 (lex_state, act_state, &ZI217);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

		assert((ZI216) != NULL);

		if ((ZIexit) == NULL) {
			(ZI218) = (ZI216);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI218) = fsm_subtract((ZI216), (ZIexit));
			if ((ZI218) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 569 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, &ZIa, &ZIz, &ZI217, &ZI218);
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
#line 233 "src/lx/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 593 "src/lx/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_227 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_LPAREN):
		{
			fsm ZI220;
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI221;
			zone ZI169;
			fsm ZIq;
			zone ZI222;
			fsm ZI223;
			string ZI224;
			fsm ZI225;

			ADVANCE_LEXER;
			p_expr (lex_state, act_state, ZIz, &ZI220);
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
			p_182 (lex_state, act_state, ZI220, &ZI194);
			p_196 (lex_state, act_state, &ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, &ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, &ZIz, &ZI188, &ZI221);
			p_171 (lex_state, act_state, ZIz, ZI221, &ZI169, &ZIq);
			p_165 (lex_state, act_state, ZIz, ZIq, &ZI222, &ZI223);
			p_111 (lex_state, act_state, &ZI224);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

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
	
#line 663 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, &ZIa, &ZIz, &ZI224, &ZI225);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TILDE):
		{
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI202;
			zone ZI169;
			fsm ZIq;
			zone ZI203;
			fsm ZI204;
			string ZI205;
			fsm ZI206;

			ADVANCE_LEXER;
			p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI194);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-complement */
			{
#line 674 "src/lx/parser.act"

		assert((ZI194) != NULL);

		if (!fsm_complement((ZI194))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 703 "src/lx/parser.c"
			}
			/* END OF ACTION: op-complement */
			p_196 (lex_state, act_state, &ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, &ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, &ZIz, &ZI188, &ZI202);
			p_171 (lex_state, act_state, ZIz, ZI202, &ZI169, &ZIq);
			p_165 (lex_state, act_state, ZIz, ZIq, &ZI203, &ZI204);
			p_111 (lex_state, act_state, &ZI205);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

		assert((ZI204) != NULL);

		if ((ZIexit) == NULL) {
			(ZI206) = (ZI204);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI206) = fsm_subtract((ZI204), (ZIexit));
			if ((ZI206) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 738 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, &ZIa, &ZIz, &ZI205, &ZI206);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_TOKEN):
		{
			string ZI228;
			fsm ZI229;
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI230;
			zone ZI169;
			fsm ZIq;
			zone ZI231;
			fsm ZI232;
			string ZI233;
			fsm ZI234;

			/* BEGINNING OF EXTRACT: TOKEN */
			{
#line 225 "src/lx/parser.act"

		/* TODO: submatch addressing */
		ZI228 = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZI228 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 774 "src/lx/parser.c"
			}
			/* END OF EXTRACT: TOKEN */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-token */
			{
#line 316 "src/lx/parser.act"

		const struct ast_mapping *m;
		fsm_state_t start;

		assert((ZI228) != NULL);
		assert((ZI228) != NULL);

		(ZI229) = fsm_new(act_state->opt);
		if ((ZI229) == NULL) {
			perror("fsm_new");
			goto ZL1;
		}

		if (!fsm_addstate((ZI229), &start)) {
			perror("fsm_addstate");
			goto ZL1;
		}

		for (m = (ZIz)->ml; m != NULL; m = m->next) {
			struct fsm *fsm;
			fsm_state_t base_r, base_fsm;
			fsm_state_t ms;

			if (m->token == NULL) {
				continue;
			}

			if (0 != strcmp(m->token->s, (ZI228))) {
				continue;
			}

			fsm = fsm_clone(m->fsm);
			if (fsm == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(void) fsm_getstart(fsm, &ms);

			/*
			 * We don't want to take these with us, because the output here is a
			 * general-purpose FSM for which no mapping has yet been made,
			 * so that it can be used in expressions as for any pattern.
			 */
			fsm_setendopaque(fsm, NULL);

			(ZI229) = fsm_merge((ZI229), fsm, &base_r, &base_fsm);
			if ((ZI229) == NULL) {
				perror("fsm_union");
				goto ZL1;
			}

			ms    += base_fsm;
			start += base_r;

			if (!fsm_addedge_epsilon((ZI229), start, ms)) {
				perror("fsm_addedge_epsilon");
				goto ZL1;
			}
		}

		fsm_setstart((ZI229), start);
	
#line 844 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-token */
			p_182 (lex_state, act_state, ZI229, &ZI194);
			p_196 (lex_state, act_state, &ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, &ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, &ZIz, &ZI188, &ZI230);
			p_171 (lex_state, act_state, ZIz, ZI230, &ZI169, &ZIq);
			p_165 (lex_state, act_state, ZIz, ZIq, &ZI231, &ZI232);
			p_111 (lex_state, act_state, &ZI233);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

		assert((ZI232) != NULL);

		if ((ZIexit) == NULL) {
			(ZI234) = (ZI232);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI234) = fsm_subtract((ZI232), (ZIexit));
			if ((ZI234) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 880 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, &ZIa, &ZIz, &ZI233, &ZI234);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_RE): case (TOK_STR):
		{
			fsm ZI236;
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI237;
			zone ZI169;
			fsm ZIq;
			zone ZI238;
			fsm ZI239;
			string ZI240;
			fsm ZI241;

			p_pattern_C_Cbody (lex_state, act_state);
			p_200 (lex_state, act_state, &ZI236);
			p_182 (lex_state, act_state, ZI236, &ZI194);
			p_196 (lex_state, act_state, &ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, &ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, &ZIz, &ZI188, &ZI237);
			p_171 (lex_state, act_state, ZIz, ZI237, &ZI169, &ZIq);
			p_165 (lex_state, act_state, ZIz, ZIq, &ZI238, &ZI239);
			p_111 (lex_state, act_state, &ZI240);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

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
	
#line 940 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, &ZIa, &ZIz, &ZI240, &ZI241);
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
#line 827 "src/lx/parser.act"

		err_expected(lex_state, "mapping, binding or zone");
	
#line 964 "src/lx/parser.c"
		}
		/* END OF ACTION: err-expected-thing */
	}
}

static void
p_159(lex_state lex_state, act_state act_state, ast ZI153, zone ZI154, fsm ZI155, ast *ZO156, zone *ZO157, fsm *ZO158)
{
	ast ZI156;
	zone ZI157;
	fsm ZI158;

ZL2_159:;
	switch (CURRENT_TERMINAL) {
	case (TOK_TOKEN): case (TOK_IDENT): case (TOK_ESC): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_RE): case (TOK_STR):
	case (TOK_LPAREN): case (TOK_TILDE): case (TOK_BANG): case (TOK_HAT):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI153, ZI154, ZI155);
			/* BEGINNING OF INLINE: 159 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_159;
			}
			/* END OF INLINE: 159 */
		}
		/*UNREACHED*/
	default:
		{
			ZI156 = ZI153;
			ZI157 = ZI154;
			ZI158 = ZI155;
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
	*ZO158 = ZI158;
}

static void
p_165(lex_state lex_state, act_state act_state, zone ZI161, fsm ZI162, zone *ZO163, fsm *ZO164)
{
	zone ZI163;
	fsm ZI164;

ZL2_165:;
	switch (CURRENT_TERMINAL) {
	case (TOK_PIPE):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI161, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-alt */
			{
#line 740 "src/lx/parser.act"

		assert((ZI162) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_union((ZI162), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_union");
			goto ZL1;
		}
	
#line 1046 "src/lx/parser.c"
			}
			/* END OF ACTION: op-alt */
			/* BEGINNING OF INLINE: 165 */
			ZI162 = ZIq;
			goto ZL2_165;
			/* END OF INLINE: 165 */
		}
		/*UNREACHED*/
	default:
		{
			ZI163 = ZI161;
			ZI164 = ZI162;
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
	*ZO163 = ZI163;
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

			/* BEGINNING OF INLINE: 83 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 220 "src/lx/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 1096 "src/lx/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_ESC):
					{
						/* BEGINNING OF EXTRACT: ESC */
						{
#line 150 "src/lx/parser.act"

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
	
#line 1124 "src/lx/parser.c"
						}
						/* END OF EXTRACT: ESC */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						/* BEGINNING OF EXTRACT: HEX */
						{
#line 213 "src/lx/parser.act"

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
	
#line 1161 "src/lx/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						/* BEGINNING OF EXTRACT: OCT */
						{
#line 186 "src/lx/parser.act"

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
	
#line 1198 "src/lx/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 83 */
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 257 "src/lx/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 1216 "src/lx/parser.c"
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
p_171(lex_state lex_state, act_state act_state, zone ZI167, fsm ZI168, zone *ZO169, fsm *ZO170)
{
	zone ZI169;
	fsm ZI170;

ZL2_171:;
	switch (CURRENT_TERMINAL) {
	case (TOK_AND):
		{
			fsm ZIb;
			fsm ZIq;

			ADVANCE_LEXER;
			p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI167, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-intersect */
			{
#line 729 "src/lx/parser.act"

		assert((ZI168) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_intersect((ZI168), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_intersect");
			goto ZL1;
		}
	
#line 1267 "src/lx/parser.c"
			}
			/* END OF ACTION: op-intersect */
			/* BEGINNING OF INLINE: 171 */
			ZI168 = ZIq;
			goto ZL2_171;
			/* END OF INLINE: 171 */
		}
		/*UNREACHED*/
	default:
		{
			ZI169 = ZI167;
			ZI170 = ZI168;
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
	*ZO169 = ZI169;
	*ZO170 = ZI170;
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
p_182(lex_state lex_state, act_state act_state, fsm ZI176, fsm *ZO181)
{
	fsm ZI181;

ZL2_182:;
	switch (CURRENT_TERMINAL) {
	case (TOK_STAR): case (TOK_CROSS): case (TOK_QMARK):
		{
			fsm ZIq;

			ZIq = ZI176;
			/* BEGINNING OF INLINE: 243 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CROSS):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-cross */
						{
#line 590 "src/lx/parser.act"

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
	
#line 1404 "src/lx/parser.c"
						}
						/* END OF ACTION: op-cross */
						/* BEGINNING OF INLINE: 182 */
						ZI176 = ZIq;
						goto ZL2_182;
						/* END OF INLINE: 182 */
					}
					/*UNREACHED*/
				case (TOK_QMARK):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-qmark */
						{
#line 635 "src/lx/parser.act"

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
	
#line 1463 "src/lx/parser.c"
						}
						/* END OF ACTION: op-qmark */
						/* BEGINNING OF INLINE: 182 */
						ZI176 = ZIq;
						goto ZL2_182;
						/* END OF INLINE: 182 */
					}
					/*UNREACHED*/
				case (TOK_STAR):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: op-star */
						{
#line 540 "src/lx/parser.act"

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
	
#line 1527 "src/lx/parser.c"
						}
						/* END OF ACTION: op-star */
						/* BEGINNING OF INLINE: 182 */
						ZI176 = ZIq;
						goto ZL2_182;
						/* END OF INLINE: 182 */
					}
					/*UNREACHED*/
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 243 */
		}
		/*UNREACHED*/
	default:
		{
			ZI181 = ZI176;
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
}

static void
p_expr_C_Cpostfix_Hexpr(lex_state lex_state, act_state act_state, zone ZI173, fsm *ZO181)
{
	fsm ZI181;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_expr_C_Cprimary_Hexpr (lex_state, act_state, ZI173, &ZIq);
		p_182 (lex_state, act_state, ZIq, &ZI181);
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
	*ZO181 = ZI181;
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
#line 692 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_reverse((ZIq))) {
			perror("fsm_reverse");
			goto ZL1;
		}
	
#line 1610 "src/lx/parser.c"
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
#line 683 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complete((ZIq), fsm_isany)) {
			perror("fsm_complete");
			goto ZL1;
		}
	
#line 1634 "src/lx/parser.c"
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
#line 674 "src/lx/parser.act"

		assert((ZIq) != NULL);

		if (!fsm_complement((ZIq))) {
			perror("fsm_complement");
			goto ZL1;
		}
	
#line 1658 "src/lx/parser.c"
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
p_190(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI188, fsm *ZOq)
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
#line 718 "src/lx/parser.act"

		assert((*ZI188) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_subtract((*ZI188), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_subtract");
			goto ZL1;
		}
	
#line 1716 "src/lx/parser.c"
			}
			/* END OF ACTION: op-subtract */
		}
		break;
	default:
		{
			ZIq = *ZI188;
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
p_193(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI191, fsm *ZOq)
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
#line 701 "src/lx/parser.act"

		assert((*ZI191) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((*ZI191), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 1767 "src/lx/parser.c"
			}
			/* END OF ACTION: op-concat */
		}
		break;
	default:
		{
			ZIq = *ZI191;
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
p_token_Hmapping(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOr, string *ZOt)
{
	fsm ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr (lex_state, act_state, ZIz, &ZIr);
		p_111 (lex_state, act_state, &ZIt);
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
p_196(lex_state lex_state, act_state act_state, zone *ZIz, fsm *ZI194, fsm *ZOq)
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
#line 713 "src/lx/parser.act"

		fprintf(stderr, "unimplemented\n");
		(ZIq) = NULL;
		goto ZL1;
	
#line 1861 "src/lx/parser.c"
			}
			/* END OF ACTION: op-product */
		}
		break;
	default:
		{
			ZIq = *ZI194;
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
p_200(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RE):
		{
			flags ZIf;
			arr ZIa;

			/* BEGINNING OF EXTRACT: RE */
			{
#line 245 "src/lx/parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */

		if (-1 == re_flags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			err(lex_state, "/%s: Unrecognised regexp flags", lex_state->buf.a + 1);
			exit(EXIT_FAILURE);
		}

		ZIf |= RE_ANCHORED; /* regexps in lx are implicitly anchored */
	
#line 1908 "src/lx/parser.c"
			}
			/* END OF EXTRACT: RE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 271 "src/lx/parser.act"

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
	
#line 1939 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 394 "src/lx/parser.act"

		struct re_err err;

		assert((ZIa) != NULL);
		assert((ZIa)->p != NULL);

		(ZIr) = re_comp_new(RE_NATIVE, act_agetc, (ZIa), act_state->opt, (ZIf), &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_NATIVE, &err, NULL, (ZIa)->p);
			exit(EXIT_FAILURE);
		}
	
#line 1959 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-regex */
			/* BEGINNING OF ACTION: free-arr */
			{
#line 781 "src/lx/parser.act"

		free((ZIa));
	
#line 1968 "src/lx/parser.c"
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
#line 271 "src/lx/parser.act"

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
	
#line 2005 "src/lx/parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 379 "src/lx/parser.act"

		struct re_err err;

		assert((ZIa) != NULL);
		assert((ZIa)->p != NULL);

		(ZIr) = re_comp_new(RE_LITERAL, act_agetc, (ZIa), act_state->opt, 0, &err);
		if ((ZIr) == NULL) {
			assert(err.e != RE_EBADDIALECT);
			/* TODO: pass filename for .lx source */
			re_perror(RE_LITERAL, &err, NULL, (ZIa)->p); /* XXX */
			exit(EXIT_FAILURE);
		}
	
#line 2025 "src/lx/parser.c"
			}
			/* END OF ACTION: compile-literal */
			/* BEGINNING OF ACTION: free-arr */
			{
#line 781 "src/lx/parser.act"

		free((ZIa));
	
#line 2034 "src/lx/parser.c"
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
p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr(lex_state lex_state, act_state act_state, zone ZI167, fsm *ZO170)
{
	fsm ZI170;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI169;

		p_expr_C_Cinfix_Hexpr_C_Csub_Hexpr (lex_state, act_state, ZI167, &ZIq);
		p_171 (lex_state, act_state, ZI167, ZIq, &ZI169, &ZI170);
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
	*ZO170 = ZI170;
}

static void
p_227(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BIND):
		{
			fsm ZIr;
			fsm ZIq;

			/* BEGINNING OF INLINE: 120 */
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
#line 803 "src/lx/parser.act"

		err_expected(lex_state, "'='");
	
#line 2131 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-bind */
				}
			ZL2:;
			}
			/* END OF INLINE: 120 */
			p_expr (lex_state, act_state, *ZIz, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

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
		}
	
#line 2165 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_121 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 500 "src/lx/parser.act"

		struct var *v;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((ZIq) != NULL);
		assert((*ZIn) != NULL);

		(void) (*ZIa);

		v = var_bind(&(*ZIz)->vl, (*ZIn), (ZIq));
		if (v == NULL) {
			perror("var_bind");
			goto ZL1;
		}
	
#line 2192 "src/lx/parser.c"
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
			fsm ZI244;
			fsm ZI194;
			fsm ZI191;
			fsm ZI188;
			fsm ZI245;
			zone ZI169;
			fsm ZIq;
			zone ZI246;
			fsm ZI247;
			string ZI248;
			fsm ZI249;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 288 "src/lx/parser.act"

		struct ast_zone *z;

		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);

		for (z = (*ZIz); z != NULL; z = z->parent) {
			(ZI244) = var_find(z->vl, (*ZIn));
			if ((ZI244) != NULL) {
				break;
			}

			if ((*ZIz)->parent == NULL) {
				/* TODO: use *err */
				err(lex_state, "No such variable: %s", (*ZIn));
				goto ZL1;
			}
		}

		(ZI244) = fsm_clone((ZI244));
		if ((ZI244) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 2245 "src/lx/parser.c"
			}
			/* END OF ACTION: deref-var */
			p_182 (lex_state, act_state, ZI244, &ZI194);
			p_196 (lex_state, act_state, ZIz, &ZI194, &ZI191);
			p_193 (lex_state, act_state, ZIz, &ZI191, &ZI188);
			p_190 (lex_state, act_state, ZIz, &ZI188, &ZI245);
			p_171 (lex_state, act_state, *ZIz, ZI245, &ZI169, &ZIq);
			p_165 (lex_state, act_state, *ZIz, ZIq, &ZI246, &ZI247);
			p_111 (lex_state, act_state, &ZI248);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 751 "src/lx/parser.act"

		assert((ZI247) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI249) = (ZI247);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI249) = fsm_subtract((ZI247), (*ZIexit));
			if ((ZI249) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}
		}
	
#line 2281 "src/lx/parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_250 (lex_state, act_state, ZIa, ZIz, &ZI248, &ZI249);
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
#line 530 "src/lx/parser.act"

		(ZIparent) = NULL;
	
#line 2321 "src/lx/parser.c"
		}
		/* END OF ACTION: no-zone */
		/* BEGINNING OF ACTION: make-ast */
		{
#line 439 "src/lx/parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 2334 "src/lx/parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 447 "src/lx/parser.act"

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
	
#line 2357 "src/lx/parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 526 "src/lx/parser.act"

		(ZIexit) = NULL;
	
#line 2366 "src/lx/parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 515 "src/lx/parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 2378 "src/lx/parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 150 */
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
#line 823 "src/lx/parser.act"

		err_expected(lex_state, "EOF");
	
#line 2406 "src/lx/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 150 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 439 "src/lx/parser.act"

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
#line 791 "src/lx/parser.act"

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
		fsm ZI191;

		p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr (lex_state, act_state, ZIz, &ZI191);
		p_193 (lex_state, act_state, &ZIz, &ZI191, &ZIq);
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
p_111(lex_state lex_state, act_state act_state, string *ZOt)
{
	string ZIt;

	switch (CURRENT_TERMINAL) {
	case (TOK_MAP):
		{
			/* BEGINNING OF INLINE: 112 */
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
#line 799 "src/lx/parser.act"

		err_expected(lex_state, "'->'");
	
#line 2503 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-map */
				}
			ZL2:;
			}
			/* END OF INLINE: 112 */
			switch (CURRENT_TERMINAL) {
			case (TOK_TOKEN):
				/* BEGINNING OF EXTRACT: TOKEN */
				{
#line 225 "src/lx/parser.act"

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
#line 522 "src/lx/parser.act"

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
p_expr_C_Cinfix_Hexpr_C_Calt_Hexpr(lex_state lex_state, act_state act_state, zone ZI161, fsm *ZO164)
{
	fsm ZI164;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI163;

		p_expr_C_Cinfix_Hexpr_C_Cand_Hexpr (lex_state, act_state, ZI161, &ZIq);
		p_165 (lex_state, act_state, ZI161, ZIq, &ZI163, &ZI164);
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
p_expr_C_Cinfix_Hexpr_C_Cdot_Hexpr(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOq)
{
	fsm ZIq;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI194;

		p_expr_C_Cprefix_Hexpr (lex_state, act_state, ZIz, &ZI194);
		p_196 (lex_state, act_state, &ZIz, &ZI194, &ZIq);
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
p_121(lex_state lex_state, act_state act_state)
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
#line 807 "src/lx/parser.act"

		err_expected(lex_state, "';'");
	
#line 2634 "src/lx/parser.c"
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
		fsm ZI188;

		p_expr_C_Cinfix_Hexpr_C_Ccat_Hexpr (lex_state, act_state, ZIz, &ZI188);
		p_190 (lex_state, act_state, &ZIz, &ZI188, &ZIq);
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
p_250(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI248, fsm *ZI249)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_SEMI):
		{
			zone ZIto;

			p_121 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-zone */
			{
#line 530 "src/lx/parser.act"

		(ZIto) = NULL;
	
#line 2685 "src/lx/parser.c"
			}
			/* END OF ACTION: no-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 468 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI249) != NULL);

		if ((*ZI248) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI248));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI249), t, (ZIto));
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
	
#line 2723 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 135 */
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
#line 811 "src/lx/parser.act"

		err_expected(lex_state, "'..'");
	
#line 2754 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 135 */
			p_token_Hmapping (lex_state, act_state, *ZIz, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 447 "src/lx/parser.act"

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
	
#line 2786 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 468 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI249) != NULL);

		if ((*ZI248) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI248));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI249), t, (ZIchild));
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
	
#line 2824 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 468 "src/lx/parser.act"

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

		if (print_progress) {
			if (important(nmappings)) {
				fprintf(stderr, " m%u", nmappings);
			}
			nmappings++;
		}
	
#line 2862 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: 137 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_121 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-zone */
						{
#line 530 "src/lx/parser.act"

		(ZIx) = NULL;
	
#line 2887 "src/lx/parser.c"
						}
						/* END OF ACTION: no-zone */
						/* BEGINNING OF ACTION: no-token */
						{
#line 522 "src/lx/parser.act"

		(ZIy) = NULL;
	
#line 2896 "src/lx/parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 771 "src/lx/parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 2911 "src/lx/parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 410 "src/lx/parser.act"

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
	
#line 2947 "src/lx/parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 751 "src/lx/parser.act"

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
		}
	
#line 2972 "src/lx/parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 468 "src/lx/parser.act"

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
	
#line 3010 "src/lx/parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						p_144 (lex_state, act_state);
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						p_145 (lex_state, act_state);
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
#line 831 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3038 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 137 */
		}
		break;
	case (TOK_OPEN):
		{
			fsm ZIr2;
			zone ZIchild;

			/* BEGINNING OF ACTION: no-exit */
			{
#line 526 "src/lx/parser.act"

		(ZIr2) = NULL;
	
#line 3058 "src/lx/parser.c"
			}
			/* END OF ACTION: no-exit */
			/* BEGINNING OF ACTION: make-zone */
			{
#line 447 "src/lx/parser.act"

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
	
#line 3081 "src/lx/parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 468 "src/lx/parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI249) != NULL);

		if ((*ZI248) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI248));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI249), t, (ZIchild));
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
	
#line 3119 "src/lx/parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF INLINE: 130 */
			{
				{
					p_144 (lex_state, act_state);
					p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
					p_145 (lex_state, act_state);
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
#line 831 "src/lx/parser.act"

		err_expected(lex_state, "list of mappings, bindings or zones");
	
#line 3142 "src/lx/parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL6:;
			}
			/* END OF INLINE: 130 */
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

#line 883 "src/lx/parser.act"


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

		assert(ast != NULL);

		return ast;
	}

#line 3215 "src/lx/parser.c"

/* END OF FILE */
