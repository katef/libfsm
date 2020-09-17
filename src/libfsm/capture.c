/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "capture_internal.h"

int
fsm_capture_init(struct fsm *fsm)
{
	(void)fsm;
	return 0;
}

void
fsm_capture_free(struct fsm *fsm)
{
	(void)fsm;
}

unsigned
fsm_countcaptures(const struct fsm *fsm)
{
	(void)fsm;
	return 0;
}

int
fsm_capture_has_capture_actions(const struct fsm *fsm, fsm_state_t state)
{
	(void)fsm;
	(void)state;
	return 0;
}

int
fsm_capture_set_path(struct fsm *fsm, unsigned capture_id,
    fsm_state_t start, fsm_state_t end)
{
	(void)fsm;
	(void)capture_id;
	(void)start;
	(void)end;
	return 0;
}

void
fsm_capture_rebase_capture_id(struct fsm *fsm, unsigned base)
{
	(void)fsm;
	(void)base;
}

struct fsm_capture *
fsm_capture_alloc(const struct fsm *fsm)
{
	(void)fsm;
	return NULL;
}

void
fsm_capture_update_captures(const struct fsm *fsm,
    fsm_state_t cur_state, fsm_state_t next_state, size_t offset,
    struct fsm_capture *captures)
{
	(void)fsm;
	(void)cur_state;
	(void)next_state;
	(void)offset;
	(void)captures;
}

void
fsm_capture_finalize_captures(const struct fsm *fsm,
    size_t capture_count, struct fsm_capture *captures);

#ifndef NDEBUG
#include <stdio.h>

/* Dump capture metadata about an FSM. */
void
fsm_capture_dump(FILE *f, const char *tag, const struct fsm *fsm)
{
	(void)f;
	(void)tag;
	(void)fsm;
}
#endif
