/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <fsm/bool.h>
#include <fsm/pred.h>

#include <re/re.h>

#include "class.h"
#include "ast.h"

#include "libfsm/internal.h" /* XXX */

/*
 * Sort of an IR; this is our intermediate state when converting from a
 * character class's AST to link into an existing FSM. First we construct
 * stand-alone FSM for just the class, and these are held here.
 *
 * This struct is private to this file only, not even exposed as a
 * partial declaration. I'm reading "cc" as "compiled class", rather
 * than "character class".
 */
struct cc {
	enum re_flags re_flags;

	enum ast_class_flags flags;

	const struct fsm_options *opt;
	struct re_err *err;

	/*
	 * These are set to NULL once they've been incorporated into the
	 * overall regex FSM, so they no longer need to be freed by the
	 * char class functions.
	 */
	struct fsm *set;
	struct fsm *dup;
};

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

/* XXX: to go when dups show all spellings for group overlap */
static const struct fsm_state *
fsm_any(const struct fsm *fsm,
    int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	const struct fsm_state *s;
	
	assert(fsm != NULL);
	assert(predicate != NULL);
	
	for (s = fsm->sl; s != NULL; s = s->next) {
		if (predicate(fsm, s)) {
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
	struct fsm *q;
	
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
	
	/* TODO: centralise as fsm_clearends() or somesuch */
	{
		struct fsm_state *s;

		for (s = b->sl; s != NULL; s = s->next) {
			fsm_setend(b, s, 0);
		}
	}
	
	q = fsm_merge(a, b);
	assert(q != NULL);
	
	fsm_setstart(q, sa);
	
	if (!fsm_addedge_epsilon(q, x, sb)) {
		return 0;
	}
	
	if (!fsm_addedge_epsilon(q, end, y)) {
		return 0;
	}
	
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

static int
link_class_into_fsm(struct cc *cc, struct fsm *fsm,
    struct fsm_state *x, struct fsm_state *y)
{
	int is_empty;
	struct re_err *err = cc->err;

	assert(cc != NULL);

	is_empty = fsm_empty(cc->dup);
	if (is_empty == -1) {
		if (err != NULL) { err->e = RE_EERRNO; }
		return 0;
	}
	
	if (!is_empty) {
		const struct fsm_state *end;
		int n;

		if (err == NULL) {
			return 0;
		}

		/* TODO: would like to show the original spelling verbatim, too */
		
		/* fsm_example requires no epsilons;
		 * TODO: would fsm_glushkovise() here, when we have it */
		if (!fsm_determinise(cc->dup)) {
			err->e = RE_EERRNO;
			return 0;
		}

		/* XXX: this is just one example; really I want to show the entire set */
		end = fsm_any(cc->dup, fsm_isend);
		assert(end != NULL);
		assert(end != fsm_getstart(cc->dup)); /* due to the structure */
		
		n = fsm_example(cc->dup, end, err->dup, sizeof err->dup);
		if (n == -1) {
			err->e = RE_EERRNO;
			return 0;
		}
		
		/* due to the structure */
		assert(n > 0);
		
		/* XXX: to return when we can show minimal coverage again */
		strcpy(err->set, err->dup);
		
		err->e  = RE_EOVERLAP;
		return 0;
	}
	
	if (!fsm_minimise(cc->set)) {
		if (err != NULL) { err->e = RE_EERRNO; }
		return 0;
	}
	
	if (!fsm_unionxy(fsm, cc->set, x, y)) {
		if (err != NULL) { err->e = RE_EERRNO; }
		return 0;
	}
	cc->set = NULL;

	fsm_free(cc->dup);
	cc->dup = NULL;
		
	return 1;
}

int
cc_add_char(struct cc *cc, unsigned char c)
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
	
	if (!addedge_literal(fsm, cc->re_flags, start, end, c)) {
		goto fail;
	}
	
	return 1;

fail:
	return 0;
}

static int
cc_add_range(struct cc *cc, 
    const struct ast_range_endpoint *from,
    const struct ast_range_endpoint *to)
{
	unsigned char lower, upper;
	unsigned int i;

	if (from->type != AST_RANGE_ENDPOINT_LITERAL ||
	    to->type != AST_RANGE_ENDPOINT_LITERAL) {
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
cc_add_named_class(struct cc *cc, class_constructor *ctor)
{
	struct fsm *q;
	int r;
	struct re_err *err = cc->err;
	struct fsm *constructed = ctor(cc->opt);

	if (constructed == NULL) {
		if (err != NULL) { err->e = RE_EERRNO; }
		return 0;
	}

	/* TODO: maybe it is worth using carryopaque, after the entire group is constructed */
	{
		struct fsm *a, *b;
		
		a = fsm_clone(cc->set);
		if (a == NULL) {
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
		
		b = fsm_clone(constructed);
		if (b == NULL) {
			fsm_free(a);
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
		
		q = fsm_intersect(a, b);
		if (q == NULL) {
			fsm_free(a);
			fsm_free(b);
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
		
		r = fsm_empty(q);
		
		if (r == -1) {
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
	}
	
	if (!r) {
		cc->dup = fsm_union(cc->dup, q);
		if (cc->dup == NULL) {
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
	} else {
		fsm_free(q);
		
		cc->set = fsm_union(cc->set, constructed);
		if (cc->set == NULL) {
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
		
		/* we need a DFA here for sake of fsm_exec() identifying duplicates */
		if (!fsm_determinise(cc->set)) {
			if (err != NULL) { err->e = RE_EERRNO; }
			return 0;
		}
	}
	
	return 1;
}

static int
cc_invert(struct cc *cc)
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

static int
comp_iter(struct cc *cc, struct ast_class *n)
{
	assert(cc != NULL);
	assert(n != NULL);
	switch (n->type) {
	case AST_CLASS_CONCAT:
		if (!comp_iter(cc, n->u.concat.l)) { return 0; }
		if (!comp_iter(cc, n->u.concat.r)) { return 0; }
		break;
	case AST_CLASS_LITERAL:
		if (!cc_add_char(cc, n->u.literal.c)) { return 0; }
		break;
	case AST_CLASS_RANGE:
		if (!cc_add_range(cc, &n->u.range.from, &n->u.range.to)) { return 0; }
		break;
	case AST_CLASS_NAMED:
		if (!cc_add_named_class(cc, n->u.named.ctor)) { return 0; }
		break;
	case AST_CLASS_FLAGS:
		cc->flags |= n->u.flags.f;
		break;
	case AST_CLASS_SUBTRACT:
		assert(!"unimplemented");
	default:
		assert(!"unreached");
	}

	return 1;
}

int
ast_class_compile(struct ast_class *class,
    struct fsm *fsm, enum re_flags flags,
    struct re_err *err, const struct fsm_options *opt,
    struct fsm_state *x, struct fsm_state *y)
{
	struct cc cc;

	memset(&cc, 0x00, sizeof(cc));
	
	cc.set = new_blank(opt);
	if (cc.set == NULL) { goto cleanup; }

	cc.dup = new_blank(opt);
	if (cc.dup == NULL) { goto cleanup; }

	cc.opt = opt;
	cc.err = err;
	cc.re_flags = flags;

	if (!comp_iter(&cc, class)) { goto cleanup; }

	if (cc.flags & AST_CLASS_FLAG_INVERTED) {
		if (!cc_invert(&cc)) { goto cleanup; }
	}

	if (!link_class_into_fsm(&cc, fsm, x, y)) { goto cleanup; }

	return 1;

cleanup:
	if (cc.set != NULL) { fsm_free(cc.set); }
	if (cc.dup != NULL) { fsm_free(cc.dup); }
	return 0;
}

