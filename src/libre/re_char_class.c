/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "re_char_class.h"
#include "re_ast.h"
#include "class.h"

#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

struct re_char_class {
	enum re_flags flags;

	enum re_char_class_flags cc_flags;

	const struct fsm_options *opt;
	struct re_err *err;

	/* These are set to NULL once they've been incorporated into the
	 * overall regex FSM, so they no longer need to be freed by the
	 * char class functions. */
	struct fsm *set;
	struct fsm *dup;
};

static int
link_char_class_into_fsm(struct re_char_class *cc, struct fsm *fsm,
    struct fsm_state *x, struct fsm_state *y);

static int
cc_add_char(struct re_char_class *cc, unsigned char byte);

static int
cc_add_range(struct re_char_class *cc, 
    const struct ast_range_endpoint *from,
    const struct ast_range_endpoint *to);

static int
cc_add_named_class(struct re_char_class *cc, char_class_constructor_fun *ctor);

static int
cc_invert(struct re_char_class *cc);

struct re_char_class_ast *
re_char_class_ast_concat(struct re_char_class_ast *l,
    struct re_char_class_ast *r)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_CONCAT;
	res->u.concat.l = l;
	res->u.concat.r = r;
	return res;
}

struct re_char_class_ast *
re_char_class_ast_literal(unsigned char c)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_LITERAL;
	res->u.literal.c = c;
	return res;
}

struct re_char_class_ast *
re_char_class_ast_range(const struct ast_range_endpoint *from, struct ast_pos start,
    const struct ast_range_endpoint *to, struct ast_pos end)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	assert(from != NULL);
	assert(to != NULL);

	res->t = RE_CHAR_CLASS_AST_RANGE;
	res->u.range.from = *from;
	res->u.range.start = start;
	res->u.range.to = *to;
	res->u.range.end = end;
	return res;
}

struct re_char_class_ast *
re_char_class_ast_flags(enum re_char_class_flags flags)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_FLAGS;
	res->u.flags.f = flags;
	return res;
}

struct re_char_class_ast *
re_char_class_ast_named_class(char_class_constructor_fun *ctor)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_NAMED;
	res->u.named.ctor = ctor;
	return res;
}

struct re_char_class_ast *
re_char_class_ast_subtract(struct re_char_class_ast *ast,
    struct re_char_class_ast *mask)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_SUBTRACT;
	res->u.subtract.ast = ast;
	res->u.subtract.mask = mask;
	return res;
}

const char *
re_char_class_id_str(enum ast_char_class_id id)
{
	switch (id) {
	case AST_CHAR_CLASS_ALNUM: return "ALNUM";
	case AST_CHAR_CLASS_ALPHA: return "ALPHA";
	case AST_CHAR_CLASS_ANY: return "ANY";
	case AST_CHAR_CLASS_ASCII: return "ASCII";
	case AST_CHAR_CLASS_BLANK: return "BLANK";
	case AST_CHAR_CLASS_CNTRL: return "CNTRL";
	case AST_CHAR_CLASS_DIGIT: return "DIGIT";
	case AST_CHAR_CLASS_GRAPH: return "GRAPH";
	case AST_CHAR_CLASS_LOWER: return "LOWER";
	case AST_CHAR_CLASS_PRINT: return "PRINT";
	case AST_CHAR_CLASS_PUNCT: return "PUNCT";
	case AST_CHAR_CLASS_SPACE: return "SPACE";
	case AST_CHAR_CLASS_SPCHR: return "SPCHR";
	case AST_CHAR_CLASS_UPPER: return "UPPER";
	case AST_CHAR_CLASS_WORD: return "WORD";
	case AST_CHAR_CLASS_XDIGIT: return "XDIGIT";
	default:
		return "<matchfail>";
	}
}

static void
free_iter(struct re_char_class_ast *n)
{
	assert(n != NULL);
	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
		free_iter(n->u.concat.l);
		free_iter(n->u.concat.r);
		break;
	case RE_CHAR_CLASS_AST_SUBTRACT:
		free_iter(n->u.subtract.ast);
		free_iter(n->u.subtract.mask);
		break;
	case RE_CHAR_CLASS_AST_LITERAL:
	case RE_CHAR_CLASS_AST_RANGE:
	case RE_CHAR_CLASS_AST_NAMED:
	case RE_CHAR_CLASS_AST_FLAGS:
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
	free(n);	
}

void
re_char_class_ast_free(struct re_char_class_ast *ast)
{
	free_iter(ast);
}

void
re_char_class_free(struct re_char_class *cc)
{
	free(cc);
}

static int
comp_iter(struct re_char_class *cc, struct re_char_class_ast *n)
{
	assert(cc != NULL);
	assert(n != NULL);
	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
		if (!comp_iter(cc, n->u.concat.l)) { return 0; }
		if (!comp_iter(cc, n->u.concat.r)) { return 0; }
		break;
	case RE_CHAR_CLASS_AST_LITERAL:
		if (!cc_add_char(cc, n->u.literal.c)) { return 0; }
		break;
	case RE_CHAR_CLASS_AST_RANGE:
		if (!cc_add_range(cc, &n->u.range.from, &n->u.range.to)) { return 0; }
		break;
	case RE_CHAR_CLASS_AST_NAMED:
		if (!cc_add_named_class(cc, n->u.named.ctor)) { return 0; }
		break;
	case RE_CHAR_CLASS_AST_FLAGS:
		cc->cc_flags |= n->u.flags.f;
		break;
	case RE_CHAR_CLASS_AST_SUBTRACT:
		fprintf(stderr, "TODO %s:%d\n", __FILE__, __LINE__);
		assert(0);
		break;
	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}

	return 1;
}

static struct fsm *
new_blank(const struct fsm_options *opt)
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

int
re_char_class_ast_compile(struct re_char_class_ast *cca,
    struct fsm *fsm, enum re_flags flags,
    struct re_err *err, const struct fsm_options *opt,
    struct fsm_state *x, struct fsm_state *y)
{
	struct re_char_class cc;
	memset(&cc, 0x00, sizeof(cc));
	
	cc.set = new_blank(opt);
	if (cc.set == NULL) { goto cleanup; }

	cc.dup = new_blank(opt);
	if (cc.dup == NULL) { goto cleanup; }

	cc.opt = opt;
	cc.err = err;
	cc.flags = flags;

	if (!comp_iter(&cc, cca)) { goto cleanup; }

	if (cc.cc_flags & RE_CHAR_CLASS_FLAG_INVERTED) {
		if (!cc_invert(&cc)) { goto cleanup; }
	}

	if (!link_char_class_into_fsm(&cc, fsm, x, y)) { goto cleanup; }

	return 1;

cleanup:
	if (cc.set != NULL) { fsm_free(cc.set); }
	if (cc.dup != NULL) { fsm_free(cc.dup); }
	return 0;
}



#include "../libfsm/internal.h" /* XXX */

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

static int
link_char_class_into_fsm(struct re_char_class *cc, struct fsm *fsm,
    struct fsm_state *x, struct fsm_state *y)
{
	int is_empty;
	struct re_err *err = cc->err;
	is_empty = fsm_empty(cc->dup);
	if (is_empty == -1) {
		err->e = RE_EERRNO;
		return 0;
	}
	
	if (!is_empty) {
		const struct fsm_state *end;
		/* TODO: would like to show the original spelling verbatim, too */
		
		/* XXX: this is just one example; really I want to show the entire set */
		end = fsm_any(cc->dup, fsm_isend);
		assert(end != NULL);
		
		if (-1 == fsm_example(cc->dup, end, err->dup, sizeof err->dup)) {
			err->e = RE_EERRNO;
			return 0;
		}
		
		/* XXX: to return when we can show minimal coverage again */
		strcpy(err->set, err->dup);
		
		err->e  = RE_EOVERLAP;
		return 0;
	}
	
	if (!fsm_minimise(cc->set)) {
		err->e = RE_EERRNO;
		return 0;
	}
	
	if (!fsm_unionxy(fsm, cc->set, x, y)) {
		err->e = RE_EERRNO;
		return 0;
	}
	cc->set = NULL;

	fsm_free(cc->dup);
	cc->dup = NULL;
		
	return 1;
}

/* FIXME: duplication */
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

int
cc_add_char(struct re_char_class *cc, unsigned char c)
{
	const struct fsm_state *p;
	struct fsm_state *start, *end;
	struct fsm *fsm;
	char a[2];
	char *s = a;
	
	assert(cc != NULL);
	
	a[0] = c;
	a[1] = '\0';
	
	errno = 0;
	p = fsm_exec(cc->set, fsm_sgetc, &s);
	if (p == NULL && errno != 0) {
		goto fail;
	}
	
	if (p == NULL) {
		fsm = cc->set;
	} else {
		fsm = cc->dup;
	}
	
	start = fsm_getstart(fsm);
	assert(start != NULL);
	
	end = fsm_addstate(fsm);
	if (end == NULL) {
		goto fail;
	}
	
	fsm_setend(fsm, end, 1);
	
	if (!addedge_literal(fsm, cc->flags, start, end, c)) {
		goto fail;
	}
	
	return 1;

fail:
	return 0;
}

static int
cc_add_range(struct re_char_class *cc, 
    const struct ast_range_endpoint *from,
    const struct ast_range_endpoint *to)
{
	unsigned char lower, upper;
	unsigned int i;

	if (from->t != AST_RANGE_ENDPOINT_LITERAL ||
	    to->t != AST_RANGE_ENDPOINT_LITERAL) {
		/* not yet supported */
		return 0;
	}
	lower = from->u.literal.c;
	upper = to->u.literal.c;

	assert(cc != NULL);
	assert(lower <= upper);
	for (i = lower; i <= upper; i++) {
		if (!cc_add_char(cc, i)) {
			return 0;
		}
	}
	return 1;
}

static int
cc_add_named_class(struct re_char_class *cc, char_class_constructor_fun *ctor)
{
	struct fsm *q;
	int r;
	struct re_err *err = cc->err;
	struct fsm *constructed = ctor(cc->opt);

	if (constructed == NULL) {
		err->e = RE_EERRNO;
		return 0;
	}

	/* TODO: maybe it is worth using carryopaque, after the entire group is constructed */
	{
		struct fsm *a, *b;
		
		a = fsm_clone(cc->set);
		if (a == NULL) {
			err->e = RE_EERRNO;
			return 0;
		}
		
		b = fsm_clone(constructed);
		if (b == NULL) {
			fsm_free(a);
			err->e = RE_EERRNO;
			return 0;
		}
		
		q = fsm_intersect(a, b);
		if (q == NULL) {
			fsm_free(a);
			fsm_free(b);
			err->e = RE_EERRNO;
			return 0;
		}
		
		r = fsm_empty(q);
		
		if (r == -1) {
			err->e = RE_EERRNO;
			return 0;
		}
	}
	
	if (!r) {
		cc->dup = fsm_union(cc->dup, q);
		if (cc->dup == NULL) {
			err->e = RE_EERRNO;
			return 0;
		}
	} else {
		fsm_free(q);
		
		cc->set = fsm_union(cc->set, constructed);
		if (cc->set == NULL) {
			err->e = RE_EERRNO;
			return 0;
		}
		
		/* we need a DFA here for sake of fsm_exec() identifying duplicates */
		if (!fsm_determinise(cc->set)) {
			err->e = RE_EERRNO;
			return 0;
		}
	}
	
	return 1;
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

int
cc_invert(struct re_char_class *cc)
{
	struct fsm *inverted = negate(cc->set, cc->opt);
	if (inverted == NULL) { return 0; }

	cc->set = inverted;

	/*
	 * Note we don't invert the dup set here; duplicates are always
	 * kept in the positive.
	 */
	return 1;
}
