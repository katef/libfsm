/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "endids_internal.h"

struct fsm_endid_info *
fsm_endid_alloc(struct fsm_alloc *alloc)
{
	(void)alloc;
	return NULL;
}

void
fsm_endid_free(struct fsm_endid_info *ei)
{
	(void)ei;
}

int
fsm_endid_set(struct fsm_endid_info *ei,
    fsm_state_t state, fsm_end_id_t id)
{
	(void)ei;
	(void)state;
	(void)id;
	return 0;
}

size_t
fsm_endid_count(const struct fsm_endid_info *ei,
    fsm_state_t state)
{
	(void)ei;
	(void)state;
	return 0;
}

int
fsm_endid_get(const struct fsm_endid_info *ei, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written)
{
	(void)ei;
	(void)end_state;
	(void)id_buf;
	(void)id_buf_count;
	(void)ids_written;
	return 0;
}
