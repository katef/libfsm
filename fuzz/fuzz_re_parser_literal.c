/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "fuzz_libfsm.h"

#include "type_info_re.h"

bool
test_re_parser_literal(uint8_t verbosity,
	const uint8_t *re, size_t re_size,
	size_t count, const struct string_pair *pairs)
{
	struct re_err err;
	struct fsm *fsm;
	size_t i;

	const struct fsm_options opt = {
		.anonymous_states  = 1,
		.consolidate_edges = 1
	};

	{
		struct scanner s = {
			.tag   = 'S',
			.magic = &s.magic,
			.str   = re,
			.size  = re_size
		};

		fsm = re_comp(RE_LITERAL, scanner_next, &s, &opt, RE_MULTI, &err);

		assert(s.str == re);
		assert(s.magic == &s.magic);
	}

	/* Invalid RE: did we flag an error? */
	if (fsm == NULL) {
		if (err.e == RE_ESUCCESS) {
			LOG_FAIL(verbosity, "Expected error, got RE_ESUCCESS\n");
			return false;
		}

		return true;
	}

	if (!fsm_determinise(fsm)) {
		LOG_FAIL(verbosity, "DETERMINISE\n");
		goto fail;
	}

	/* Valid RE: check if expected strings are matched */
	for (i = 0; i < count; i++) {
		struct fsm_state *st;

		struct scanner s = {
			.tag   = 'S',
			.magic = &s.magic,
			.str   = pairs[i].string,
			.size  = pairs[i].size
		};

		st = fsm_exec(fsm, scanner_next, &s);

		assert(s.str == pairs[i].string);
		assert(s.magic == &s.magic);

		if (verbosity > 0) {
			fsm_print(fsm, stderr, FSM_OUT_DOT);
		}
		if (st == NULL) {
			LOG_FAIL(verbosity, "EXEC FAIL\n");
			goto fail;
		} else if (!fsm_isend(fsm, st)) {
			LOG_FAIL(verbosity, "NOT END STATE\n");
			goto fail;
		}
	}

	fsm_free(fsm);

	return true;

fail:

	fsm_free(fsm);

	return false;
}

