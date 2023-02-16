/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "gtest.h"

#include <assert.h>
#include <string.h>

#include <fsm/options.h>
#include <fsm/bool.h>

/* Note: not "gentest" because then this matches the gen* pattern and
 * the Makefile tries to build this as a standalone test runner. */

/* Should never need this many for any of the tests. */
#define MAX_STEPS 10000

void
gtest_init_matches(struct exp_matches *ms)
{
	for (size_t m_i = 0; m_i < ms->count; m_i++) {
		struct exp_match *m = &ms->matches[m_i];
		m->length = strlen(m->s);
	}
}

enum fsm_generate_matches_cb_res
gtest_matches_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	(void)depth;
	(void)match_count;
	(void)steps;

	if (steps == MAX_STEPS) {
		return FSM_GENERATE_MATCHES_CB_RES_HALT;
	}

	if (getenv("VERBOSE")) {
		fprintf(stderr,
		    "gtest_matches_cb: depth %zu, match_count %zu, steps %zu, input \"%s\"(%zu), end_state %d\n",
		    depth, match_count, steps, input, input_length, end_state);
	}

	struct exp_matches *ms = opaque;
	assert(ms != NULL);
	assert(ms->magic == EXP_MATCHES_MAGIC);

	for (size_t m_i = 0; m_i < ms->count; m_i++) {
		struct exp_match *m = &ms->matches[m_i];
		if (m->length == input_length
		    && 0 == strncmp(m->s, input, input_length)) {
			m->found = true;

#define ID_BUF_COUNT 1
			fsm_end_id_t id_buf[ID_BUF_COUNT];
			size_t written;
			enum fsm_getendids_res gres = fsm_getendids(fsm,
			    end_state, ID_BUF_COUNT, id_buf, &written);

			if (gres != FSM_GETENDIDS_FOUND) {
				fprintf(stderr,
				    "ERROR: fsm_getendids: returned %d\n",
				    gres);
				return FSM_GENERATE_MATCHES_CB_RES_HALT;
			}

			if (written != 1 || id_buf[0] != m_i) {
				fprintf(stderr, "ERROR: endid mismatch, expected %zu, got %u\n",
				    m_i, id_buf[0]);
				return FSM_GENERATE_MATCHES_CB_RES_HALT;
			}

			return m->prune
			    ? FSM_GENERATE_MATCHES_CB_RES_PRUNE
			    : FSM_GENERATE_MATCHES_CB_RES_CONTINUE;
		}
	}

	fprintf(stderr, "ERROR: unexpected input: \"%s\"(%zu)\n",
	    input, input_length);

	return FSM_GENERATE_MATCHES_CB_RES_HALT;
}

static const struct fsm_options options; /* use defaults */

struct fsm *
gtest_fsm_of_matches(const struct exp_matches *ms)
{
	struct fsm *res = NULL;
	for (size_t m_i = 0; m_i < ms->count; m_i++) {
		const struct exp_match *m = &ms->matches[m_i];

		struct fsm *fsm = fsm_new(&options);
		assert(fsm != NULL);

		if (!fsm_addstate_bulk(fsm, m->length + 1)) {
			return NULL;
		}

		fsm_setstart(fsm, 0);

		for (size_t i = 0; i < m->length; i++) {
			if (!fsm_addedge_literal(fsm,
				i, i + 1, m->s[i])) {
				return NULL;
			}
		}

		fsm_setend(fsm, m->length, 1);
		fsm_setendid(fsm, m_i);

		if (res == NULL) {
			res = fsm;
		} else {
			res = fsm_union(res, fsm, NULL);
			assert(res != NULL);
		}
	}

	if (!fsm_determinise(res)) {
		return NULL;
	}

	return res;
}

bool
gtest_all_found(const struct exp_matches *ms, FILE *f)
{
	bool res = true;
	for (size_t i = 0; i < ms->count; i++) {
		const struct exp_match *m = &ms->matches[i];
		if (!m->found) {
			res = false;
			if (f != NULL) {
				fprintf(f, "NOT FOUND: \"%s\"(%zu)\n",
				    m->s, m->length);
			}
		}
	}
	return res;
}
