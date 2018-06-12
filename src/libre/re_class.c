#include "re_class.h"
#include "re_ast.h"

#include <ctype.h>

struct ast_char_class *
ast_char_class_new(void)
{
	struct ast_char_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	fprintf(stderr, "NEW: ");
	ast_char_class_dump(stderr, res);
	fprintf(stderr, "\n");
	return res;
}

void
ast_char_class_free(struct ast_char_class *c)
{
	free(c);
}

static void
bitset_pos(unsigned char byte, unsigned *pos, unsigned char *bit)
{
	*pos = byte / 8;
	*bit = 1U << (byte & 0x07);
}

void
ast_char_class_add_byte(struct ast_char_class *cc, unsigned char byte)
{
	unsigned pos;
	unsigned char bit;
	assert(cc != NULL);
	fprintf(stderr, "ADDING 0x%02x\n", byte);
	bitset_pos(byte, &pos, &bit);
	cc->chars[pos] |= bit;

	ast_char_class_dump(stderr, cc);
	fprintf(stderr, "\n");
}

void
ast_char_class_add_range(struct ast_char_class *cc, unsigned char from, unsigned char to)
{
	unsigned char i;
	assert(cc != NULL);
	assert(from <= to);
	for (i = from; i <= to; i++) {
		ast_char_class_add_byte(cc, i);		
	}
	
}

void
ast_char_class_invert(struct ast_char_class *cc)
{
	unsigned i;
	for (i = 0; i < sizeof(cc->chars)/sizeof(cc->chars[0]); i++) {
		cc->chars[i] = ~cc->chars[i];
	}
}

/* void
 * ast_char_class_add_named_class(struct ast_char_class *c, )
 * {
 * 
 * } */

void
ast_char_class_mask(struct ast_char_class *cc, const struct ast_char_class *mask)
{
	unsigned i;
	for (i = 0; i < sizeof(cc->chars)/sizeof(cc->chars[0]); i++) {
		cc->chars[i] &= mask->chars[i];
	}
}

void
ast_char_class_dump(FILE *f, struct ast_char_class *cc)
{
	unsigned i;
	for (i = 0; i < 256; i++) {
		unsigned pos;
		unsigned char bit;
		bitset_pos((unsigned char)i, &pos, &bit);
		if (cc->chars[pos] & bit) {
		/* if (c->chars[i/(8*sizeof(c->chars[0]))] & (1U << (i & 0x07))) { */
			if (isprint(i)) {
				fprintf(f, "%c", i);
			} else {
				fprintf(f, "\\x%02x", i);
			}
		}
	}
}
