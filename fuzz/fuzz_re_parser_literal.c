/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "fuzz_libfsm.h"

#include "type_info_re.h"

bool test_re_parser_literal(uint8_t verbosity,
    const uint8_t *re, size_t re_size,
    size_t count, const struct string_pair *pairs)
{
	enum re_flags flags = RE_MULTI;
	struct re_err err = { .e = 0 };
	const struct fsm_options fsm_config = {
		.anonymous_states = 1,
		.consolidate_edges = 1,
	};

	struct scanner s = { .tag = 'S', .str = re, .size = re_size, };
	struct fsm *fsm = re_comp(RE_LITERAL, scanner_next, &s,
	    &fsm_config, flags, &err);

	/* Invalid RE: did we flag an error? */
	if (fsm == NULL) {
		if (err.e == RE_ESUCCESS) {
			LOG_FAIL(verbosity, "Expected error, got RE_ESUCCESS\n");
			return false;
		} else {
			return true;
		}
	}

	if (!fsm_determinise(fsm)) {
		LOG_FAIL(verbosity, "DETERMINISE\n");
		goto fail;
	}

	/* Valid RE: check if expected strings are matched */
	for (size_t i = 0; i < count; i++) {
		s = (struct scanner) {
			.tag = 'S',
			.str = pairs[i].string,
			.size = pairs[i].size,
		};
		struct fsm_state *st = fsm_exec(fsm, scanner_next, &s);
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

