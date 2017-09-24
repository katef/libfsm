/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FUZZ_RE_H
#define FUZZ_RE_H

bool test_re_pcre_minimize(theft_seed seed);

bool test_re_parser_literal(uint8_t verbosity,
	const uint8_t *re, size_t re_size,
	size_t count, const struct string_pair *pairs);

bool test_re_parser_pcre(uint8_t verbosity,
	const uint8_t *re_string, size_t re_size,
	size_t pos_count, const struct string_pair *pos_pairs,
	size_t neg_count, const struct string_pair *neg_pairs);

#endif

