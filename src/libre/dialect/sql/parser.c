/*
 * Automatically generated from the files:
 *	src/libre/dialect/sql/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 131 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include "libfsm/internal.h" /* XXX */

	#include <fsm/fsm.h>
	#include <fsm/bool.h>
	#include <fsm/pred.h>

	#include <re/re.h>

	#ifndef DIALECT
	#error DIALECT required
	#endif

	#define PASTE(a, b) a ## b
	#define CAT(a, b)   PASTE(a, b)

	#define LX_PREFIX CAT(lx_, DIALECT)

	#define LX_TOKEN   CAT(LX_PREFIX, _token)
	#define LX_STATE   CAT(LX_PREFIX, _lx)
	#define LX_NEXT    CAT(LX_PREFIX, _next)
	#define LX_INIT    CAT(LX_PREFIX, _init)

	#define DIALECT_COMP  CAT(comp_, DIALECT)

	/* XXX: get rid of this; use same %entry% for all grammars */
	#define DIALECT_ENTRY CAT(p_re__, DIALECT)

	#define TOK_CLASS__alnum  TOK_CLASS_ALNUM
	#define TOK_CLASS__alpha  TOK_CLASS_ALPHA
	#define TOK_CLASS__ascii  TOK_CLASS_ASCII
	#define TOK_CLASS__blank  TOK_CLASS_BLANK
	#define TOK_CLASS__cntrl  TOK_CLASS_CNTRL
	#define TOK_CLASS__digit  TOK_CLASS_DIGIT
	#define TOK_CLASS__graph  TOK_CLASS_GRAPH
	#define TOK_CLASS__lower  TOK_CLASS_LOWER
	#define TOK_CLASS__print  TOK_CLASS_PRINT
	#define TOK_CLASS__punct  TOK_CLASS_PUNCT
	#define TOK_CLASS__space  TOK_CLASS_SPACE
	#define TOK_CLASS__spchr  TOK_CLASS_SPCHR
	#define TOK_CLASS__upper  TOK_CLASS_UPPER
	#define TOK_CLASS__word   TOK_CLASS_WORD
	#define TOK_CLASS__xdigit TOK_CLASS_XDIGIT

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"

	struct grp {
		struct fsm *set;
		struct fsm *dup;
	};

	typedef char         t_char;
	typedef const char * t_class;
	typedef unsigned     t_unsigned;
	typedef unsigned     t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef struct fsm * t_fsm;
	typedef struct fsm_state * t_fsm__state;
	typedef struct grp t_grp;

	struct act_state {
		enum LX_TOKEN lex_tok;
		enum LX_TOKEN lex_tok_save;
		int overlap; /* permit overlap in groups */

		/*
		 * Lexical position stored for syntax errors.
		 */
		struct re_pos synstart;
		struct re_pos synend;

		/*
		 * Lexical positions stored for errors which describe multiple tokens.
		 * We're able to store these without needing a stack, because these are
		 * non-recursive productions.
		 */
		struct re_pos groupstart; struct re_pos groupend;
		struct re_pos rangestart; struct re_pos rangeend;
		struct re_pos countstart; struct re_pos countend;
	};

	struct lex_state {
		struct LX_STATE lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		int (*f)(void *opaque);
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { mark(&act_state->synstart, &lex_state->lx.start); \
	                              mark(&act_state->synend,   &lex_state->lx.end);   \
	                              act_state->lex_tok = LX_NEXT(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void
	mark(struct re_pos *r, const struct lx_pos *pos)
	{
		assert(r != NULL);
		assert(pos != NULL);

		r->byte = pos->byte;
	}

	/* TODO: centralise perhaps */
	static void
	snprintdots(char *s, size_t sz, const char *msg)
	{
		size_t n;

		assert(s != NULL);
		assert(sz > 3);
		assert(msg != NULL);

		n = sprintf(s, "%.*s", (int) sz - 3 - 1, msg);
		if (n == sz - 3 - 1) {
			strcpy(s + sz, "...");
		}
	}

	/* TODO: centralise */
	/* XXX: escaping really depends on dialect */
	static const char *
	escchar(char *s, size_t sz, int c)
	{
		size_t i;

		const struct {
			int c;
			const char *s;
		} a[] = {
			{ '\\', "\\\\" },

			{ '^',  "\\^"  },
			{ '-',  "\\-"  },
			{ ']',  "\\]"  },
			{ '[',  "\\["  },

			{ '\f', "\\f"  },
			{ '\n', "\\n"  },
			{ '\r', "\\r"  },
			{ '\t', "\\t"  },
			{ '\v', "\\v"  }
		};

		assert(s != NULL);
		assert(sz >= 5);

		(void) sz;

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].c == c) {
				return a[i].s;
			}
		}

		if (!isprint((unsigned char) c)) {
			sprintf(s, "\\x%02X", (unsigned char) c);
			return s;
		}

		sprintf(s, "%c", c);
		return s;
	}

	static int
	escputc(int c, FILE *f)
	{
		char s[5];

		return fputs(escchar(s, sizeof s, c), f);
	}

	static int
	addedge_literal(struct fsm *fsm, enum re_flags flags,
		struct fsm_state *from, struct fsm_state *to, char c)
	{
		assert(fsm != NULL);
		assert(from != NULL);
		assert(to != NULL);

		if (flags & RE_ICASE) {
			if (!fsm_addedge_literal(fsm, from, to, tolower((unsigned char) c))) {
				return 0;
			}

			if (!fsm_addedge_literal(fsm, from, to, toupper((unsigned char) c))) {
				return 0;
			}
		} else {
			if (!fsm_addedge_literal(fsm, from, to, c)) {
				return 0;
			}
		}

		return 1;
	}

	static int
	group_add(struct grp *g, enum re_flags flags, char c)
	{
		const struct fsm_state *p;
		struct fsm_state *start, *end;
		struct fsm *fsm;
		char a[2];
		char *s = a;

		assert(g != NULL);

		a[0] = c;
		a[1] = '\0';

		errno = 0;
		p = fsm_exec(g->set, fsm_sgetc, &s);
		if (p == NULL && errno != 0) {
			return -1;
		}

		if (p == NULL) {
			fsm = g->set;
		} else {
			fsm = g->dup;
		}

		start = fsm_getstart(fsm);
		assert(start != NULL);

		end = fsm_addstate(fsm);
		if (end == NULL) {
			return -1;
		}

		fsm_setend(fsm, end, 1);

		if (!addedge_literal(fsm, flags, start, end, c)) {
			return -1;
		}

		return 0;
	}

	static struct fsm *
	fsm_new_blank(const struct fsm_options *opt)
	{
		struct fsm *new;
		struct fsm_state *start;

		new = fsm_new(opt);
		if (new == NULL) {
			return NULL;
		}

		start = fsm_addstate(new);
		if (start == NULL) {
			goto error;
		}

		fsm_setstart(new, start);

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

	static struct fsm *
	fsm_new_any(const struct fsm_options *opt)
	{
		struct fsm_state *a, *b;
		struct fsm *new;

		assert(opt != NULL);

		new = fsm_new(opt);
		if (new == NULL) {
			return NULL;
		}

		a = fsm_addstate(new);
		if (a == NULL) {
			goto error;
		}

		b = fsm_addstate(new);
		if (b == NULL) {
			goto error;
		}

		/* TODO: provide as a class. for utf8, this would be /./ */
		if (!fsm_addedge_any(new, a, b)) {
			goto error;
		}

		fsm_setstart(new, a);
		fsm_setend(new, b, 1);

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

	/* XXX: to go when dups show all spellings for group overlap */
	static const struct fsm_state *
	fsm_any(const struct fsm *fsm,
		int (*predicate)(const struct fsm *, const struct fsm_state *))
	{
		const struct fsm_state *s;

		assert(fsm != NULL);
		assert(predicate != NULL);

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!predicate(fsm, s)) {
				return s;
			}
		}

		return NULL;
	}

#line 362 "src/libre/dialect/sql/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__sql(fsm, flags, lex_state, act_state, err);
static void p_expr_C_Clist_Hof_Halts_C_Calt(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_155(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_160(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_164(fsm, flags, lex_state, act_state, err, t_grp *, t_char *, t_pos *);
static void p_group(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbody(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr_C_Clist_Hof_Hatoms(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbm(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr_C_Clist_Hof_Halts(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_expr_C_Catom(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state *);
static void p_literal(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

void
p_re__sql(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 574 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 418 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 144 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
			case (TOK_CHAR):
				{
					p_expr (fsm, flags, lex_state, act_state, err, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL3;
					}
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 808 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 444 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-alts */
				{
#line 992 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
	
#line 461 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 144 */
		/* BEGINNING OF INLINE: 145 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 1016 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
	
#line 490 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 145 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Clist_Hof_Halts_C_Calt(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 801 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 522 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-epsilon */
		{
#line 808 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIz))) {
			goto ZL1;
		}
	
#line 533 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: add-epsilon */
		p_expr_C_Clist_Hof_Hatoms (fsm, flags, lex_state, act_state, err, ZIz, ZIy);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_155(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZI151, t_fsm__state ZI152, t_fsm__state *ZO153, t_fsm__state *ZO154)
{
	t_fsm__state ZI153;
	t_fsm__state ZI154;

ZL2_155:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts_C_Calt (fsm, flags, lex_state, act_state, err, ZI151, ZI152);
			/* BEGINNING OF INLINE: 155 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_155;
			}
			/* END OF INLINE: 155 */
		}
		/* UNREACHED */
	default:
		{
			ZI153 = ZI151;
			ZI154 = ZI152;
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
	*ZO153 = ZI153;
	*ZO154 = ZI154;
}

static void
p_160(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;
			t_pos ZI115;
			t_pos ZI116;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 391 "src/libre/parser.act"

		ZIb = '-';
		ZI115 = lex_state->lx.start;
		ZI116   = lex_state->lx.end;
	
#line 606 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 621 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZIb))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 619 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
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
p_164(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg, t_char *ZI161, t_pos *ZI162)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;
			t_pos ZIend;

			/* BEGINNING OF INLINE: 93 */
			{
				{
					t_char ZI94;
					t_pos ZI95;
					t_pos ZI96;

					switch (CURRENT_TERMINAL) {
					case (TOK_RANGE):
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 391 "src/libre/parser.act"

		ZI94 = '-';
		ZI95 = lex_state->lx.start;
		ZI96   = lex_state->lx.end;
	
#line 661 "src/libre/dialect/sql/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-range */
					{
#line 998 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
	
#line 681 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: err-expected-range */
				}
			ZL2:;
			}
			/* END OF INLINE: 93 */
			/* BEGINNING OF INLINE: 97 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						t_pos ZI98;

						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 510 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI98 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIb = lex_state->buf.a[0];
	
#line 707 "src/libre/dialect/sql/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_RANGE):
					{
						t_pos ZI100;

						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 391 "src/libre/parser.act"

		ZIb = '-';
		ZI100 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 725 "src/libre/dialect/sql/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 97 */
			/* BEGINNING OF ACTION: mark-range */
			{
#line 1024 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI162));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 743 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 690 "src/libre/parser.act"

		int i;

		if ((unsigned char) (ZIb) < (unsigned char) (*ZI161)) {
			char a[5], b[5];

			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);

			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, (*ZI161)), escchar(b, sizeof b, (ZIb)));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		for (i = (unsigned char) (*ZI161); i <= (unsigned char) (ZIb); i++) {
			if (-1 == group_add((ZIg), flags, (char) i)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}
		}
	
#line 770 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 621 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (*ZI161))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 786 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZIg;

		/* BEGINNING OF ACTION: make-group */
		{
#line 587 "src/libre/parser.act"

		(ZIg).set = fsm_new_blank(fsm->opt);
		if ((ZIg).set == NULL) {
			goto ZL1;
		}

		(ZIg).dup = fsm_new_blank(fsm->opt);
		if ((ZIg).dup == NULL) {
			fsm_free((ZIg).set);
			goto ZL1;
		}
	
#line 824 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: make-group */
		p_group_C_Cgroup_Hbm (fsm, flags, lex_state, act_state, err, &ZIg);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 712 "src/libre/parser.act"

		int r;

		r = fsm_empty((&ZIg)->dup);
		if (r == -1) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		if (!r) {
			const struct fsm_state *end;

			/* TODO: would like to show the original spelling verbatim, too */

			/* XXX: this is just one example; really I want to show the entire set */
			end = fsm_any((&ZIg)->dup, fsm_isend);
			assert(end != NULL);

			if (-1 == fsm_example((&ZIg)->dup, end, err->dup, sizeof err->dup)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			/* XXX: to return when we can show minimal coverage again */
			strcpy(err->set, err->dup);

			err->e  = RE_EOVERLAP;
			goto ZL1;
		}

		if (!fsm_minimise((&ZIg)->set)) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		/* TODO: centralise as fsm_unionxy() perhaps */
		{
			struct fsm_state *a, *b;
			struct fsm_state *end;

			a = fsm_getstart(fsm);
			b = fsm_getstart((&ZIg)->set);

			end = fsm_collate((&ZIg)->set, fsm_isend);
			if (end == NULL) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			if (!fsm_merge(fsm, (&ZIg)->set)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			fsm_setstart(fsm, a);

			if (!fsm_addedge_epsilon(fsm, (ZIx), b)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			fsm_setend(fsm, end, 0);

			if (!fsm_addedge_epsilon(fsm, end, (ZIy))) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			fsm_free((&ZIg)->dup);
		}
	
#line 906 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: group-to-states */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Cgroup_Hbody(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 108 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI157;
					t_pos ZI158;
					t_pos ZI159;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 402 "src/libre/parser.act"

		ZI157 = ']';
		ZI158 = lex_state->lx.start;
		ZI159   = lex_state->lx.end;
	
#line 940 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 621 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZI157))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 953 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: group-add-char */
					p_160 (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_RANGE):
				{
					t_char ZIc;
					t_pos ZI111;
					t_pos ZI112;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 391 "src/libre/parser.act"

		ZIc = '-';
		ZI111 = lex_state->lx.start;
		ZI112   = lex_state->lx.end;
	
#line 977 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 621 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZIc))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 990 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 108 */
		p_group_C_Clist_Hof_Hterms (fsm, flags, lex_state, act_state, err, ZIg);
		/* BEGINNING OF INLINE: 117 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 117 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Clist_Hof_Halts (fsm, flags, lex_state, act_state, err, ZIx, ZIy);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_group_C_Clist_Hof_Hterms:;
	{
		/* BEGINNING OF INLINE: 104 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_char ZI161;
					t_pos ZI162;
					t_pos ZI163;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 510 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI162 = lex_state->lx.start;
		ZI163   = lex_state->lx.end;

		ZI161 = lex_state->buf.a[0];
	
#line 1064 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
					p_164 (fsm, flags, lex_state, act_state, err, ZIg, &ZI161, &ZI162);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_CLASS__alnum): case (TOK_CLASS__alpha): case (TOK_CLASS__digit): case (TOK_CLASS__lower):
			case (TOK_CLASS__space): case (TOK_CLASS__spchr): case (TOK_CLASS__upper):
				{
					t_fsm ZIk;

					/* BEGINNING OF INLINE: group::class */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_CLASS__alnum):
							{
								/* BEGINNING OF EXTRACT: CLASS_alnum */
								{
#line 417 "src/libre/parser.act"
 ZIk = class_alnum_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1089 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_alnum */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__alpha):
							{
								/* BEGINNING OF EXTRACT: CLASS_alpha */
								{
#line 418 "src/libre/parser.act"
 ZIk = class_alpha_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1101 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_alpha */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__digit):
							{
								/* BEGINNING OF EXTRACT: CLASS_digit */
								{
#line 422 "src/libre/parser.act"
 ZIk = class_digit_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1113 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_digit */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__lower):
							{
								/* BEGINNING OF EXTRACT: CLASS_lower */
								{
#line 424 "src/libre/parser.act"
 ZIk = class_lower_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1125 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_lower */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__space):
							{
								/* BEGINNING OF EXTRACT: CLASS_space */
								{
#line 427 "src/libre/parser.act"
 ZIk = class_space_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1137 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_space */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__spchr):
							{
								/* BEGINNING OF EXTRACT: CLASS_spchr */
								{
#line 428 "src/libre/parser.act"
 ZIk = class_spchr_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1149 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_spchr */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__upper):
							{
								/* BEGINNING OF EXTRACT: CLASS_upper */
								{
#line 429 "src/libre/parser.act"
 ZIk = class_upper_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1161 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLASS_upper */
								ADVANCE_LEXER;
							}
							break;
						default:
							goto ZL4;
						}
					}
					/* END OF INLINE: group::class */
					/* BEGINNING OF ACTION: group-add-class */
					{
#line 635 "src/libre/parser.act"

		struct fsm *q;
		int r;

		/* TODO: maybe it is worth using carryopaque, after the entire group is constructed */
		{
			struct fsm *a, *b;

			a = fsm_clone((ZIg)->set);
			if (a == NULL) {
				err->e = RE_EERRNO;
				goto ZL4;
			}

			b = fsm_clone((ZIk));
			if (b == NULL) {
				fsm_free(a);
				err->e = RE_EERRNO;
				goto ZL4;
			}

			q = fsm_intersect(a, b);
			if (q == NULL) {
				fsm_free(a);
				fsm_free(b);
				err->e = RE_EERRNO;
				goto ZL4;
			}

			r = fsm_empty(q);

			if (r == -1) {
				err->e = RE_EERRNO;
				goto ZL4;
			}
		}

		if (!r) {
			(ZIg)->dup = fsm_union((ZIg)->dup, q);
			if ((ZIg)->dup == NULL) {
				err->e = RE_EERRNO;
				goto ZL4;
			}
		} else {
			fsm_free(q);

			(ZIg)->set = fsm_union((ZIg)->set, (ZIk));
			if ((ZIg)->set == NULL) {
				err->e = RE_EERRNO;
				goto ZL4;
			}

			/* we need a DFA here for sake of fsm_exec() identifying duplicates */
			if (!fsm_determinise((ZIg)->set)) {
				err->e = RE_EERRNO;
				goto ZL4;
			}
		}
	
#line 1234 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: group-add-class */
				}
				break;
			default:
				goto ZL4;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-term */
				{
#line 974 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
	
#line 1253 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 104 */
		/* BEGINNING OF INLINE: 105 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLASS__alnum): case (TOK_CLASS__alpha): case (TOK_CLASS__digit): case (TOK_CLASS__lower):
			case (TOK_CLASS__space): case (TOK_CLASS__spchr): case (TOK_CLASS__upper): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: group::list-of-terms */
					goto ZL2_group_C_Clist_Hof_Hterms;
					/* END OF INLINE: group::list-of-terms */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 105 */
	}
}

static void
p_expr_C_Clist_Hof_Hatoms(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 801 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 1298 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_expr_C_Catom (fsm, flags, lex_state, act_state, err, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 138 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
			case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: expr::list-of-atoms */
					ZIx = ZIz;
					goto ZL2_expr_C_Clist_Hof_Hatoms;
					/* END OF INLINE: expr::list-of-atoms */
				}
				/* UNREACHED */
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 808 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 1324 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 138 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Cgroup_Hbm(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZIstart;
		t_pos ZI120;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 397 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI120   = lex_state->lx.end;
	
#line 1361 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF INLINE: 121 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVERT):
				{
					t_char ZI122;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 387 "src/libre/parser.act"

		ZI122 = '^';
	
#line 1382 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: INVERT */
					ADVANCE_LEXER;
					p_group_C_Cgroup_Hbody (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: invert-group */
					{
#line 605 "src/libre/parser.act"

		struct fsm *any;

		any = fsm_new_any(fsm->opt);
		if (any == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		(ZIg)->set = fsm_subtract(any, (ZIg)->set);
		if ((ZIg)->set == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		/*
		 * Note we don't invert the dup set here; duplicates are always
		 * kept in the positive.
		 */
	
#line 1414 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			case (TOK_CLOSEGROUP): case (TOK_RANGE): case (TOK_CLASS__alnum): case (TOK_CLASS__alpha):
			case (TOK_CLASS__digit): case (TOK_CLASS__lower): case (TOK_CLASS__space): case (TOK_CLASS__spchr):
			case (TOK_CLASS__upper): case (TOK_CHAR):
				{
					p_group_C_Cgroup_Hbody (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 121 */
		/* BEGINNING OF INLINE: 123 */
		{
			{
				t_char ZI124;
				t_pos ZI125;
				t_pos ZIend;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 402 "src/libre/parser.act"

		ZI124 = ']';
		ZI125 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1452 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL4;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 1019 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 1467 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 1004 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
	
#line 1482 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
			}
		ZL3:;
		}
		/* END OF INLINE: 123 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-groupbody */
		{
#line 1010 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXGROUPBODY;
		}
	
#line 1501 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-groupbody */
	}
}

static void
p_expr_C_Clist_Hof_Halts(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZI151, t_fsm__state ZI152)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI153;
		t_fsm__state ZI154;

		p_expr_C_Clist_Hof_Halts_C_Calt (fsm, flags, lex_state, act_state, err, ZI151, ZI152);
		p_155 (fsm, flags, lex_state, act_state, err, ZI151, ZI152, &ZI153, &ZI154);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Catom(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 133 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 836 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 1554 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: add-any */
				}
				break;
			case (TOK_MANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 836 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 1573 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: add-any */
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 915 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL1;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL1;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL1;
			}

			(*ZIy) = z;
		}
	
#line 1605 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			case (TOK_OPENSUB):
				{
					ADVANCE_LEXER;
					p_expr (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSESUB):
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
			case (TOK_OPENGROUP):
				{
					p_group (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_CHAR):
				{
					p_literal (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 133 */
		/* BEGINNING OF INLINE: 134 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 942 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 1680 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 915 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 1717 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 965 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 1731 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: count-1 */
				}
				break;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 980 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
	
#line 1748 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 134 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_literal(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 127 */
		{
			{
				t_pos ZI128;
				t_pos ZI129;

				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 510 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI128 = lex_state->lx.start;
		ZI129   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1791 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					break;
				default:
					goto ZL1;
				}
				ADVANCE_LEXER;
			}
		}
		/* END OF INLINE: 127 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 825 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!addedge_literal(fsm, flags, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 1815 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 1185 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->opaque != NULL);

		lex_state = lx->opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	static int
	parse(int (*f)(void *opaque), void *opaque,
		void (*entry)(struct fsm *, flags, lex_state, act_state, err),
		enum re_flags flags, int overlap,
		struct fsm *new, struct re_err *err)
	{
		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		assert(f != NULL);
		assert(entry != NULL);

		if (err == NULL) {
			err = &dummy;
		}

		lex_state    = &lex_state_s;
		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		LX_INIT(lx);

		lx->lgetc  = lgetc;
		lx->opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		/* (except for pushing "[" and "]" around ::group-$dialect) */
		lx->buf   = &lex_state->buf;
		lx->push  = CAT(LX_PREFIX, _dynpush);
		lx->pop   = CAT(LX_PREFIX, _dynpop);
		lx->clear = CAT(LX_PREFIX, _dynclear);
		lx->free  = CAT(LX_PREFIX, _dynfree);

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->overlap = overlap;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		entry(new, flags, lex_state, act_state, err);

		lx->free(lx);

		if (err->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			goto error;
		}

		return 0;

	error:

		/*
		 * Some errors describe multiple tokens; for these, the start and end
		 * positions belong to potentially different tokens, and therefore need
		 * to be stored statefully (in act_state). These are all from
		 * non-recursive productions by design, and so a stack isn't needed.
		 *
		 * Lexical errors describe a problem with a single token; for these,
		 * the start and end positions belong to that token.
		 *
		 * Syntax errors occur at the first point the order of tokens is known
		 * to be incorrect, rather than describing a span of bytes. For these,
		 * the start of the next token is most relevant.
		 */

		switch (err->e) {
		case RE_EOVERLAP:  err->start = act_state->groupstart; err->end = act_state->groupend; break;
		case RE_ENEGRANGE: err->start = act_state->rangestart; err->end = act_state->rangeend; break;
		case RE_ENEGCOUNT: err->start = act_state->countstart; err->end = act_state->countend; break;

		case RE_EHEXRANGE:
		case RE_EOCTRANGE:
		case RE_ECOUNTRANGE:
			/*
			 * Lexical errors: These are always generated for the current token,
			 * so lx->start/end here is correct because ADVANCE_LEXER has
			 * not been called.
			 */
			mark(&err->start, &lx->start);
			mark(&err->end,   &lx->end);
			break;

		default:
			/*
			 * Due to LL(1) lookahead, lx->start/end is the next token.
			 * This is approximately correct as the position of an error,
			 * but to be exactly correct, we store the pos for the previous token.
			 * This is more visible when whitespace exists.
			 */
			err->start = act_state->synstart;
			err->end   = act_state->synstart; /* single point */
			break;
		}

		return -1;
	}

	struct fsm *
	DIALECT_COMP(int (*f)(void *opaque), void *opaque,
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct fsm *new;

		assert(f != NULL);

		new = fsm_new_blank(opt);
		if (new == NULL) {
			return NULL;
		}

		if (-1 == parse(f, opaque, DIALECT_ENTRY, flags, overlap, new, err)) {
			fsm_free(new);
			return NULL;
		}

		return new;
	}

#line 1980 "src/libre/dialect/sql/parser.c"

/* END OF FILE */
