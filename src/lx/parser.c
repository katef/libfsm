/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 60 "parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>

	#include <fsm/bool.h>
	#include <fsm/graph.h>

	#include <re/re.h>
	#include "libre/internal.h"	/* XXX */

	#include <adt/xalloc.h>

	#include "lexer.h"
	#include "parser.h"
	#include "ast.h"

	typedef char *               string;
	typedef struct re *          re;
	typedef struct ast_zone *    zone;
	typedef struct ast_mapping * mapping;

	struct act_state {
		int lex_tok;
		int lex_tok_save;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   lex_unknown
	#define ADVANCE_LEXER    do { act_state->lex_tok = lex_nexttoken(lex_state); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                      } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save;  } while (0)

	static void err_expected(const char *token) {
		fprintf(stderr, "syntax error: expected %s\n", token);
		exit(EXIT_FAILURE);
	}

#line 54 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone);
static void p_token_Hmapping_C_Cpattern(lex_state, act_state, re *);
static void p_63(lex_state, act_state);
static void p_token_Hmapping(lex_state, act_state, re *, string *);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state, re *);
static void p_77(lex_state, act_state, ast, zone, ast *, zone *);
static void p_81(lex_state, act_state, re, re *);
static void p_85(lex_state, act_state, ast *, zone *, re *, string *);
extern void p_lx(lex_state, act_state, ast *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI73, zone ZI74)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		ast ZI75;
		zone ZI76;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI73, ZI74);
		p_77 (lex_state, act_state, ZI73, ZI74, &ZI75, &ZI76);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-list */
		{
#line 246 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 100 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		re ZI83;
		string ZI84;

		p_token_Hmapping (lex_state, act_state, &ZI83, &ZI84);
		p_85 (lex_state, act_state, &ZIa, &ZIz, &ZI83, &ZI84);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER (11);
	return;
}

static void
p_token_Hmapping_C_Cpattern(lex_state lex_state, act_state act_state, re *ZOr)
{
	re ZIr;

	switch (CURRENT_TERMINAL) {
	case 1:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_literal */
			{
#line 85 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 149 "parser.c"
			}
			/* END OF EXTRACT: pattern_literal */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 103 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 166 "parser.c"
			}
			/* END OF ACTION: compile-literal */
		}
		break;
	case 0:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_regex */
			{
#line 77 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 185 "parser.c"
			}
			/* END OF EXTRACT: pattern_regex */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 114 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 202 "parser.c"
			}
			/* END OF ACTION: compile-regex */
		}
		break;
	case 11:
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZOr = ZIr;
}

static void
p_63(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 4:
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
#line 226 "parser.act"

		err_expected("';'");
	
#line 244 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_token_Hmapping(lex_state lex_state, act_state act_state, re *ZOr, string *ZOt)
{
	re ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		p_token_Hmapping_C_Cconcaternated_Hpatterns (lex_state, act_state, &ZIr);
		/* BEGINNING OF INLINE: 46 */
		{
			switch (CURRENT_TERMINAL) {
			case 2:
				{
					/* BEGINNING OF INLINE: 47 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case 2:
								break;
							default:
								goto ZL4;
							}
							ADVANCE_LEXER;
						}
						goto ZL3;
					ZL4:;
						{
							/* BEGINNING OF ACTION: err-expected-map */
							{
#line 222 "parser.act"

		err_expected("'->'");
	
#line 286 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL3:;
					}
					/* END OF INLINE: 47 */
					switch (CURRENT_TERMINAL) {
					case 6:
						/* BEGINNING OF EXTRACT: token_name */
						{
#line 93 "parser.act"

		ZIt = xstrdup(lex_tokbuf(lex_state));
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 305 "parser.c"
						}
						/* END OF EXTRACT: token_name */
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
#line 191 "parser.act"

		(ZIt) = NULL;
	
#line 323 "parser.c"
					}
					/* END OF ACTION: no-token */
				}
				break;
			case 11:
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 46 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOt = ZIt;
}

static void
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state, re *ZO80)
{
	re ZI80;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		re ZIr;

		p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIr);
		p_81 (lex_state, act_state, ZIr, &ZI80);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZO80 = ZI80;
}

static void
p_77(lex_state lex_state, act_state act_state, ast ZI73, zone ZI74, ast *ZO75, zone *ZO76)
{
	ast ZI75;
	zone ZI76;

ZL2_77:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI73, ZI74);
			/* BEGINNING OF INLINE: 77 */
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_77;
			}
			/* END OF INLINE: 77 */
		}
		/*UNREACHED*/
	default:
		{
			ZI75 = ZI73;
			ZI76 = ZI74;
		}
		break;
	case 11:
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZO75 = ZI75;
	*ZO76 = ZI76;
}

static void
p_81(lex_state lex_state, act_state act_state, re ZI79, re *ZO80)
{
	re ZI80;

ZL2_81:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			re ZIb;
			re ZIr;

			p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIb);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: concat-regex */
			{
#line 125 "parser.act"

		assert((ZI79) != NULL);
		assert((ZIb) != NULL);

		(ZI79)->fsm = fsm_concat((ZI79)->fsm, (ZIb)->fsm);
		if ((ZI79)->fsm == NULL) {
			perror("re_concat");
			goto ZL1;
		}

		(ZIr) = (ZI79);

		free((ZIb));
	
#line 443 "parser.c"
			}
			/* END OF ACTION: concat-regex */
			/* BEGINNING OF INLINE: 81 */
			ZI79 = ZIr;
			goto ZL2_81;
			/* END OF INLINE: 81 */
		}
		/*UNREACHED*/
	default:
		{
			ZI80 = ZI79;
		}
		break;
	case 11:
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZO80 = ZI80;
}

static void
p_85(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, re *ZI83, string *ZI84)
{
	switch (CURRENT_TERMINAL) {
	case 3:
		{
			re ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 58 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case 3:
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
#line 230 "parser.act"

		err_expected("'..'");
	
#line 498 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 58 */
			p_token_Hmapping (lex_state, act_state, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 148 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 522 "parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 62 */
			{
				switch (CURRENT_TERMINAL) {
				case 4:
					{
						zone ZIx;
						string ZIy;

						p_63 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == 11) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-to */
						{
#line 195 "parser.act"

		(ZIx) = NULL;
	
#line 544 "parser.c"
						}
						/* END OF ACTION: no-to */
						/* BEGINNING OF ACTION: no-token */
						{
#line 191 "parser.act"

		(ZIy) = NULL;
	
#line 553 "parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: complement-regex */
						{
#line 199 "parser.act"

		assert((ZIr2) != NULL);

		if (!fsm_complement((ZIr2)->fsm)) {
			perror("fsm_complement");
			goto ZL5;
		}
	
#line 567 "parser.c"
						}
						/* END OF ACTION: complement-regex */
						/* BEGINNING OF ACTION: make-mapping */
						{
#line 161 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((ZIchild) != NULL);
		assert((ZIchild) != (ZIx));
		assert((ZIr2) != NULL);

		if ((ZIy) != NULL) {
			t = ast_addtoken((*ZIa), (ZIy));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL5;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIchild), (ZIr2), t, (ZIx));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL5;
		}
	
#line 598 "parser.c"
						}
						/* END OF ACTION: make-mapping */
					}
					break;
				case 7:
					{
						/* BEGINNING OF INLINE: 66 */
						{
							{
								switch (CURRENT_TERMINAL) {
								case 7:
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
#line 234 "parser.act"

		err_expected("'{'");
	
#line 625 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 66 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild);
						/* BEGINNING OF INLINE: 67 */
						{
							if ((CURRENT_TERMINAL) == 11) {
								RESTORE_LEXER;
								goto ZL5;
							}
							{
								switch (CURRENT_TERMINAL) {
								case 8:
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
#line 238 "parser.act"

		err_expected("'}'");
	
#line 657 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 67 */
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
#line 246 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 678 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 62 */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 161 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI83) != NULL);

		if ((*ZI84) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI84));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI83), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 713 "parser.c"
			}
			/* END OF ACTION: make-mapping */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 161 "parser.act"

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
	
#line 744 "parser.c"
			}
			/* END OF ACTION: make-mapping */
		}
		break;
	case 4:
		{
			zone ZIto;

			p_63 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-to */
			{
#line 195 "parser.act"

		(ZIto) = NULL;
	
#line 764 "parser.c"
			}
			/* END OF ACTION: no-to */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 161 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI83) != NULL);

		if ((*ZI84) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI84));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI83), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 795 "parser.c"
			}
			/* END OF ACTION: make-mapping */
		}
		break;
	case 11:
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER (11);
	return;
}

void
p_lx(lex_state lex_state, act_state act_state, ast *ZOa)
{
	ast ZIa;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		zone ZIz;

		/* BEGINNING OF ACTION: make-ast */
		{
#line 140 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 832 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 148 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 847 "parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 184 "parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 859 "parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz);
		/* BEGINNING OF INLINE: 71 */
		{
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				switch (CURRENT_TERMINAL) {
				case 10:
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
#line 242 "parser.act"

		err_expected("EOF");
	
#line 887 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 71 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 140 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 908 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 214 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 918 "parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
	goto ZL0;
ZL4:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZOa = ZIa;
}

/* BEGINNING OF TRAILER */

#line 277 "parser.act"


	struct ast *lx_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;
		struct ast *ast;

		assert(f != NULL);

		lex_state = lex_init(f);
		if (lex_state == NULL) {
			perror("lex_init");
			return NULL;
		}

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state, &ast);

		assert(ast != NULL);

		lex_free(lex_state);

		return ast;
	}

#line 962 "parser.c"

/* END OF FILE */
