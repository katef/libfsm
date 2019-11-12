/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../ast.h"
#include "../print.h"

static void
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n);

static int
atomic(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CLASS:
	case AST_EXPR_GROUP:
		return 1;

	case AST_EXPR_REPEATED:
		return 0;

	case AST_EXPR_FLAGS:
		return 0; /* XXX */

	default:
		assert(!"unreached");
	}
}

static void
print_class_name(FILE *f, const char *abstract_name)
{
	const char *p;

	/*
	 * We have three kinds of names to emit here:
	 *
	 * - ABNF provides a set of pre-defined _Core Rules_ (RFC 5234 B.1); we
	 *   provide a mapping to emit these names in preference, for our classes
	 *   which are equivalent. These class names are all lower case.
	 *
	 * - Our internal names may have spaces. These are used for UTF-8 names,
	 *   which always appear in title case. For ABNF output these are printed
	 *   as rules, and rule names must have no spaces (at least according to
	 *   kgt's implementation).
	 *
	 * - Aside from handling spaces, abstract class names are internal strings,
	 *   assumed to not need escaping.
	 */

	static const struct {
		const char *name;
		const char *rule;
	} core_rules[] = {
		{ "alpha",  "ALPHA"  }, /* %x41-5A / %x61-7A ; A-Z / a-z */
		{ "ascii",  "CHAR"   }, /* %x01-7F */
		{ "cntrl",  "CTL"    }, /* %x00-1F / %x7F */
		{ "digit",  "DIGIT"  }, /* %x30-39 ; 0-9 */
		{ "xdigit", "HEXDIG" }, /* DIGIT / "A" / "B" / "C" / "D" / "E" / "F" */
		{ "any",    "OCTET"  }, /* %x00-FF */
		{ "spchr",  "SP"     }, /* %x20 */
		{ "graph",  "VCHAR"  }, /* %x21-7E ; visible (printing) characters */
		{ "hspace", "WSP"    }  /* SP / HTAB */

		/*
		 * No equivalent for: BIT, CR, DQUOTE, HTAB, LF
		 * Non-class core rules: CRLF, LWSP
		 */
	};

	assert(abstract_name != NULL);

	if (islower((unsigned char) abstract_name[0])) {
		size_t i;

		for (i = 0; i < sizeof core_rules / sizeof *core_rules; i++) {
			if (0 == strcmp(core_rules[i].name, abstract_name)) {
				fputs(core_rules[i].rule, f);
				return;
			}
		}
	}

	for (p = abstract_name; *p != '\0'; p++) {
		if (*p == ' ') {
			continue;
		}

		fputc(*p, f);
	}
}

static void
print_grouped(FILE *f, const struct fsm_options *opt, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	if (atomic(n)) {
		pp_iter(f, opt, n);
	} else {
		fprintf(f, "(");
		pp_iter(f, opt, n);
		fprintf(f, ")");
	}
}

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt, struct ast_class *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	switch (n->type) {
	case AST_CLASS_LITERAL:
		abnf_escputc(f, opt, n->u.literal.c);
		break;

	case AST_CLASS_RANGE: {
		if (n->u.range.from.type != AST_ENDPOINT_LITERAL
		 || n->u.range.to.type != AST_ENDPOINT_LITERAL)
		{
			assert(!"unimplemented");
			abort();
		}

		fprintf(f, "%%x%02X-%02X",
			(unsigned char) n->u.range.from.u.literal.c,
			(unsigned char) n->u.range.to.u.literal.c);
		}
		break;

	case AST_CLASS_NAMED:
		print_class_name(f, class_name(n->u.named.ctor));
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

	if (n == NULL) {
		return;
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			pp_iter(f, opt, n->u.concat.n[i]);
			if (i + 1 < n->u.concat.count) {
				fprintf(f, " ");
			}
		}
		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			pp_iter(f, opt, n->u.alt.n[i]);
			if (i + 1 < n->u.alt.count) {
				fprintf(f, " / "); /* XXX: indent */
			}
		}
		break;
	}

	case AST_EXPR_LITERAL:
		abnf_escputc(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_ANY:
		fprintf(f, "OCTET"); /* ABNF core rule for %x00-FF */
		break;

	case AST_EXPR_REPEATED: {
		assert(n->u.repeated.low != AST_COUNT_UNBOUNDED);

		if (n->u.repeated.low == 0 && n->u.repeated.high == 1) {
			fprintf(f, "[ ");
			pp_iter(f, opt, n->u.repeated.e);
			fprintf(f, " ]");
			return;
		}

		if (n->u.repeated.low == 1 && n->u.repeated.high == 1) {
			pp_iter(f, opt, n->u.repeated.e);
			return;
		}

		if (n->u.repeated.low == n->u.repeated.high) {
			fprintf(f, "%u", n->u.repeated.high);
			print_grouped(f, opt, n->u.repeated.e);
			return;
		}

		if (n->u.repeated.low > 0) {
			fprintf(f, "%u", n->u.repeated.low);
		}

		fprintf(f, "*");

		if (n->u.repeated.high != AST_COUNT_UNBOUNDED) {
			fprintf(f, "%u", n->u.repeated.high);
		}

		print_grouped(f, opt, n->u.repeated.e);

		break;
	}

	case AST_EXPR_CLASS: {
		size_t i;

		fprintf(f, "(");

		for (i = 0; i < n->u.class.count; i++) {
			cc_pp_iter(f, opt, n->u.class.n[i]);

			if (i + 1 < n->u.class.count) {
				fprintf(f, " / ");
			}
		}

		fprintf(f, ")");
		break;
	}

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START
		    || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.type == AST_ANCHOR_START ? "<^>" : "<$>");
		break;

	case AST_EXPR_SUBTRACT:
		assert(!"unimplemented");
		pp_iter(f, opt, n->u.subtract.a);
		fprintf(f, " - ");
		pp_iter(f, opt, n->u.subtract.b);
		break;

	case AST_EXPR_INVERT:
		assert(!"unimplemented");
		fprintf(f, " - ");
		pp_iter(f, opt, n->u.invert.e);
		break;

	case AST_EXPR_FLAGS:
		abort();

	default:
		assert(!"unreached");
	}
}

void
ast_print_abnf(FILE *f, const struct fsm_options *opt,
	const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	fprintf(f, "e = ");

	pp_iter(f, opt, ast->expr);

	fprintf(f, "\n");
	fprintf(f, "\n");
}

