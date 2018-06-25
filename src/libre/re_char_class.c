#include "re_char_class.h"
#include "re_ast.h"

#include <string.h>
#include <ctype.h>

#define TRACE_OPERATIONS 0

typedef int char_filter(int c);

static void
cc_free(struct re_char_class *cc);

static void
cc_add_byte(struct re_char_class *cc, unsigned char byte);

static void
cc_add_range(struct re_char_class *cc, 
    const struct ast_range_endpoint *from,
    const struct ast_range_endpoint *to);

static int
cc_add_named_class(struct re_char_class *cc, enum ast_class_id id);

static int
cc_add_char_type(struct re_char_class *cc, enum ast_char_type_id id);

static int
cc_add_char_type(struct re_char_class *cc, enum ast_char_type_id id);

static void
cc_invert(struct re_char_class *cc);

static void
cc_mask(struct re_char_class *cc, struct re_char_class *mask);

void
cc_dump(FILE *f, struct re_char_class *cc);

static const char *
char_class_id_str(enum ast_class_id id);

static const char *
char_type_id_str(enum ast_char_type_id id);

static char_filter *
char_filter_for_class_id(enum ast_class_id id);

static void
char_endpoints_for_filter(char_filter *f, unsigned negated,
    unsigned char *low, unsigned char *high);

static int
char_filter_for_char_type_id(enum ast_char_type_id id,
    char_filter **f, unsigned *negated_res);

static void
pp_range_endpoint(FILE *f, const struct ast_range_endpoint *r);

struct re_char_class_ast *
re_char_class_ast_concat(struct re_char_class_ast *l,
    struct re_char_class_ast *r)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_CONCAT;
	res->u.concat.l = l;
	res->u.concat.r = r;
	LOG("-- %s: %p <- %p, %p\n", __func__,
	    (void *)res, (void *)l, (void *)r);
	return res;
}

struct re_char_class_ast *
re_char_class_ast_literal(unsigned char c)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_LITERAL;
	res->u.literal.c = c;
	LOG("-- %s: %p <- '%c'\n", __func__, (void *)res, c);
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
	LOG("-- %s: %p <- ", __func__, (void *)res);
#if PRETTYPRINT_AST
	pp_range_endpoint(stderr, &res->u.range.from);
	LOG("-");
	pp_range_endpoint(stderr, &res->u.range.to);
	LOG("\n");
#endif
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
re_char_class_ast_named_class(enum ast_class_id id)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_NAMED;
	res->u.named.id = id;
	LOG("-- %s: %p <- [:%s:]\n", __func__, (void *)res, char_class_id_str(id));
	return res;
}

struct re_char_class_ast *
re_char_class_ast_char_type(enum ast_char_type_id id)
{
	struct re_char_class_ast *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = RE_CHAR_CLASS_AST_CHAR_TYPE;
	res->u.char_type.id = id;
	LOG("-- %s: %p <- [:%s:]\n", __func__, (void *)res, char_type_id_str(id));
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
	LOG("-- %s: %p <- %p ^ %p\n", __func__, (void *)res, (void *)ast, (void *)mask);
	return res;
}

static void
print_char_or_esc(FILE *f, unsigned char c)
{
	if (isprint(c)) {
		fprintf(f, "%c", c);
	} else {
		fprintf(f, "\\x%02x", c);
	}
}

static const char *
char_class_id_str(enum ast_class_id id)
{
	switch (id) {
	case AST_CLASS_ALNUM: return "ALNUM";
	case AST_CLASS_ALPHA: return "ALPHA";
	case AST_CLASS_ANY: return "ANY";
	case AST_CLASS_ASCII: return "ASCII";
	case AST_CLASS_BLANK: return "BLANK";
	case AST_CLASS_CNTRL: return "CNTRL";
	case AST_CLASS_DIGIT: return "DIGIT";
	case AST_CLASS_GRAPH: return "GRAPH";
	case AST_CLASS_LOWER: return "LOWER";
	case AST_CLASS_PRINT: return "PRINT";
	case AST_CLASS_PUNCT: return "PUNCT";
	case AST_CLASS_SPACE: return "SPACE";
	case AST_CLASS_SPCHR: return "SPCHR";
	case AST_CLASS_UPPER: return "UPPER";
	case AST_CLASS_WORD: return "WORD";
	case AST_CLASS_XDIGIT: return "XDIGIT";
	default:
		return "<matchfail>";
	}
}

/* While this could be done via the lexer, doing it here
 * greatly reduces the surface area of the types we need to
 * declare in the parser. */
int
re_char_class_id_of_name(const char *name, enum ast_class_id *id)
{
	struct pairs {
		const char *s;
		enum ast_char_type_id id;
	};
	const struct pairs table[] = {
		/* the pcre dialect uses lowercase IDs */
		{ "alnum", AST_CLASS_ALNUM },
		{ "alpha", AST_CLASS_ALPHA },
		{ "ascii", AST_CLASS_ASCII },
		{ "blank", AST_CLASS_BLANK },
		{ "cntrl", AST_CLASS_CNTRL },
		{ "digit", AST_CLASS_DIGIT },
		{ "graph", AST_CLASS_GRAPH },
		{ "lower", AST_CLASS_LOWER },
		{ "print", AST_CLASS_PRINT },
		{ "punct", AST_CLASS_PUNCT },
		{ "space", AST_CLASS_SPACE },
		{ "upper", AST_CLASS_UPPER },
		{ "word", AST_CLASS_WORD },
		{ "xdigit", AST_CLASS_XDIGIT },

		/* the SQL dialect uses uppercase IDs */
		{ "ALNUM", AST_CLASS_ALNUM },
		{ "ALPHA", AST_CLASS_ALPHA },
		{ "DIGIT", AST_CLASS_DIGIT },
		{ "LOWER", AST_CLASS_LOWER },
		{ "SPACE", AST_CLASS_SPACE },
		{ "UPPER", AST_CLASS_UPPER },
		{ "WHITESPACE", AST_CLASS_SPACE },
	};
	unsigned i;

	if (name == NULL || name[0] != '[' || name[1] != ':') {
		return 0;
	}
	name += 2;

	for (i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
		if (0 == strncmp(table[i].s, name, strlen(table[i].s))) {
			*id = table[i].id;
			return 1;
		}
	}
	return 0;
}

static const char *
char_type_id_str(enum ast_char_type_id id)
{
	switch (id) {
	case AST_CHAR_TYPE_DECIMAL: return "\\d";
	case AST_CHAR_TYPE_HORIZ_WS: return "\\h";
	case AST_CHAR_TYPE_WS: return "\\s";
	case AST_CHAR_TYPE_VERT_WS: return "\\v";
	case AST_CHAR_TYPE_WORD: return "\\w";
	case AST_CHAR_TYPE_NON_DECIMAL: return "\\D";
	case AST_CHAR_TYPE_NON_HORIZ_WS: return "\\H";
	case AST_CHAR_TYPE_NON_WS: return "\\S";
	case AST_CHAR_TYPE_NON_VERT_WS: return "\\V";
	case AST_CHAR_TYPE_NON_WORD: return "\\W";
	case AST_CHAR_TYPE_NON_NL: return "\\N";
	default:
		return "<match fail>";
	}
}

int
re_char_class_type_id_of_name(const char *name,
    enum ast_char_type_id *id)
{
	assert(name != NULL);
	assert(id != NULL);

	if (name[0] != '\\') { return 0; }
	if (name[1] == '\0' || name[2] != '\0') { return 0; }

	switch (name[1]) {
	case 'd': *id = AST_CHAR_TYPE_DECIMAL; break;
	case 'h': *id = AST_CHAR_TYPE_HORIZ_WS; break;
	case 's': *id = AST_CHAR_TYPE_WS; break;
	case 'v': *id = AST_CHAR_TYPE_VERT_WS; break;
	case 'w': *id = AST_CHAR_TYPE_WORD; break;
	case 'D': *id = AST_CHAR_TYPE_NON_DECIMAL; break;
	case 'H': *id = AST_CHAR_TYPE_NON_HORIZ_WS; break;
	case 'S': *id = AST_CHAR_TYPE_NON_WS; break;
	case 'V': *id = AST_CHAR_TYPE_NON_VERT_WS; break;
	case 'W': *id = AST_CHAR_TYPE_NON_WORD; break;
	case 'N': *id = AST_CHAR_TYPE_NON_NL; break;
	default:
		return 0;
	}
	return 1;
}

static void
pp_range_endpoint(FILE *f, const struct ast_range_endpoint *r)
{
	switch (r->t) {
	case AST_RANGE_ENDPOINT_LITERAL:
		fprintf(f, "'");
		print_char_or_esc(f, r->u.literal.c);
		fprintf(f, "'");
		break;
	case AST_RANGE_ENDPOINT_CHAR_TYPE:
		fprintf(f, "%s", char_type_id_str(r->u.char_type.id));
		break;
	case AST_RANGE_ENDPOINT_CHAR_CLASS:
		fprintf(f, "[:%s:]", char_class_id_str(r->u.char_class.id));
		break;
	}
}

static void
pp_iter(FILE *f, struct re_char_class_ast *n, size_t indent)
{
	size_t i;
	assert(n != NULL);
	for (i = 0; i < indent; i++) { fprintf(f, " "); }

	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
		fprintf(f, "CLASS-CONCAT %p: \n", (void *)n);
		pp_iter(f, n->u.concat.l, indent + 4);
		pp_iter(f, n->u.concat.r, indent + 4);
		break;
	case RE_CHAR_CLASS_AST_LITERAL:
		fprintf(f, "CLASS-LITERAL %p: '", (void *)n);
		print_char_or_esc(f, n->u.literal.c);
		fprintf(f, "'\n");
		break;
	case RE_CHAR_CLASS_AST_RANGE:
		fprintf(f, "CLASS-RANGE %p: ", (void *)n);
		pp_range_endpoint(f, &n->u.range.from);
		fprintf(f, "-");
		pp_range_endpoint(f, &n->u.range.to);
		fprintf(f, "\n");
		break;
	case RE_CHAR_CLASS_AST_NAMED:
		fprintf(f, "CLASS-NAMED %p: '%s'\n",
		    (void *)n, char_class_id_str(n->u.named.id));
		break;
	case RE_CHAR_CLASS_AST_CHAR_TYPE:
		fprintf(f, "CLASS-CHAR-TYPE %p: %s\n", (void *)n, char_type_id_str(n->u.char_type.id));
		break;
	case RE_CHAR_CLASS_AST_FLAGS:
		fprintf(f, "CLASS-FLAGS %p: [", (void *)n);
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_INVERTED) { 
			fprintf(f, "^");
		}
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_MINUS) {
			fprintf(f, "-");
		}
		fprintf(f, "]\n");
		break;
	case RE_CHAR_CLASS_AST_SUBTRACT:
		fprintf(f, "CLASS-SUBTRACT %p:\n", (void *)n);
		pp_iter(f, n->u.subtract.ast, indent + 4);
		pp_iter(f, n->u.subtract.mask, indent + 4);
		break;
	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
}

void
re_char_class_ast_prettyprint(FILE *f,
    struct re_char_class_ast *ast, size_t indent)
{
	pp_iter(f, ast, indent);
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
	case RE_CHAR_CLASS_AST_CHAR_TYPE:
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
		cc_add_byte(cc, n->u.literal.c);
		break;
	case RE_CHAR_CLASS_AST_RANGE:
		cc_add_range(cc, &n->u.range.from, &n->u.range.to);
		break;
	case RE_CHAR_CLASS_AST_NAMED:
		if (!cc_add_named_class(cc, n->u.named.id)) { return 0; }
		break;
	case RE_CHAR_CLASS_AST_CHAR_TYPE:
		if (!cc_add_char_type(cc, n->u.char_type.id)) { return 0; }
		break;
	case RE_CHAR_CLASS_AST_FLAGS:
		cc->flags |= n->u.flags.f;
		break;
	case RE_CHAR_CLASS_AST_SUBTRACT:
	{
		struct re_char_class *mask;
		if (!comp_iter(cc, n->u.subtract.ast)) { return 0; }

		mask = re_char_class_ast_compile(n->u.subtract.mask);
		if (mask == NULL) { return 0; }
		cc_mask(cc, mask);
		re_char_class_free(mask);
		break;
	}
	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}

	return 1;
}

struct re_char_class *
re_char_class_ast_compile(struct re_char_class_ast *cca)
{
	struct re_char_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }

	if (!comp_iter(res, cca)) {
		cc_free(res);
		return NULL;
	}

	if (res->flags & RE_CHAR_CLASS_FLAG_MINUS) {
		cc_add_byte(res, '-');
		res->flags &=~ RE_CHAR_CLASS_FLAG_MINUS;
	}

	if (res->flags & RE_CHAR_CLASS_FLAG_INVERTED) {
		cc_invert(res);
		res->flags &=~ RE_CHAR_CLASS_FLAG_INVERTED;
	}

	return res;
}

struct re_char_class *
re_char_class_type_compile(enum ast_char_type_id id)
{
	struct re_char_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }

	cc_add_char_type(res, id);
	
	return res;
}


static void
bitset_pos(unsigned char byte, unsigned *pos, unsigned char *bit)
{
	*pos = byte / 8;
	*bit = 1U << (byte & 0x07);
}

static void
cc_free(struct re_char_class *c)
{
	free(c);
}

void
cc_add_byte(struct re_char_class *cc, unsigned char byte)
{
	unsigned pos;
	unsigned char bit;
	assert(cc != NULL);
	bitset_pos(byte, &pos, &bit);
	cc->chars[pos] |= bit;

#if TRACE_OPERATIONS
	fprintf(stderr, "ADDING 0x%02x\n", byte);
	cc_dump(stderr, cc);
	fprintf(stderr, "\n");
#endif
}

void
re_char_class_endpoint_span(const struct ast_range_endpoint *r,
    unsigned char *from, unsigned char *to)
{
	char_filter *f = NULL;
	assert(r != NULL);

	switch (r->t) {
	case AST_RANGE_ENDPOINT_LITERAL:
		if (from != NULL) { *from = r->u.literal.c; }
		if (to != NULL) { *to = r->u.literal.c; }
		break;
	case AST_RANGE_ENDPOINT_CHAR_TYPE:
	{
		unsigned negated = 0;
		char_filter_for_char_type_id(r->u.char_type.id, &f, &negated);
		char_endpoints_for_filter(f, negated, from, to);
		break;
	}
	case AST_RANGE_ENDPOINT_CHAR_CLASS:
		f = char_filter_for_class_id(r->u.char_class.id);
		assert(f != NULL);
		char_endpoints_for_filter(f, 0, from, to);
		break;
	default:
		assert(0);
	}
}

static void
char_endpoints_for_filter(char_filter *f, unsigned negated,
    unsigned char *low, unsigned char *high)
{
	unsigned i, highest;
	int has_l = 0, has_h = 0;

	assert(f != NULL);
	for (i = 0; i < 256; i++) {
		int match = f(i);
		if ((negated && match) || (!negated && !match)) {
			continue;
		}
		if (low != NULL && !has_l) {
			has_l = 1;
			*low = i;
			if (high == NULL) { return; }
		}

		has_h = 1;
		highest = i;
	}

	if (high != NULL && has_h) {
		*high = highest;
	}
}

static void
cc_add_range(struct re_char_class *cc, 
    const struct ast_range_endpoint *from,
    const struct ast_range_endpoint *to)
{
	unsigned char lower, upper;
	unsigned char i;

	re_char_class_endpoint_span(from, &lower, NULL);
	re_char_class_endpoint_span(to, NULL, &upper);

	assert(cc != NULL);
	assert(lower <= upper);
	for (i = lower; i <= upper; i++) {
		cc_add_byte(cc, i);		
	}
}

static char_filter *
char_filter_for_class_id(enum ast_class_id id)
{
	char_filter *filter = NULL;
	switch (id) {
	case AST_CLASS_ALNUM:
		filter = isalnum; break;
	case AST_CLASS_ALPHA:
		filter = isalpha; break;
	case AST_CLASS_ANY:
		break;
	case AST_CLASS_ASCII:
		/* filter = isascii; */
		break;
	case AST_CLASS_BLANK:
		/* filter = isblank; */
		break;
	case AST_CLASS_CNTRL:
		filter = iscntrl; break;
	case AST_CLASS_DIGIT:
		filter = isdigit; break;
	case AST_CLASS_GRAPH:
		filter = isgraph; break;
	case AST_CLASS_LOWER:
		filter = islower; break;
	case AST_CLASS_PRINT:
		filter = isprint; break;
	case AST_CLASS_PUNCT:
		filter = ispunct; break;
	case AST_CLASS_SPACE:
		filter = isspace; break;
	case AST_CLASS_SPCHR:
		/* filter = isspchr; */
		break;
	case AST_CLASS_UPPER:
		filter = isupper; break;
	case AST_CLASS_WORD:
		/* filter = isword; */
		break;
	case AST_CLASS_XDIGIT:
		filter = isxdigit; break;
	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
	return filter;
}

static int
cc_add_named_class(struct re_char_class *cc, enum ast_class_id id)
{
	unsigned i;
	char_filter *filter = NULL;
	assert(cc != NULL);

	filter = char_filter_for_class_id(id);

	if (filter == NULL) {
		return 0;
	}

	for (i = 0; i < 256; i++) {
		if (filter(i)) { cc_add_byte(cc, i); }
	}
	return 1;
}

static int
f_hws(int c)
{
	switch (c) {
	case ' ':
	case '\t':
		return 1;
	default:
		return 0;
	}
}

static int
f_vws(int c)
{
	switch (c) {
	case '\x0a':		/* AKA '\n' */
	case '\x0b':		/* vertical tab */
	case '\x0c':		/* form feed */
	case '\x0d':		/* AKA '\r' */
		return 1;
	default:
		return 0;
	}
}

static int
f_word(int c)
{
	if (isalnum(c)) { return 1; }
	if (c == '_') { return 1; }
	return 0;
}

static int
f_nl(int c)
{
	return (c == '\n');
}

static int
char_filter_for_char_type_id(enum ast_char_type_id id,
    char_filter **f, unsigned *negated_res)
{
	char_filter *filter = NULL;
	unsigned negated = 0;

	switch (id) {
	case AST_CHAR_TYPE_DECIMAL:
		filter = isdigit; negated = 0; break;
	case AST_CHAR_TYPE_HORIZ_WS:
		filter = f_hws; negated = 0; break;
	case AST_CHAR_TYPE_WS:
		filter = isspace; negated = 0; break;
	case AST_CHAR_TYPE_VERT_WS:
		filter = f_vws; negated = 0; break;
	case AST_CHAR_TYPE_WORD:
		filter = f_word; negated = 0; break;

	/* negated */
	case AST_CHAR_TYPE_NON_DECIMAL:
		filter = isdigit; negated = 1; break;
	case AST_CHAR_TYPE_NON_HORIZ_WS:
		filter = f_hws; negated = 1; break;
	case AST_CHAR_TYPE_NON_WS:
		filter = isspace; negated = 1; break;
	case AST_CHAR_TYPE_NON_VERT_WS:
		filter = f_vws; negated = 1; break;
	case AST_CHAR_TYPE_NON_WORD:
		filter = f_word; negated = 1; break;
	case AST_CHAR_TYPE_NON_NL:
		filter = f_nl; negated = 1; break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		return 0;
	}

	if (filter == NULL) { return 0; }
	*f = filter;
	*negated_res = negated;

	return 1;
}

static int
cc_add_char_type(struct re_char_class *cc, enum ast_char_type_id id)
{
	unsigned i, negated;
	char_filter *filter = NULL;
	assert(cc != NULL);

	if (!char_filter_for_char_type_id(id, &filter, &negated)) {
		return 0;
	}

	if (filter == NULL) {
		return 0;
	}

	for (i = 0; i < 256; i++) {
		if (filter(i)) { cc_add_byte(cc, i); }
	}

	if (negated) {
		cc_invert(cc);
	}
	
	return 1;
}

void
cc_invert(struct re_char_class *cc)
{
	unsigned i;
	for (i = 0; i < sizeof(cc->chars)/sizeof(cc->chars[0]); i++) {
		cc->chars[i] = ~cc->chars[i];
	}
}

void
cc_mask(struct re_char_class *cc, struct re_char_class *mask)
{
	unsigned i;
	for (i = 0; i < sizeof(cc->chars)/sizeof(cc->chars[0]); i++) {
		cc->chars[i] &= ~mask->chars[i];
	}
}

void
cc_dump(FILE *f, struct re_char_class *cc)
{
	unsigned i;
	for (i = 0; i < 256; i++) {
		unsigned pos;
		unsigned char bit;
		bitset_pos((unsigned char)i, &pos, &bit);
		if (cc->chars[pos] & bit) {
			if (isprint(i)) {
				fprintf(f, "%c", i);
			} else {
				fprintf(f, "\\x%02x", i);
			}
		}
	}
}
