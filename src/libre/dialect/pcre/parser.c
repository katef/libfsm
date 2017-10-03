/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 23 "src/libre/parser.act"


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
	#define TOK_CLASS__any    TOK_CLASS_ANY
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

	#define TOK_CLASS__nspace  TOK_CLASS_NSPACE
	#define TOK_CLASS__ndigit  TOK_CLASS_NDIGIT

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"

	struct grp {
		struct fsm *set;
		struct fsm *dup;
	};

	struct flags {
		enum re_flags flags;
		struct flags *parent;
	};

	typedef char     t_char;
	typedef const char * t_class;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef struct fsm * t_fsm;
	typedef enum re_flags t_re__flags;
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

	/* TODO: centralise as fsm_unionxy() perhaps */
	static int
	fsm_unionxy(struct fsm *a, struct fsm *b, struct fsm_state *x, struct fsm_state *y)
	{
		struct fsm_state *sa, *sb;
		struct fsm_state *end;

		assert(a != NULL);
		assert(b != NULL);
		assert(x != NULL);
		assert(y != NULL);

		sa = fsm_getstart(a);
		sb = fsm_getstart(b);

		end = fsm_collate(b, fsm_isend);
		if (end == NULL) {
			return 0;
		}

		if (!fsm_merge(a, b)) {
			return 0;
		}

		fsm_setstart(a, sa);

		if (!fsm_addedge_epsilon(a, x, sb)) {
			return 0;
		}

		fsm_setend(a, end, 0);

		if (!fsm_addedge_epsilon(a, end, y)) {
			return 0;
		}

		return 1;
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

	static struct fsm *
	negate(struct fsm *class, const struct fsm_options *opt)
	{
		struct fsm *any;

		any = class_any_fsm(opt);

		if (any == NULL || class == NULL) {
			if (any) fsm_free(any);
			if (class) fsm_free(class);
		};

		class = fsm_subtract(any, class);

		return class;
	}

#line 427 "src/libre/dialect/pcre/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_anchor(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_256(fsm, flags, lex_state, act_state, err, t_fsm__state *, t_fsm__state *, t_pos *, t_unsigned *);
static void p_inline_Hflags_C_Cnegative(fsm, flags, lex_state, act_state, err);
static void p_expr_C_Clist_Hof_Halts_C_Calt(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbody(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_inline_Hflags(fsm, flags, lex_state, act_state, err);
static void p_expr(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr_C_Clist_Hof_Hatoms(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbm(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr_C_Clist_Hof_Halts(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_223(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_228(fsm, flags, lex_state, act_state, err, t_grp *);
extern void p_re__pcre(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_expr_C_Catom(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state *);
static void p_literal(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_252(fsm, flags, lex_state, act_state, err, t_grp *, t_char *, t_pos *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_anchor(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pred ZIp;

		/* BEGINNING OF INLINE: 180 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_END):
				{
					/* BEGINNING OF EXTRACT: END */
					{
#line 653 "src/libre/parser.act"

		switch (flags->flags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_EOT; break;
		case RE_MULTI: ZIp = RE_EOL; break;
		case RE_ZONE:  ZIp = RE_EOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 488 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: END */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_START):
				{
					/* BEGINNING OF EXTRACT: START */
					{
#line 641 "src/libre/parser.act"

		switch (flags->flags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_SOT; break;
		case RE_MULTI: ZIp = RE_SOL; break;
		case RE_ZONE:  ZIp = RE_SOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 510 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: START */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 180 */
		/* BEGINNING OF ACTION: add-pred */
		{
#line 889 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

/* TODO:
		if (!fsm_addedge_predicate(fsm, (ZIx), (ZIy), (ZIp))) {
			goto ZL1;
		}
*/
	
#line 534 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: add-pred */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_256(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state *ZIx, t_fsm__state *ZIy, t_pos *ZI253, t_unsigned *ZI255)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI201;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 478 "src/libre/parser.act"

		ZI201 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 560 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 1167 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI253));
		mark(&act_state->countend,   &(ZIend));
	
#line 571 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 956 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI255) < (*ZI255)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZI255);
			err->n = (*ZI255);
			goto ZL1;
		}

		if ((*ZI255) == 0) {
			if (!fsm_addedge_epsilon(fsm, (*ZIx), (*ZIy))) {
				goto ZL1;
			}
		}

		b = (*ZIy);

		for (i = 1; i < (*ZI255); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI255)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 619 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZIend;
			t_pos ZI204;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 665 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIn = (unsigned int) u;
	
#line 655 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				/* BEGINNING OF EXTRACT: CLOSECOUNT */
				{
#line 478 "src/libre/parser.act"

		ZIend = lex_state->lx.start;
		ZI204   = lex_state->lx.end;
	
#line 672 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 1167 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI253));
		mark(&act_state->countend,   &(ZIend));
	
#line 687 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 956 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((ZIn) < (*ZI255)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZI255);
			err->n = (ZIn);
			goto ZL1;
		}

		if ((*ZI255) == 0) {
			if (!fsm_addedge_epsilon(fsm, (*ZIx), (*ZIy))) {
				goto ZL1;
			}
		}

		b = (*ZIy);

		for (i = 1; i < (ZIn); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI255)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 735 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
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
p_inline_Hflags_C_Cnegative(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_inline_Hflags_C_Cnegative:;
	{
		/* BEGINNING OF INLINE: 189 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_FLAG__INSENSITIVE):
				{
					t_re__flags ZIc;

					/* BEGINNING OF EXTRACT: FLAG_INSENSITIVE */
					{
#line 685 "src/libre/parser.act"

		ZIc = RE_ICASE;
	
#line 772 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: FLAG_INSENSITIVE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: clear-flag */
					{
#line 1082 "src/libre/parser.act"

		flags->flags &= ~(ZIc);
	
#line 782 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: clear-flag */
				}
				break;
			case (TOK_FLAG__UNKNOWN):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unknown-flag */
					{
#line 1136 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 799 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unknown-flag */
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 189 */
		/* BEGINNING OF INLINE: 190 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_FLAG__UNKNOWN): case (TOK_FLAG__INSENSITIVE):
				{
					/* BEGINNING OF INLINE: inline-flags::negative */
					goto ZL2_inline_Hflags_C_Cnegative;
					/* END OF INLINE: inline-flags::negative */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 190 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Clist_Hof_Halts_C_Calt(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENCAPTURE): case (TOK_OPENGROUP):
	case (TOK_CLASS__digit): case (TOK_CLASS__space): case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace):
	case (TOK_OPENFLAGS): case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL):
	case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR): case (TOK_START):
	case (TOK_END):
		{
			t_fsm__state ZIz;

			/* BEGINNING OF ACTION: add-concat */
			{
#line 876 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 852 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: add-concat */
			/* BEGINNING OF ACTION: add-epsilon */
			{
#line 883 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIz))) {
			goto ZL1;
		}
	
#line 863 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: add-epsilon */
			p_expr_C_Clist_Hof_Hatoms (fsm, flags, lex_state, act_state, err, ZIz, ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: add-epsilon */
			{
#line 883 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 883 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: add-epsilon */
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
#line 699 "src/libre/parser.act"

		(ZIg).set = fsm_new_blank(fsm->opt);
		if ((ZIg).set == NULL) {
			goto ZL1;
		}

		(ZIg).dup = fsm_new_blank(fsm->opt);
		if ((ZIg).dup == NULL) {
			fsm_free((ZIg).set);
			goto ZL1;
		}
	
#line 921 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: make-group */
		p_group_C_Cgroup_Hbm (fsm, flags, lex_state, act_state, err, &ZIg);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 814 "src/libre/parser.act"

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

		if (!fsm_unionxy(fsm, (&ZIg)->set, (ZIx), (ZIy))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		fsm_free((&ZIg)->dup);
	
#line 974 "src/libre/dialect/pcre/parser.c"
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
		/* BEGINNING OF INLINE: 147 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI225;
					t_pos ZI226;
					t_pos ZI227;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 467 "src/libre/parser.act"

		ZI225 = ']';
		ZI226 = lex_state->lx.start;
		ZI227   = lex_state->lx.end;
	
#line 1008 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 725 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags->flags, (ZI225))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 1021 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: group-add-char */
					p_228 (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_RANGE):
				{
					t_char ZIc;
					t_pos ZI150;
					t_pos ZI151;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 456 "src/libre/parser.act"

		ZIc = '-';
		ZI150 = lex_state->lx.start;
		ZI151   = lex_state->lx.end;
	
#line 1045 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 725 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags->flags, (ZIc))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 1058 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 147 */
		p_group_C_Clist_Hof_Hterms (fsm, flags, lex_state, act_state, err, ZIg);
		/* BEGINNING OF INLINE: 156 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 156 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_inline_Hflags(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_OPENFLAGS):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF INLINE: 191 */
		{
		ZL3_191:;
			switch (CURRENT_TERMINAL) {
			case (TOK_FLAG__UNKNOWN): case (TOK_FLAG__INSENSITIVE):
				{
					/* BEGINNING OF INLINE: inline-flags::positive */
					{
						{
							/* BEGINNING OF INLINE: 185 */
							{
								switch (CURRENT_TERMINAL) {
								case (TOK_FLAG__INSENSITIVE):
									{
										t_re__flags ZIc;

										/* BEGINNING OF EXTRACT: FLAG_INSENSITIVE */
										{
#line 685 "src/libre/parser.act"

		ZIc = RE_ICASE;
	
#line 1120 "src/libre/dialect/pcre/parser.c"
										}
										/* END OF EXTRACT: FLAG_INSENSITIVE */
										ADVANCE_LEXER;
										/* BEGINNING OF ACTION: set-flag */
										{
#line 1078 "src/libre/parser.act"

		flags->flags |= (ZIc);
	
#line 1130 "src/libre/dialect/pcre/parser.c"
										}
										/* END OF ACTION: set-flag */
									}
									break;
								case (TOK_FLAG__UNKNOWN):
									{
										ADVANCE_LEXER;
										/* BEGINNING OF ACTION: err-unknown-flag */
										{
#line 1136 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 1147 "src/libre/dialect/pcre/parser.c"
										}
										/* END OF ACTION: err-unknown-flag */
									}
									break;
								default:
									goto ZL1;
								}
							}
							/* END OF INLINE: 185 */
							/* BEGINNING OF INLINE: 191 */
							goto ZL3_191;
							/* END OF INLINE: 191 */
						}
						/* UNREACHED */
					}
					/* END OF INLINE: inline-flags::positive */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 191 */
		/* BEGINNING OF INLINE: 192 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NEGATE):
				{
					ADVANCE_LEXER;
					p_inline_Hflags_C_Cnegative (fsm, flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 192 */
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSEFLAGS):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-closeflags */
		{
#line 1143 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEFLAGS;
		}
		goto ZL7;
	
#line 1209 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-closeflags */
	}
	goto ZL0;
ZL7:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
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
		/* BEGINNING OF INLINE: 143 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_char ZI249;
					t_pos ZI250;
					t_pos ZI251;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 631 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI250 = lex_state->lx.start;
		ZI251   = lex_state->lx.end;

		ZI249 = lex_state->buf.a[0];
	
#line 1268 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
					p_252 (fsm, flags, lex_state, act_state, err, ZIg, &ZI249, &ZI250);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_CONTROL):
				{
					t_char ZI233;
					t_pos ZI234;
					t_pos ZI235;

					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 535 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI233 = lex_state->buf.a[2];
		if ((unsigned char) ZI233 > 127) {
			goto ZL4;
		}
		ZI233 = (((toupper((unsigned char)ZI233)) - 64) % 128 + 128) % 128;

		ZI234 = lex_state->lx.start;
		ZI235   = lex_state->lx.end;
	
#line 1303 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					p_252 (fsm, flags, lex_state, act_state, err, ZIg, &ZI233, &ZI234);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_ESC):
				{
					t_char ZI229;
					t_pos ZI230;
					t_pos ZI231;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 503 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI229 = lex_state->buf.a[1];

		switch (ZI229) {
		case 'a': ZI229 = '\a'; break;
		case 'f': ZI229 = '\f'; break;
		case 'n': ZI229 = '\n'; break;
		case 'r': ZI229 = '\r'; break;
		case 't': ZI229 = '\t'; break;
		case 'v': ZI229 = '\v'; break;
		default:             break;
		}

		ZI230 = lex_state->lx.start;
		ZI231   = lex_state->lx.end;
	
#line 1343 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
					p_252 (fsm, flags, lex_state, act_state, err, ZIg, &ZI229, &ZI230);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_HEX):
				{
					t_char ZI245;
					t_pos ZI246;
					t_pos ZI247;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 591 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI246 = lex_state->lx.start;
		ZI247   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL4;
		}

		ZI245 = (char) (unsigned char) u;
	
#line 1402 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
					p_252 (fsm, flags, lex_state, act_state, err, ZIg, &ZI245, &ZI246);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_NOESC):
				{
					t_char ZI237;
					t_pos ZI238;
					t_pos ZI239;

					/* BEGINNING OF EXTRACT: NOESC */
					{
#line 524 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI237 = lex_state->buf.a[1];

		ZI238 = lex_state->lx.start;
		ZI239   = lex_state->lx.end;
	
#line 1432 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: NOESC */
					ADVANCE_LEXER;
					p_252 (fsm, flags, lex_state, act_state, err, ZIg, &ZI237, &ZI238);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_OCT):
				{
					t_char ZI241;
					t_pos ZI242;
					t_pos ZI243;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 551 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI242 = lex_state->lx.start;
		ZI243   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL4;
		}

		ZI241 = (char) (unsigned char) u;
	
#line 1491 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
					p_252 (fsm, flags, lex_state, act_state, err, ZIg, &ZI241, &ZI242);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_CLASS__alnum): case (TOK_CLASS__alpha): case (TOK_CLASS__ascii): case (TOK_CLASS__blank):
			case (TOK_CLASS__cntrl): case (TOK_CLASS__digit): case (TOK_CLASS__graph): case (TOK_CLASS__lower):
			case (TOK_CLASS__print): case (TOK_CLASS__punct): case (TOK_CLASS__space): case (TOK_CLASS__upper):
			case (TOK_CLASS__word): case (TOK_CLASS__xdigit): case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace):
				{
					t_fsm ZIf;

					/* BEGINNING OF INLINE: group::class */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_CLASS__alnum):
							{
								/* BEGINNING OF EXTRACT: CLASS_alnum */
								{
#line 483 "src/libre/parser.act"
 ZIf = class_alnum_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1518 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_alnum */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__alpha):
							{
								/* BEGINNING OF EXTRACT: CLASS_alpha */
								{
#line 484 "src/libre/parser.act"
 ZIf = class_alpha_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1530 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_alpha */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__ascii):
							{
								/* BEGINNING OF EXTRACT: CLASS_ascii */
								{
#line 486 "src/libre/parser.act"
 ZIf = class_ascii_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1542 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_ascii */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__blank):
							{
								/* BEGINNING OF EXTRACT: CLASS_blank */
								{
#line 487 "src/libre/parser.act"
 ZIf = class_blank_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1554 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_blank */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__cntrl):
							{
								/* BEGINNING OF EXTRACT: CLASS_cntrl */
								{
#line 488 "src/libre/parser.act"
 ZIf = class_cntrl_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1566 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_cntrl */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__digit):
							{
								/* BEGINNING OF EXTRACT: CLASS_digit */
								{
#line 489 "src/libre/parser.act"
 ZIf = class_digit_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1578 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_digit */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__graph):
							{
								/* BEGINNING OF EXTRACT: CLASS_graph */
								{
#line 490 "src/libre/parser.act"
 ZIf = class_graph_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1590 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_graph */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__lower):
							{
								/* BEGINNING OF EXTRACT: CLASS_lower */
								{
#line 491 "src/libre/parser.act"
 ZIf = class_lower_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1602 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_lower */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__ndigit):
							{
								/* BEGINNING OF EXTRACT: CLASS_ndigit */
								{
#line 500 "src/libre/parser.act"
 ZIf = class_digit_fsm(fsm->opt); ZIf = negate(ZIf, fsm->opt); if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1614 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_ndigit */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__nspace):
							{
								/* BEGINNING OF EXTRACT: CLASS_nspace */
								{
#line 501 "src/libre/parser.act"
 ZIf = class_space_fsm(fsm->opt); ZIf = negate(ZIf, fsm->opt); if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1626 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_nspace */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__print):
							{
								/* BEGINNING OF EXTRACT: CLASS_print */
								{
#line 492 "src/libre/parser.act"
 ZIf = class_print_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1638 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_print */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__punct):
							{
								/* BEGINNING OF EXTRACT: CLASS_punct */
								{
#line 493 "src/libre/parser.act"
 ZIf = class_punct_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1650 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_punct */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__space):
							{
								/* BEGINNING OF EXTRACT: CLASS_space */
								{
#line 494 "src/libre/parser.act"
 ZIf = class_space_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1662 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_space */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__upper):
							{
								/* BEGINNING OF EXTRACT: CLASS_upper */
								{
#line 496 "src/libre/parser.act"
 ZIf = class_upper_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1674 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_upper */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__word):
							{
								/* BEGINNING OF EXTRACT: CLASS_word */
								{
#line 497 "src/libre/parser.act"
 ZIf = class_word_fsm(fsm->opt);   if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1686 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_word */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__xdigit):
							{
								/* BEGINNING OF EXTRACT: CLASS_xdigit */
								{
#line 498 "src/libre/parser.act"
 ZIf = class_xdigit_fsm(fsm->opt); if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1698 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_xdigit */
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
#line 732 "src/libre/parser.act"

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

			b = fsm_clone((ZIf));
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

			(ZIg)->set = fsm_union((ZIg)->set, (ZIf));
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
	
#line 1771 "src/libre/dialect/pcre/parser.c"
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
#line 1087 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL1;
	
#line 1791 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 143 */
		/* BEGINNING OF INLINE: 144 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLASS__alnum): case (TOK_CLASS__alpha): case (TOK_CLASS__ascii): case (TOK_CLASS__blank):
			case (TOK_CLASS__cntrl): case (TOK_CLASS__digit): case (TOK_CLASS__graph): case (TOK_CLASS__lower):
			case (TOK_CLASS__print): case (TOK_CLASS__punct): case (TOK_CLASS__space): case (TOK_CLASS__upper):
			case (TOK_CLASS__word): case (TOK_CLASS__xdigit): case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace):
			case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
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
		/* END OF INLINE: 144 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
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
#line 876 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 1844 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF INLINE: 208 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_START): case (TOK_END):
				{
					p_anchor (fsm, flags, lex_state, act_state, err, ZIx, ZIz);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENCAPTURE): case (TOK_OPENGROUP):
			case (TOK_CLASS__digit): case (TOK_CLASS__space): case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace):
			case (TOK_OPENFLAGS): case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					p_expr_C_Catom (fsm, flags, lex_state, act_state, err, ZIx, &ZIz);
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
		/* END OF INLINE: 208 */
		/* BEGINNING OF INLINE: 209 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENCAPTURE): case (TOK_OPENGROUP):
			case (TOK_CLASS__digit): case (TOK_CLASS__space): case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace):
			case (TOK_OPENFLAGS): case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR): case (TOK_START):
			case (TOK_END):
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
#line 883 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 1901 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
		}
		/* END OF INLINE: 209 */
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
		t_pos ZI159;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 462 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI159   = lex_state->lx.end;
	
#line 1935 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF INLINE: 160 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVERT):
				{
					t_char ZI161;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 452 "src/libre/parser.act"

		ZI161 = '^';
	
#line 1956 "src/libre/dialect/pcre/parser.c"
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
#line 712 "src/libre/parser.act"

		(ZIg)->set = negate((ZIg)->set, fsm->opt);
		if ((ZIg)->set == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		/*
		 * Note we don't invert the dup set here; duplicates are always
		 * kept in the positive.
		 */
	
#line 1980 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			case (TOK_CLOSEGROUP): case (TOK_RANGE): case (TOK_CLASS__alnum): case (TOK_CLASS__alpha):
			case (TOK_CLASS__ascii): case (TOK_CLASS__blank): case (TOK_CLASS__cntrl): case (TOK_CLASS__digit):
			case (TOK_CLASS__graph): case (TOK_CLASS__lower): case (TOK_CLASS__print): case (TOK_CLASS__punct):
			case (TOK_CLASS__space): case (TOK_CLASS__upper): case (TOK_CLASS__word): case (TOK_CLASS__xdigit):
			case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace): case (TOK_ESC): case (TOK_NOESC):
			case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
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
		/* END OF INLINE: 160 */
		/* BEGINNING OF INLINE: 162 */
		{
			{
				t_char ZI163;
				t_pos ZI164;
				t_pos ZIend;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 467 "src/libre/parser.act"

		ZI163 = ']';
		ZI164 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2021 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL4;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 1157 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 2036 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 1122 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 2052 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
			}
		ZL3:;
		}
		/* END OF INLINE: 162 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-groupbody */
		{
#line 1129 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXGROUPBODY;
		}
		goto ZL5;
	
#line 2072 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-groupbody */
	}
	goto ZL0;
ZL5:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
}

static void
p_expr_C_Clist_Hof_Halts(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZI219, t_fsm__state ZI220)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI221;
		t_fsm__state ZI222;

		p_expr_C_Clist_Hof_Halts_C_Calt (fsm, flags, lex_state, act_state, err, ZI219, ZI220);
		p_223 (fsm, flags, lex_state, act_state, err, ZI219, ZI220, &ZI221, &ZI222);
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
p_223(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZI219, t_fsm__state ZI220, t_fsm__state *ZO221, t_fsm__state *ZO222)
{
	t_fsm__state ZI221;
	t_fsm__state ZI222;

ZL2_223:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts_C_Calt (fsm, flags, lex_state, act_state, err, ZI219, ZI220);
			/* BEGINNING OF INLINE: 223 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_223;
			}
			/* END OF INLINE: 223 */
		}
		/* UNREACHED */
	default:
		{
			ZI221 = ZI219;
			ZI222 = ZI220;
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
	*ZO221 = ZI221;
	*ZO222 = ZI222;
}

static void
p_228(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;
			t_pos ZI154;
			t_pos ZI155;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 456 "src/libre/parser.act"

		ZIb = '-';
		ZI154 = lex_state->lx.start;
		ZI155   = lex_state->lx.end;
	
#line 2164 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 725 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags->flags, (ZIb))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 2177 "src/libre/dialect/pcre/parser.c"
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

void
p_re__pcre(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 215 */
		{
			{
				p_expr (fsm, flags, lex_state, act_state, err, ZIx, ZIy);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL3;
				}
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-alts */
				{
#line 1108 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL1;
	
#line 2221 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 215 */
		/* BEGINNING OF INLINE: 216 */
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
#line 1150 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 2251 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 216 */
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
		/* BEGINNING OF INLINE: 196 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 911 "src/libre/parser.act"

		struct fsm *any;

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		any = class_any_fsm(fsm->opt);
		if (any == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		if (!fsm_unionxy(fsm, any, (ZIx), (*ZIy))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 2298 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: add-any */
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
			case (TOK_OPENFLAGS):
				{
					p_inline_Hflags (fsm, flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 883 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 2327 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					p_literal (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_CLASS__digit): case (TOK_CLASS__space): case (TOK_CLASS__ndigit): case (TOK_CLASS__nspace):
				{
					t_fsm ZIf;

					/* BEGINNING OF INLINE: pcre-class */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_CLASS__digit):
							{
								/* BEGINNING OF EXTRACT: CLASS_digit */
								{
#line 489 "src/libre/parser.act"
 ZIf = class_digit_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL1; } 
#line 2355 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_digit */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__ndigit):
							{
								/* BEGINNING OF EXTRACT: CLASS_ndigit */
								{
#line 500 "src/libre/parser.act"
 ZIf = class_digit_fsm(fsm->opt); ZIf = negate(ZIf, fsm->opt); if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL1; } 
#line 2367 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_ndigit */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__nspace):
							{
								/* BEGINNING OF EXTRACT: CLASS_nspace */
								{
#line 501 "src/libre/parser.act"
 ZIf = class_space_fsm(fsm->opt); ZIf = negate(ZIf, fsm->opt); if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL1; } 
#line 2379 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_nspace */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__space):
							{
								/* BEGINNING OF EXTRACT: CLASS_space */
								{
#line 494 "src/libre/parser.act"
 ZIf = class_space_fsm(fsm->opt);  if (ZIf == NULL) { err->e = RE_EERRNO; goto ZL1; } 
#line 2391 "src/libre/dialect/pcre/parser.c"
								}
								/* END OF EXTRACT: CLASS_space */
								ADVANCE_LEXER;
							}
							break;
						default:
							goto ZL1;
						}
					}
					/* END OF INLINE: pcre-class */
					/* BEGINNING OF ACTION: add-class */
					{
#line 929 "src/libre/parser.act"

		if (!fsm_unionxy(fsm, (ZIf), (ZIx), (*ZIy))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 2411 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: add-class */
				}
				break;
			case (TOK_OPENSUB): case (TOK_OPENCAPTURE):
				{
					/* BEGINNING OF INLINE: 197 */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_OPENCAPTURE):
							{
								ADVANCE_LEXER;
							}
							break;
						case (TOK_OPENSUB):
							{
								ADVANCE_LEXER;
							}
							break;
						default:
							goto ZL1;
						}
					}
					/* END OF INLINE: 197 */
					/* BEGINNING OF ACTION: push-flags */
					{
#line 1061 "src/libre/parser.act"

		struct flags *n = malloc(sizeof *n);
		if (n == NULL) {
			goto ZL1;
		}
		n->parent = flags;
		n->flags = flags->flags;
		flags = n;
	
#line 2448 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: push-flags */
					p_expr (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: pop-flags */
					{
#line 1071 "src/libre/parser.act"

		struct flags *t = flags->parent;
		assert(t != NULL);
		free(flags);
		flags = t;
	
#line 2465 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: pop-flags */
					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSE):
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 196 */
		/* BEGINNING OF INLINE: 198 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_pos ZI253;
					t_pos ZI254;
					t_unsigned ZI255;

					/* BEGINNING OF EXTRACT: OPENCOUNT */
					{
#line 473 "src/libre/parser.act"

		ZI253 = lex_state->lx.start;
		ZI254   = lex_state->lx.end;
	
#line 2498 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENCOUNT */
					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 665 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL6;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL6;
		}

		ZI255 = (unsigned int) u;
	
#line 2526 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL6;
					}
					ADVANCE_LEXER;
					p_256 (fsm, flags, lex_state, act_state, err, &ZIx, ZIy, &ZI253, &ZI255);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL6;
					}
				}
				break;
			case (TOK_OPT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 999 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL6;
		}
	
#line 2552 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 1032 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL6;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL6;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL6;
			}

			(*ZIy) = z;
		}
	
#line 2585 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 1005 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL6;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL6;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL6;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL6;
			}

			(*ZIy) = z;
		}
	
#line 2622 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 1055 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 2636 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: count-1 */
				}
				break;
			}
			goto ZL5;
		ZL6:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 1094 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL1;
	
#line 2654 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL5:;
		}
		/* END OF INLINE: 198 */
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

		/* BEGINNING OF INLINE: 166 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI177;
					t_pos ZI178;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 631 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI177 = lex_state->lx.start;
		ZI178   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 2697 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					t_pos ZI169;
					t_pos ZI170;

					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 535 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZIc = lex_state->buf.a[2];
		if ((unsigned char) ZIc > 127) {
			goto ZL1;
		}
		ZIc = (((toupper((unsigned char)ZIc)) - 64) % 128 + 128) % 128;

		ZI169 = lex_state->lx.start;
		ZI170   = lex_state->lx.end;
	
#line 2726 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					t_pos ZI167;
					t_pos ZI168;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 503 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZI167 = lex_state->lx.start;
		ZI168   = lex_state->lx.end;
	
#line 2760 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI175;
					t_pos ZI176;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 591 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI175 = lex_state->lx.start;
		ZI176   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 2813 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_NOESC):
				{
					t_pos ZI171;
					t_pos ZI172;

					/* BEGINNING OF EXTRACT: NOESC */
					{
#line 524 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		ZI171 = lex_state->lx.start;
		ZI172   = lex_state->lx.end;
	
#line 2837 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: NOESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI173;
					t_pos ZI174;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 551 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI173 = lex_state->lx.start;
		ZI174   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 2890 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 166 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 900 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!addedge_literal(fsm, flags->flags, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 2914 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_252(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg, t_char *ZI249, t_pos *ZI250)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;
			t_pos ZIend;

			/* BEGINNING OF INLINE: 128 */
			{
				{
					t_char ZI129;
					t_pos ZI130;
					t_pos ZI131;

					switch (CURRENT_TERMINAL) {
					case (TOK_RANGE):
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 456 "src/libre/parser.act"

		ZI129 = '-';
		ZI130 = lex_state->lx.start;
		ZI131   = lex_state->lx.end;
	
#line 2950 "src/libre/dialect/pcre/parser.c"
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
#line 1115 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL1;
	
#line 2971 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-expected-range */
				}
			ZL2:;
			}
			/* END OF INLINE: 128 */
			/* BEGINNING OF INLINE: 132 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						t_pos ZI138;

						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 631 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI138 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIb = lex_state->buf.a[0];
	
#line 2997 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_CONTROL):
					{
						t_pos ZI135;

						/* BEGINNING OF EXTRACT: CONTROL */
						{
#line 535 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZIb = lex_state->buf.a[2];
		if ((unsigned char) ZIb > 127) {
			goto ZL1;
		}
		ZIb = (((toupper((unsigned char)ZIb)) - 64) % 128 + 128) % 128;

		ZI135 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 3025 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: CONTROL */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_ESC):
					{
						t_pos ZI133;

						/* BEGINNING OF EXTRACT: ESC */
						{
#line 503 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIb = lex_state->buf.a[1];

		switch (ZIb) {
		case 'a': ZIb = '\a'; break;
		case 'f': ZIb = '\f'; break;
		case 'n': ZIb = '\n'; break;
		case 'r': ZIb = '\r'; break;
		case 't': ZIb = '\t'; break;
		case 'v': ZIb = '\v'; break;
		default:             break;
		}

		ZI133 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 3058 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: ESC */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						t_pos ZI137;

						/* BEGINNING OF EXTRACT: HEX */
						{
#line 591 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI137 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIb = (char) (unsigned char) u;
	
#line 3110 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						t_pos ZI136;

						/* BEGINNING OF EXTRACT: OCT */
						{
#line 551 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI136 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIb = (char) (unsigned char) u;
	
#line 3162 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_RANGE):
					{
						t_pos ZI139;

						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 456 "src/libre/parser.act"

		ZIb = '-';
		ZI139 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 3180 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 132 */
			/* BEGINNING OF ACTION: mark-range */
			{
#line 1162 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI250));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 3198 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 792 "src/libre/parser.act"

		int i;

		if ((unsigned char) (ZIb) < (unsigned char) (*ZI249)) {
			char a[5], b[5];

			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);

			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, (*ZI249)), escchar(b, sizeof b, (ZIb)));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		for (i = (unsigned char) (*ZI249); i <= (unsigned char) (ZIb); i++) {
			if (-1 == group_add((ZIg), flags->flags, (char) i)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}
		}
	
#line 3225 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 725 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags->flags, (*ZI249))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 3241 "src/libre/dialect/pcre/parser.c"
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

/* BEGINNING OF TRAILER */

#line 1172 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->getc_opaque != NULL);

		lex_state = lx->getc_opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	static int
	parse(int (*f)(void *opaque), void *opaque,
		void (*entry)(struct fsm *, flags, lex_state, act_state, err,
			struct fsm_state *, struct fsm_state *),
		struct flags *flags, int overlap,
		struct fsm *new, struct re_err *err,
		struct fsm_state *x, struct fsm_state *y)
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

		lx->lgetc       = lgetc;
		lx->getc_opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		/* (except for pushing "[" and "]" around ::group-$dialect) */
		lx->buf_opaque = &lex_state->buf;
		lx->push       = CAT(LX_PREFIX, _dynpush);
		lx->clear      = CAT(LX_PREFIX, _dynclear);
		lx->free       = CAT(LX_PREFIX, _dynfree);

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->overlap = overlap;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		entry(new, flags, lex_state, act_state, err, x, y);

		lx->free(lx->buf_opaque);

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
		struct fsm_state *x, *y;
		struct flags top, *fl = &top;

		top.flags = flags;

		assert(f != NULL);

		new = fsm_new_blank(opt);
		if (new == NULL) {
			return NULL;
		}

		x = fsm_getstart(new);
		assert(x != NULL);

		y = fsm_addstate(new);
		if (y == NULL) {
			goto error;
		}

		fsm_setend(new, y, 1);

		if (-1 == parse(f, opaque, DIALECT_ENTRY, fl, overlap, new, err, x, y)) {
			goto error;
		}

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

#line 3430 "src/libre/dialect/pcre/parser.c"

/* END OF FILE */
