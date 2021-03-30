/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_re.h"

#include <string.h>
#include <limits.h>

#include <adt/xalloc.h>

static enum theft_alloc_res
re_alloc(struct theft *t, void *penv, void **output)
{
	struct test_re_info *res;
	struct test_env *env;

	env = penv;

	assert(env->tag == 'E');

	res = xcalloc(1, sizeof *res);

	res->tag = 'r';

	enum theft_alloc_res ares;
	switch (env->dialect) {
	case RE_LITERAL:
		ares = type_info_re_literal_build_info(t, res);
		break;

	case RE_PCRE:
		ares = type_info_re_pcre_build_info(t, res);
		break;

	default:
		fprintf(stderr, "NYI\n");
		ares = THEFT_ALLOC_ERROR;
		break;
	}

	if (ares != THEFT_ALLOC_OK) {
		free(res);
		return ares;
	}

	res->dialect = env->dialect;

	*output = res;

	return THEFT_ALLOC_OK;
}

static void
re_free(void *instance, void *env)
{
	struct test_re_info *info;

	(void) env;

	info = instance;

	assert(info->tag == 'r');

	switch (info->dialect) {
	case RE_LITERAL:
		/* we're just matching the string literal against itself */
		break;

	case RE_PCRE:
		free(info->string);
		type_info_re_pcre_free(info->u.pcre.head);
		break;

	default:
		assert(false);
	}

	for (size_t i = 0; i < info->pos_count; i++) {
		free(info->pos_pairs[i].string);
	}
	free(info->pos_pairs);

	for (size_t i = 0; i < info->neg_count; i++) {
		free(info->neg_pairs[i].string);
	}

	free(info->neg_pairs);
	free(info);
}

#define MAX_INDENT 80
#define PRINT_PCRE_AST 1
static void
re_print_ast(FILE *f, const struct pcre_node *n, size_t indent, char *indent_buf)
{
	if (indent > MAX_INDENT) {
		indent = MAX_INDENT;
	}
	for (size_t i = 0; i < indent; i++) {
		indent_buf[i] = ' ';
	}
	indent_buf[indent] = '\0';

	assert(n != NULL);
	switch (n->t) {
	case PN_DOT:
		fprintf(f, "%s- dot\n", indent_buf);
		break;
	case PN_LITERAL:
		fprintf(f, "%s- literal: \"%s\"\n", indent_buf, n->u.literal.string);
		break;
	case PN_QUESTION:
		fprintf(f, "%s- question:\n", indent_buf);
		re_print_ast(f, n->u.question.inner, indent + 4, indent_buf);
		break;
	case PN_KLEENE:
		fprintf(f, "%s- kleene:\n", indent_buf);
		re_print_ast(f, n->u.kleene.inner, indent + 4, indent_buf);
		break;
	case PN_PLUS:
		fprintf(f, "%s- plus:\n", indent_buf);
		re_print_ast(f, n->u.plus.inner, indent + 4, indent_buf);
		break;
	case PN_BRACKET:
		fprintf(f, "%s- bracket %s0x%x -- [0x%016lx,%016lx,%016lx,%016lxx] -- ",
		    indent_buf, n->u.bracket.negated ? "NEG " : "POS ",
		    n->u.bracket.class_flags,
		    n->u.bracket.set[0], n->u.bracket.set[1],
		    n->u.bracket.set[2], n->u.bracket.set[3]);
		for (unsigned i = 0; i < 256; i++) {
			if (n->u.bracket.set[i/64] & (1ULL << (i&63))) {
				fprintf(f, "%c", isprint(i) ? i : '.');
			}
		}
		fprintf(f, "\n");
		break;
	case PN_ALT:
		fprintf(f, "%s- alts: (%zu)\n", indent_buf, n->u.alt.count);
		for (size_t i = 0; i < n->u.alt.count; i++) {
			re_print_ast(f, n->u.alt.alts[i], indent + 4, indent_buf);
		}
		break;
	case PN_ANCHOR:
	{
		const enum pcre_anchor_type t = n->u.anchor.type;
		fprintf(f, "%s- anchor: %s\n", indent_buf,
		    t == PCRE_ANCHOR_START ? "START"
		    : t == PCRE_ANCHOR_END ? "END"
		    : "<none>");
		re_print_ast(f, n->u.anchor.inner, indent + 4, indent_buf);
		break;
	}
	default:
		break;
	}
}

static void
re_print(FILE *f, const void *instance, void *env)
{
	const struct test_re_info *info = instance;

	(void) env;

	assert(info->tag == 'r');

	/* RE string is built in the test */
	fprintf(f, "re_info{%p}, %zd: \n",
		(void *) info, info->pos_count);

	hexdump(f, info->string, info->size);
	fprintf(f, "\n");

	if (PRINT_PCRE_AST && info->dialect == RE_PCRE) {
		char indent[MAX_INDENT];
		fprintf(f, "== AST:\n");
		re_print_ast(f, info->u.pcre.head, 0, indent);
		fprintf(f, "\n");
	}

	if (info->pos_count > 0) {
		fprintf(f, "## Positive test string(s): %zd\n", info->pos_count);
	}
	for (size_t li = 0; li < info->pos_count; li++) {
		struct string_pair *pair = &info->pos_pairs[li];
		const uint8_t *str = pair->string;
		size_t len = pair->size;

		fprintf(f, "-- %zd [%zd]: ", li, len);
		hexdump(f, str, len);
		fprintf(f, "\n");
	}

	if (info->neg_count > 0) {
		fprintf(f, "## Negative test string(s): %zd\n", info->neg_count);
	}
	for (size_t li = 0; li < info->neg_count; li++) {
		struct string_pair *pair;
		uint8_t *str;
		size_t len;

		pair = &info->neg_pairs[li];
		str  = pair->string;

		if (str == NULL) {
			fprintf(f, "-- %zd: [rejected]\n", li);
			continue;
		}

		len = pair->size;

		fprintf(f, "-- %zd [%zd]: ", li, len);
		hexdump(f, str, len);
		fprintf(f, "\n");
	}
}

const struct theft_type_info type_info_re = {
	.alloc = re_alloc,
	.free  = re_free,
	.print = re_print,
	.autoshrink_config = {
		.enable = true,
	}
};

