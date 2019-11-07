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
#include "ast_compile.h"

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
	struct re_err *err;

	/*
	 * This is set to NULL once it's been incorporated into the
	 * overall regex FSM, so it can no longer need to be freed by the
	 * char class functions.
	 */
	struct fsm *set;
};

static struct fsm *
new_blank(const struct fsm_options *opt)
{
	struct fsm_state *start;
	struct fsm *new;
	
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
	struct re_err *err = cc->err;

	assert(cc != NULL);

	if (!fsm_minimise(cc->set)) {
		if (err != NULL) { err->e = RE_EERRNO; }
		return 0;
	}
	
	if (!fsm_unionxy(fsm, cc->set, x, y)) {
		if (err != NULL) { err->e = RE_EERRNO; }
		return 0;
	}

	cc->set = NULL;
		
	return 1;
}

int
cc_add_char(struct cc *cc, unsigned char c)
{
	struct fsm_state *start, *end;
	
	assert(cc != NULL);
	
	start = fsm_getstart(cc->set);
	assert(start != NULL);
	
	end = fsm_addstate(cc->set);
	if (end == NULL) {
		return 0;
	}
	
	fsm_setend(cc->set, end, 1);
	
	if (!addedge_literal(cc->set, cc->re_flags, start, end, c)) {
		return 0;
	}
	
	return 1;
}

static int
cc_add_range(struct cc *cc, 
	const struct ast_endpoint *from,
	const struct ast_endpoint *to)
{
	unsigned char lower, upper;
	unsigned int i;

	if (from->type != AST_ENDPOINT_LITERAL || to->type != AST_ENDPOINT_LITERAL) {
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
	struct fsm *constructed;

	constructed = ctor(fsm_getoptions(cc->set));
	if (constructed == NULL) {
		goto error;
	}

	cc->set = fsm_union(cc->set, constructed);
	if (cc->set == NULL) {
		goto error;
	}

	return 1;

error:

	if (cc->err != NULL) {
		cc->err->e = RE_EERRNO;
	}

	return 0;
}

static int
cc_invert(struct cc *cc)
{
	struct fsm *inverted;

	inverted = negate(cc->set, fsm_getoptions(cc->set));
	if (inverted == NULL) {
		return 0;
	}

	cc->set = inverted;

	return 1;
}

static int
comp_iter(struct cc *cc, const struct ast_class *n)
{
	assert(cc != NULL);
	assert(n != NULL);

	switch (n->type) {
	case AST_CLASS_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			if (!comp_iter(cc, n->u.concat.n[i])) {
				return 0;
			}
		}
		break;
	}

	case AST_CLASS_LITERAL:
		if (!cc_add_char(cc, n->u.literal.c)) {
			return 0;
		}
		break;

	case AST_CLASS_RANGE:
		if (!cc_add_range(cc, &n->u.range.from, &n->u.range.to)) {
			return 0;
		}
		break;

	case AST_CLASS_NAMED:
		if (!cc_add_named_class(cc, n->u.named.ctor)) {
			return 0;
		}
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
ast_compile_class(const struct ast_class *class,
	struct fsm *fsm, enum re_flags flags,
	struct re_err *err,
	struct fsm_state *x, struct fsm_state *y)
{
	const struct fsm_options *opt;
	struct cc cc;

	memset(&cc, 0x00, sizeof(cc));
	
	opt = fsm_getoptions(fsm);

	cc.set = new_blank(opt);
	if (cc.set == NULL) {
		goto error;
	}

	cc.err = err;
	cc.re_flags = flags;

	if (!comp_iter(&cc, class)) {
		goto error;
	}

	if (cc.flags & AST_CLASS_FLAG_INVERTED) {
		if (!cc_invert(&cc)) {
			goto error;
		}
	}

	if (!link_class_into_fsm(&cc, fsm, x, y)) {
		goto error;
	}

	return 1;

error:

	if (cc.set != NULL) {
		fsm_free(cc.set);
	}

	return 0;
}

