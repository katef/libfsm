/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_re.h"

#include <string.h>

#include <adt/xalloc.h>

enum theft_alloc_res
type_info_re_literal_build_info(struct theft *t,
	struct test_re_info *info)
{
	size_t size;
	uint64_t *buf64;
	uint8_t *buf;

	size = theft_random_bits(t, 8);
	if (size == 0) {
		size = 1;
	}

	/* a 64-bit-aligned buffer */
	buf64 = xmalloc(size + 1 + sizeof *buf64);

	buf = (uint8_t *) buf64;
	if (buf == NULL) {
		return THEFT_ALLOC_ERROR;
	}

	theft_random_bits_bulk(t, 8 * size, buf64);

	buf[size] = '\0';

	info->size      = size;
	info->string    = buf;
	info->pos_count = 1;
	info->pos_pairs = xcalloc(1, sizeof (struct string_pair));
	info->pos_pairs[0].size   = size;
	info->pos_pairs[0].string = buf;

	return THEFT_ALLOC_OK;
}

