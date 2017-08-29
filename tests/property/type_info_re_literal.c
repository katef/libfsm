#include "type_info_re.h"

#include <string.h>

bool type_info_re_literal_build_info(struct theft *t, 
    struct test_re_info *info)
{
	size_t size = theft_random_bits(t, 8);
	if (size == 0) { size = 1; }

	/* Get a 64-bit-aligned buffer */
	uint64_t *buf64 = malloc(size + 1 + sizeof(uint64_t));
	uint8_t *buf = (uint8_t *)buf64;
	if (buf == NULL) { return false; }
	theft_random_bits_bulk(t, 8*size, buf64);

	buf[size] = '\0';
	info->size = size;
	info->string = buf;
	info->pos_count = 1;
	info->pos_pairs = calloc(1, sizeof(struct string_pair));
	if (info->pos_pairs == NULL) {
		free(buf);
		return false;
	}

	info->pos_pairs[0].size = size;
	info->pos_pairs[0].string = buf;
	return true;
}
