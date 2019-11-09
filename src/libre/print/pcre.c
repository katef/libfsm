/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../class_lookup.h"
#include "../ast.h"
#include "../print.h"

static int
atomic(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_REPEATED:
	case AST_EXPR_CLASS:
	case AST_EXPR_GROUP:
		return 1;

	case AST_EXPR_FLAGS:
		return 0; /* XXX */

	default:
		assert(!"unreached");
	}
}

static void
re_flags_print(FILE *f, enum re_flags fl)
{
	const char *sep = "";

	if (fl & RE_ICASE  ) { fprintf(f, "%si", sep); sep = " "; }
	if (fl & RE_TEXT   ) { fprintf(f, "%sg", sep); sep = " "; }
	if (fl & RE_MULTI  ) { fprintf(f, "%sm", sep); sep = " "; }
	if (fl & RE_REVERSE) { fprintf(f, "%sr", sep); sep = " "; }
	if (fl & RE_SINGLE ) { fprintf(f, "%ss", sep); sep = " "; }
	if (fl & RE_ZONE   ) { fprintf(f, "%sz", sep); sep = " "; }
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, const struct ast_endpoint *e)
{
	switch (e->type) {
	case AST_ENDPOINT_LITERAL:
		pcre_escputc(f, opt, e->u.literal.c);
		break;

	default:
		assert(!"unreached");
		break;
	}
}

static void
print_class_name(FILE *f, const char *abstract_name)
{
	const char *p;

	assert(abstract_name != NULL);

	/*
	 * We have three different kinds of names to emit here:
	 *
	 * - Abstract names which map to corresponding special-purpose
	 *   pcre syntax. These class names are all lower case.
	 *
	 *   This is a many-to-one mapping (pcre has several spellings
	 *   for the same concept), and we defer to pcre_class_name()
	 *   to select which is preferred for a given abstract name.
	 *
	 * - Abstract names which have no corresponding pcre syntax.
	 *   These are spelled out explicitly.
	 *
	 * - Unicode. These are formatted long-hand, with \p{...}
	 */

	if (islower((unsigned char) abstract_name[0])) {
		const char *name;
		size_t i;

		static const struct {
			const char *name;
			const char *syntax;
		} a[] = {
			{ "any",   "." },
			{ "spchr", " " }
		};

		name = pcre_class_name(abstract_name);
		if (name != NULL) {
			/* the name here is an internal string, and assumed to not need escaping */
			fputs(name, f);
			return;
		}

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (0 == strcmp(a[i].name, name)) {
				fputs(a[i].name, f);
				return;
			}
		}
	}

	/*
	 * All non-Unicode class names should have been handled above;
	 * there should be none remaining which are lowercase and have
	 * no corresponding special-purpose pcre syntax.
	 */
	assert(!islower((unsigned char) abstract_name[0]));

	/* TODO: would handle "not" variants here as \P{...} */
	/* TODO: would prefer short forms, e.g. \p{Lc} rather than the full name */

	fputs("\\p{", f);
	for (p = abstract_name; *p != '\0'; p++) {
		if (*p == ' ') {
			fputc('_', f);
		} else {
			fputc(*p, f);
		}
	}
	fputs("}", f);
}

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt, struct ast_class *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	switch (n->type) {
	case AST_CLASS_CONCAT_N: {
		size_t i;

		for (i = 0; i < n->u.concat_n.count; i++) {
			cc_pp_iter(f, opt, n->u.concat_n.n[i]);
		}
		break;
	}

	case AST_CLASS_LITERAL:
		pcre_escputc(f, opt, n->u.literal.c);
		break;

	case AST_CLASS_RANGE:
		print_endpoint(f, opt, &n->u.range.from);
		fprintf(f, "-");
		print_endpoint(f, opt, &n->u.range.to);
		break;

	case AST_CLASS_NAMED:
		print_class_name(f, class_name(n->u.named.ctor));
		break;

	case AST_CLASS_FLAGS:
		if (n->u.flags.f & AST_CLASS_FLAG_INVERTED) {
			fprintf(f, "^");
		}
		if (n->u.flags.f & AST_CLASS_FLAG_MINUS) {
			fprintf(f, "-");
		}
		break;

	case AST_CLASS_SUBTRACT:
		fprintf(f, "\tn%p [ label = <{CLASS-SUBTRACT|{ast|mask}}> ];\n", (void *) n);
		cc_pp_iter(f, opt, n->u.subtract.ast);
		cc_pp_iter(f, opt, n->u.subtract.mask);
		break;

	default:
		assert(!"unreached");
	}
}

static void
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	switch (n->type) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		for (i = 0; i < n->u.concat_n.count; i++) {
			pp_iter(f, opt, n->u.concat_n.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT_N:
	{
		size_t i;
		for (i = 0; i < n->u.alt_n.count; i++) {
			pp_iter(f, opt, n->u.alt_n.n[i]);
			if (i + 1 < n->u.alt_n.count) {
				fprintf(f, "|");
			}
		}
		break;
	}

	case AST_EXPR_LITERAL:
		pcre_escputc(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_ANY:
		fprintf(f, ".");
		break;

	case AST_EXPR_REPEATED: {
		size_t i;

		static const struct {
			unsigned m;
			unsigned n;
			const char *op;
		} a[] = {
#define _ AST_COUNT_UNBOUNDED
			{ 0, 0, NULL },
			{ 0, _, "*"  },
			{ 0, 1, "?"  },
			{ 1, _, "+"  },
			{ 1, 1, NULL }
#undef _
		};

		if (atomic(n->u.repeated.e)) {
			pp_iter(f, opt, n->u.repeated.e);
		} else {
			fprintf(f, "(?:");
			pp_iter(f, opt, n->u.repeated.e);
			fprintf(f, ")");
		}

		assert(n->u.repeated.high == AST_COUNT_UNBOUNDED || n->u.repeated.high >= n->u.repeated.low);

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].m == n->u.repeated.low && a[i].n == n->u.repeated.high) {
				assert(a[i].op != NULL);
				fprintf(f, "%s", a[i].op);
				break;
			}
		}

		if (i == sizeof a / sizeof *a) {
			fprintf(f, "{");
			fprintf(f, "%u", n->u.repeated.low);
			if (n->u.repeated.high == n->u.repeated.low) {
				/* nothing */
			} else if (n->u.repeated.high == AST_COUNT_UNBOUNDED) {
				fprintf(f, ",");
			} else {
				fprintf(f, ",");
				fprintf(f, "%u", n->u.repeated.high);
			}
			fprintf(f, "}");
		}

		break;
	}

	case AST_EXPR_CLASS:
		fprintf(f, "[");
		cc_pp_iter(f, opt, n->u.class.class);
		fprintf(f, "]");
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.type == AST_ANCHOR_START ? "^" : "$");
		break;

	case AST_EXPR_FLAGS:
		fprintf(f, "\tn%p [ label = <{FLAGS|{+", (void *) n);
		re_flags_print(f, n->u.flags.pos);
		fprintf(f, "|-");
		re_flags_print(f, n->u.flags.neg);
		fprintf(f, "> ];\n");
		break;

	default:
		assert(!"unreached");
	}
}

void
ast_print_pcre(FILE *f, const struct fsm_options *opt,
	const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	pp_iter(f, opt, ast->expr);

	fprintf(f, "\n");
}

