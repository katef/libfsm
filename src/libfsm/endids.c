/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "endids_internal.h"

int
fsm_setendid(struct fsm *fsm, fsm_end_id_t id)
{
	fsm_state_t i;

	/* for every end state */
	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, i)) {
			if (!fsm_endid_set(fsm, i, id)) {
				return 0;
			}
		}
	}

	return 1;
}

int
fsm_getendids(const struct fsm *fsm, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written)
{
	return fsm_endid_get(fsm, end_state,
	    id_buf_count, id_buf, ids_written);
}

size_t
fsm_getendidcount(const struct fsm *fsm, fsm_state_t end_state)
{
	return fsm_endid_count(fsm, end_state);
}

int
fsm_endid_init(struct fsm *fsm)
{
	struct fsm_endid_info *res = f_calloc(fsm->opt->alloc, 1, sizeof(*res));
	assert(res != NULL);

	res->alloc = fsm->opt->alloc;
	fsm->endid_info = res;
	return 1;
}

void
fsm_endid_free(struct fsm *fsm)
{
	if (fsm == NULL || fsm->endid_info == NULL) {
		return;
	}

	f_free(fsm->opt->alloc, fsm->endid_info);
}

int
fsm_endid_set(struct fsm *fsm,
    fsm_state_t state, fsm_end_id_t id)
{
	size_t i;
	struct endid_info *info = NULL;
	struct fsm_endid_info *ei = NULL;
	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);
	assert(ei->count < MAX_END_STATES);
	assert(state < fsm->statecount);
	assert(fsm_isend(fsm, state));

	for (i = 0; i < ei->count; i++) {
		info = &ei->states[i];
		if (info->state == state) {
			assert(info->count < MAX_END_IDS);
			info->ids[info->count] = id;
			info->count++;
			return 1;
		}
	}

	info = &ei->states[ei->count];
	info->state = state;
	info->ids[0] = id;
	info->count++;
	ei->count++;

	return 1;
}

size_t
fsm_endid_count(const struct fsm *fsm,
    fsm_state_t state)
{
	const struct fsm_endid_info *ei = NULL;
	size_t i;
	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

	for (i = 0; i < ei->count; i++) {
		if (ei->states[i].state == state) {
			return ei->states[i].count;
		}
	}

	return 0;
}

int
fsm_endid_get(const struct fsm *fsm, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written)
{
	size_t i, j;
	const struct endid_info *info = NULL;
	const struct fsm_endid_info *ei = NULL;
	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

	assert(id_buf != NULL);
	assert(ids_written != NULL);

	assert(ei->count < MAX_END_STATES);

	for (i = 0; i < ei->count; i++) {
		info = &ei->states[i];
		if (info->state == end_state) {
			if (id_buf_count < info->count) {
				return 0;
			}
			for (j = 0; j < info->count; j++) {
				id_buf[j] = info->ids[j];
			}
			*ids_written = j;
			return 1;
		}
	}

	*ids_written = 0;
	return 1;
}

int
fsm_endid_carry(const struct fsm *src_fsm, const struct state_set *src_set,
    struct fsm *dst_fsm, fsm_state_t dst_state)
{
	const int log_carry = 0;
	/* FIXME */
	struct state_set *src_set_notconst = (struct state_set *)src_set;

#define ID_BUF_SIZE 10
	fsm_end_id_t id_buf[ID_BUF_SIZE];
	struct state_iter it;
	fsm_state_t s;
	size_t j;

	if (log_carry) {
		fprintf(stderr, "==== fsm_endid_carry: before\n");
		fsm_endid_dump(stderr, src_fsm);
	}

	for (state_set_reset(src_set_notconst, &it); state_set_next(&it, &s); ) {
		size_t written;
		if (!fsm_isend(src_fsm, s)) {
			continue;
		}
		if (!fsm_endid_get(src_fsm, s,
			ID_BUF_SIZE, id_buf, &written)) {
			assert(!"todo dynamic size");
		}

		for (j = 0; j < written; j++) {
			if (!fsm_endid_set(dst_fsm,
				dst_state, id_buf[j])) {
				return 0;
			}
		}
	}

	if (log_carry) {
		fprintf(stderr, "==== fsm_endid_carry: after\n");
		fsm_endid_dump(stderr, dst_fsm);
	}

	return 1;
}

void
fsm_endid_iter(const struct fsm *fsm,
    fsm_endid_iter_cb *cb, void *opaque)
{
	size_t i;
	const struct endid_info *info = NULL;
	const struct fsm_endid_info *ei = NULL;

	assert(fsm != NULL);
	assert(cb != NULL);

	ei = fsm->endid_info;
	if (ei == NULL) {
		return;
	}

	for (i = 0; i < ei->count; i++) {
		info = &ei->states[i];
		if (!cb(info->state, info->count, info->ids, opaque)) {
			break;
		}
	}
}

struct dump_env {
	FILE *f;
};

static int
dump_cb(fsm_state_t state,
    size_t count, const fsm_end_id_t *ids,
    void *opaque)
{
	struct dump_env *env = opaque;
	size_t i;
	fprintf(env->f, "state[%u]:", state);

	for (i = 0; i < count; i++) {
		fprintf(env->f, " %u", ids[i]);
	}
	fprintf(env->f, "\n");
	return 1;
}

void
fsm_endid_dump(FILE *f, const struct fsm *fsm)
{
	struct dump_env env;
	env.f = f;

	fsm_endid_iter(fsm, dump_cb, &env);
}
