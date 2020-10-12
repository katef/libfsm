#include "type_info_capture_string_set.h"

#include <adt/xalloc.h>

#define IGNORE_ZERO_LENGTH_CAPTURES 1

static void
gen_string(struct theft *t,
    uint8_t letter_count, uint8_t string_maxlen,
    size_t *length, char *buf);

static enum theft_alloc_res
cs_alloc(struct theft *t, void *unused, void **output)
{
	struct capture_string_set *res;
	struct capture_env *env = theft_hook_get_env(t);
	assert(env && env->tag == 'c');
	(void)unused;

	res = calloc(1, sizeof(*res));
	if (res == NULL) { return THEFT_ALLOC_ERROR; }

	const uint8_t cs_ceil = 1 + theft_random_choice(t, MAX_CAPTURE_STRINGS - 1);
	for (size_t cs_i = 0; cs_i < cs_ceil; cs_i++) {
		struct capstring *cs = &res->capture_strings[res->count];
		memset(cs, 0x00, sizeof(*cs));

		size_t length;
		gen_string(t, env->letter_count, env->string_maxlen,
		    &length, cs->string);

		const uint8_t capture_count_ceil = theft_random_choice(t, MAX_CAPTURES);
		for (size_t c_i = 0; c_i < capture_count_ceil; c_i++) {
			struct capture *c = &cs->captures[cs->capture_count];
			c->offset = theft_random_choice(t, length);
			c->length = theft_random_choice(t, length - c->offset);
			if (theft_random_bits(t, 4) > 0
#if IGNORE_ZERO_LENGTH_CAPTURES
			    && c->length > 0
#endif
				) {
				cs->capture_count++; /* keep */
			}
		}

		if (theft_random_bits(t, 3) > 0) {
			res->count++; /* keep */
		}
	}

	*output = res;
	return THEFT_ALLOC_OK;
}

static void
gen_string(struct theft *t,
    uint8_t letter_count, uint8_t string_maxlen,
    size_t *length, char *buf)
{
	/* Generate a string 'a' .. (letter_count - 1), with
	 * optional, nonconsecutive '*'s after letters. */
	size_t i = 0;
	size_t stars = 0;

	for (i = 0; i < string_maxlen; i++) {
		/* While this ordering is a bit confusing
		 * ('\0', 'a' .. letter_count, '*'), this ensures
		 * that the simplest option is first and thus theft
		 * is able to shrink the string. */
		char c = theft_random_choice(t, letter_count + 2);
		if (c == 0) {
			break;	/* '\0' */
		} else if (c == letter_count + 1) {
			/* disallow multiple consecutive '*'s */
			if (i == 0 || buf[i - 1] == '*') {
				c = 'a';
			} else {
				c = '*';
				stars++;
			}
		} else {
			c += 'a' - 1;
		}
		buf[i] = c;
	}
	assert(stars <= i);
	*length = i - stars;
}

static void
cs_print(FILE *f, const void *instance, void *env)
{
	const struct capture_string_set *css = instance;
	(void)env;

	fprintf(f, "capture_string_set[%u]:\n", css->count);
	for (size_t cs_i = 0; cs_i < css->count; cs_i++) {
		const struct capstring *cs = &css->capture_strings[cs_i];
		fprintf(f, " -- %zu: '%s'", cs_i, cs->string);
		for (size_t c_i = 0; c_i < cs->capture_count;  c_i++) {
			const uint8_t offset = cs->captures[c_i].offset;
			const uint8_t length = cs->captures[c_i].length;
			fprintf(f, " (%u,%u)", offset, offset + length);
		}
		fprintf(f, "\n");
	}
}

const struct theft_type_info type_info_capture_string_set = {
	.alloc = cs_alloc,
	.free  = theft_generic_free_cb,
	.print = cs_print,
	.autoshrink_config = {
		.enable = true,
	}
};
