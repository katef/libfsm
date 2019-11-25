/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "theft_libfsm.h"

#include "fuzz_re.h"

#include "type_info_re.h"

bool
test_re_parser_literal(uint8_t verbosity,
	const uint8_t *re, size_t re_size,
	size_t count, const struct string_pair *pairs)
{
	struct re_err err;
	struct fsm *fsm;
	size_t i;

	struct string_pair pair = {
		.string = (uint8_t *) re,
		.size   = re_size
	};

	const struct fsm_options opt = {
		.anonymous_states  = 1,
		.consolidate_edges = 1
	};

	fsm = wrap_re_comp(RE_LITERAL, &pair, &opt, &err);

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
		fsm_state_t st;
		int e;

		e = wrap_fsm_exec(fsm, &pairs[i], &st);

		if (verbosity > 0) {
			fsm_print_dot(stderr, fsm);
		}
		if (e != 1) {
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

