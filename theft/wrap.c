/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include <fsm/fsm.h>

#include <re/re.h>

#include <adt/xalloc.h>

struct scanner {
	char tag;
	void *magic;
	const uint8_t *str;
	size_t size;
	size_t offset;
};

static int
scanner_next(void *opaque)
{
	struct scanner *s;
	unsigned char c;

	s = opaque;
	assert(s->tag == 'S');

	if (s->offset == s->size) {
		return EOF;
	}

	c = s->str[s->offset];
	s->offset++;

	return (int) c;
}

int
wrap_fsm_exec(struct fsm *fsm, const struct string_pair *pair, fsm_state_t *state)
{
	int e;

	struct scanner s = {
		.tag    = 'S',
		.magic  = &s.magic,
		.str    = pair->string,
		.size   = pair->size,
		.offset = 0
	};

	e = fsm_exec(fsm, scanner_next, &s, state);

	assert(s.str == pair->string);
	assert(s.magic == &s.magic);

	return e;
}

struct fsm *
wrap_re_comp(enum re_dialect dialect, const struct string_pair *pair,
	const struct fsm_options *opt,
	struct re_err *err)
{
	struct fsm *fsm;

	struct scanner s = {
		.tag    = 'S',
		.magic  = &s.magic,
		.str    = pair->string,
		.size   = pair->size,
		.offset = 0
	};

	assert(opt != NULL);
	assert(err != NULL);

	fsm = re_comp_new(dialect, scanner_next, &s, opt, RE_MULTI, err);

	assert(s.str == pair->string);
	assert(s.magic == &s.magic);

	return fsm;
}

