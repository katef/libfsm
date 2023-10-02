/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <fsm/alloc.h>
#include <fsm/capture.h>
#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/hash.h>
#include <adt/idmap.h>

#include <string.h>
#include <assert.h>
#include <errno.h>

#include "internal.h"
#include "capture.h"
#include "capture_vm_program.h"
#include "capture_log.h"
#include "capture_vm.h"
#include "endids.h"

#define DEF_PROGRAMS_CEIL 4

struct fsm_capture_info {
	unsigned max_capture_id;

	/* For particular end states, which captures are active? */
	struct idmap *end_capture_map;

	/* Set of capture resolution programs associated with specific
	 * end states. */
	struct capvm_program_set {
		uint32_t ceil;
		uint32_t used;
		struct capvm_program **set;
	} programs;

	/* For particular end states, which capture programs are
	 * associtaed with them? */
	struct idmap *end_capvm_program_map;
};

int
fsm_capture_init(struct fsm *fsm)
{
	struct fsm_capture_info *ci = NULL;
	struct idmap *end_capture_map = NULL;
	struct idmap *end_capvm_program_map = NULL;

	ci = f_calloc(fsm->opt->alloc,
	    1, sizeof(*ci));
	if (ci == NULL) {
		goto cleanup;
	}
	end_capture_map = idmap_new(fsm->opt->alloc);
	if (end_capture_map == NULL) {
		goto cleanup;
	}
	ci->end_capture_map = end_capture_map;

	end_capvm_program_map = idmap_new(fsm->opt->alloc);
	if (end_capvm_program_map == NULL) {
		goto cleanup;
	}
	ci->end_capvm_program_map = end_capvm_program_map;

	fsm->capture_info = ci;

	return 1;

cleanup:
	f_free(fsm->opt->alloc, ci);
	idmap_free(end_capture_map);
	idmap_free(end_capvm_program_map);
	return 0;
}

void
fsm_capture_free(struct fsm *fsm)
{
	struct fsm_capture_info *ci = fsm->capture_info;
	if (ci == NULL) {
		return;
	}

	idmap_free(ci->end_capture_map);
	idmap_free(ci->end_capvm_program_map);

	for (size_t p_i = 0; p_i < ci->programs.used; p_i++) {
		fsm_capvm_program_free(fsm->opt->alloc, ci->programs.set[p_i]);
	}
	f_free(fsm->opt->alloc, ci->programs.set);

	f_free(fsm->opt->alloc, ci);
	fsm->capture_info = NULL;
}

unsigned
fsm_capture_ceiling(const struct fsm *fsm)
{
	if (fsm->capture_info == NULL) {
		return 0;
	}

#if EXPENSIVE_CHECKS
	/* check actual */
	unsigned res = 0;
	for (size_t i = 0; i < fsm->capture_info->programs.used; i++) {
		const unsigned id = fsm_capvm_program_get_max_capture_id(fsm->capture_info->programs.set[i]);
		if (id > res) {
			res = id;
		}
	}
	assert(res == fsm->capture_info->max_capture_id);
#endif

	return fsm->capture_info->max_capture_id + 1;
}

struct fsm_capture *
fsm_capture_alloc_capture_buffer(const struct fsm *fsm)
{
	assert(fsm != NULL);
	const size_t len = fsm_capture_ceiling(fsm);
	struct fsm_capture *res = f_malloc(fsm->opt->alloc,
	    len * sizeof(res[0]));
	return res;
}

void
fsm_capture_free_capture_buffer(const struct fsm *fsm,
    struct fsm_capture *capture_buffer)
{
	assert(fsm != NULL);
	f_free(fsm->opt->alloc, capture_buffer);
}


int
fsm_capture_has_captures(const struct fsm *fsm)
{
	return fsm->capture_info
	    ? fsm->capture_info->programs.used > 0
	    : 0;
}

void
fsm_capture_dump_programs(FILE *f, const struct fsm *fsm)
{
	fprintf(f, "\n==== %s:\n", __func__);
	struct fsm_capture_info *ci = fsm->capture_info;
	for (uint32_t i = 0; i < ci->programs.used; i++) {
		const struct capvm_program *p = ci->programs.set[i];
		fprintf(f, "# program %u, capture_count %u, base %u\n",
		    i, p->capture_count, p->capture_base);
		fsm_capvm_program_dump(f, p);
		fprintf(f, "\n");
	}
}

int
fsm_capture_set_active_for_end(struct fsm *fsm,
    unsigned capture_id, fsm_state_t end_state)
{
	struct fsm_capture_info *ci = fsm->capture_info;
	assert(ci != NULL);
	struct idmap *m = ci->end_capture_map;
	assert(m != NULL);

	#if EXPENSIVE_CHECKS
	assert(fsm_isend(fsm, end_state));
	#endif

	return idmap_set(m, end_state, capture_id);
}

void
fsm_capture_iter_active_for_end_state(const struct fsm *fsm, fsm_state_t state,
    fsm_capture_iter_active_for_end_cb *cb, void *opaque)
{
	/* These types should be the same. */
	idmap_iter_fun *idmap_cb = cb;
	idmap_iter_for_state(fsm->capture_info->end_capture_map, state,
	    idmap_cb, opaque);
}

void
fsm_capture_iter_active_for_all_end_states(const struct fsm *fsm,
    fsm_capture_iter_active_for_end_cb *cb, void *opaque)
{
	/* These types should be the same. */
	idmap_iter_fun *idmap_cb = cb;
	idmap_iter(fsm->capture_info->end_capture_map,
	    idmap_cb, opaque);
}

void
fsm_capture_iter_program_ids_for_end_state(const struct fsm *fsm, fsm_state_t state,
    fsm_capture_iter_program_ids_for_end_state_cb *cb, void *opaque)
{
	/* These types should be the same. */
	idmap_iter_fun *idmap_cb = cb;
	idmap_iter_for_state(fsm->capture_info->end_capvm_program_map, state,
	    idmap_cb, opaque);
}

void
fsm_capture_iter_program_ids_for_all_end_states(const struct fsm *fsm,
    fsm_capture_iter_program_ids_for_end_state_cb *cb, void *opaque)
{
	/* These types should be the same. */
	idmap_iter_fun *idmap_cb = cb;
	idmap_iter(fsm->capture_info->end_capvm_program_map,
	    idmap_cb, opaque);
}

static int
dump_active_for_ends_cb(fsm_state_t state_id, unsigned value, void *opaque)
{
	FILE *f = opaque;
	fprintf(f, " -- state %d: value %u\n", state_id, value);
	return 1;
}

void
fsm_capture_dump_active_for_ends(FILE *f, const struct fsm *fsm)
{
	fprintf(f, "%s:\n", __func__);
	idmap_iter(fsm->capture_info->end_capture_map, dump_active_for_ends_cb, f);
}

void
fsm_capture_dump_program_end_mapping(FILE *f, const struct fsm *fsm)
{
	fprintf(f, "%s:\n", __func__);
	idmap_iter(fsm->capture_info->end_capvm_program_map, dump_active_for_ends_cb, f);
}

/* Dump capture metadata about an FSM. */
void
fsm_capture_dump(FILE *f, const char *tag, const struct fsm *fsm)
{
	struct fsm_capture_info *ci;

	assert(fsm != NULL);
	ci = fsm->capture_info;
	if (ci == NULL) {
		fprintf(f, "==== %s -- no captures\n", tag);
		return;
	}

	fsm_endid_dump(f, fsm);
	fsm_capture_dump_active_for_ends(f, fsm);
	fsm_capture_dump_programs(f, fsm);
	fsm_capture_dump_program_end_mapping(f, fsm);
}

struct carry_active_captures_env {
	fsm_state_t dst;
	struct idmap *dst_m;
	int ok;
};

static int
copy_active_captures_cb(fsm_state_t state_id, unsigned value, void *opaque)
{
	(void)state_id;

	struct carry_active_captures_env *env = opaque;
	if (!idmap_set(env->dst_m, env->dst, value)) {
		env->ok = false;
		return 0;
	}
	return 1;
}

static int
copy_program_associations_cb(fsm_state_t state_id, unsigned value, void *opaque)
{
	(void)state_id;

	struct carry_active_captures_env *env = opaque;
	if (!idmap_set(env->dst_m, env->dst, value)) {
		env->ok = false;
		return 0;
	}
	return 1;
}

int
fsm_capture_copy_active_for_ends(const struct fsm *src_fsm,
	const struct state_set *states,
	struct fsm *dst_fsm, fsm_state_t dst_state)
{
	struct state_iter it;
	fsm_state_t s;

	assert(src_fsm != NULL);
	assert(src_fsm->capture_info != NULL);
	assert(src_fsm->capture_info->end_capture_map != NULL);
	assert(dst_fsm != NULL);
	assert(dst_fsm->capture_info != NULL);
	assert(dst_fsm->capture_info->end_capture_map != NULL);
	struct idmap *src_m = src_fsm->capture_info->end_capture_map;
	struct idmap *dst_m = dst_fsm->capture_info->end_capture_map;

	struct carry_active_captures_env env = {
		.dst_m = dst_m,
		.dst = dst_state,
		.ok = true,
	};

	state_set_reset(states, &it);
	while (state_set_next(&it, &s)) {
		if (!fsm_isend(src_fsm, s)) {
			continue;
		}

		idmap_iter_for_state(src_m, s, copy_active_captures_cb, &env);
		if (!env.ok) {
			goto cleanup;
		}
	}

cleanup:
	return env.ok;
}

int
fsm_capture_copy_program_end_state_associations(const struct fsm *src_fsm,
	const struct state_set *states,
	struct fsm *dst_fsm, fsm_state_t dst_state)
{
	struct state_iter it;
	fsm_state_t s;

	assert(src_fsm != NULL);
	assert(src_fsm->capture_info != NULL);
	assert(src_fsm->capture_info->end_capvm_program_map != NULL);
	assert(dst_fsm != NULL);
	assert(dst_fsm->capture_info != NULL);
	assert(dst_fsm->capture_info->end_capvm_program_map != NULL);
	struct idmap *src_m = src_fsm->capture_info->end_capvm_program_map;
	struct idmap *dst_m = dst_fsm->capture_info->end_capvm_program_map;

	struct carry_active_captures_env env = {
		.dst_m = dst_m,
		.dst = dst_state,
		.ok = true,
	};

	state_set_reset(states, &it);
	while (state_set_next(&it, &s)) {
		if (!fsm_isend(src_fsm, s)) {
			continue;
		}

		LOG(5 - LOG_CAPTURE_COMBINING_ANALYSIS,
		    "%s: dst_state %d, state_set_next => %d\n",
		    __func__, dst_state, s);

		idmap_iter_for_state(src_m, s, copy_program_associations_cb, &env);
		if (!env.ok) {
			goto cleanup;
		}
	}

cleanup:
	return env.ok;
}

int
fsm_capture_copy_programs(const struct fsm *src_fsm,
	struct fsm *dst_fsm)
{
	const struct fsm_alloc *alloc = src_fsm->opt->alloc;
	assert(alloc == dst_fsm->opt->alloc);
	const struct fsm_capture_info *src_ci = src_fsm->capture_info;

	for (uint32_t p_i = 0; p_i < src_ci->programs.used; p_i++) {
		const struct capvm_program *p = src_ci->programs.set[p_i];
		struct capvm_program *cp = capvm_program_copy(alloc, p);
		if (cp == NULL) {
			return 0;
		}

		/* unused: because this is an in-order copy, it's assumed
		 * the programs will retain their order. */
		uint32_t prog_id;
		if (!fsm_capture_add_program(dst_fsm, cp, &prog_id)) {
			return 0;
		}
	}
	return 1;
}

size_t
fsm_capture_program_count(const struct fsm *fsm)
{
	return fsm->capture_info->programs.used;
}

struct check_program_mappings_env {
	const struct fsm *fsm;
};

static int
check_program_mappings_cb(fsm_state_t state_id, unsigned value, void *opaque)
{
	const uint32_t prog_id = (uint32_t)value;
	struct check_program_mappings_env *env = opaque;
	assert(state_id < env->fsm->statecount);
	assert(prog_id < env->fsm->capture_info->programs.used);
	return 1;
}

void
fsm_capture_integrity_check(const struct fsm *fsm)
{
	if (!EXPENSIVE_CHECKS) { return; }

	/* check that all program mappings are in range */
	struct check_program_mappings_env env = {
		.fsm = fsm,
	};
	idmap_iter(fsm->capture_info->end_capvm_program_map, check_program_mappings_cb, &env);
}

struct capture_idmap_compact_env {
	int ok;
	struct idmap *dst;
	const fsm_state_t *mapping;
	size_t orig_statecount;
};

static int
copy_with_mapping_cb(fsm_state_t state_id, unsigned value, void *opaque)
{
	fsm_state_t dst_id;
	struct capture_idmap_compact_env *env = opaque;

	assert(state_id < env->orig_statecount);
	dst_id = env->mapping[state_id];

	if (dst_id == FSM_STATE_REMAP_NO_STATE) {
		return 1;		/* discard */
	}

	if (!idmap_set(env->dst, dst_id, value)) {
		env->ok = 0;
		return 0;
	}

	return 1;
}

int
fsm_capture_id_compact(struct fsm *fsm, const fsm_state_t *mapping,
    size_t orig_statecount)
{
	struct capture_idmap_compact_env env;
	struct idmap *old_idmap = fsm->capture_info->end_capture_map;
	struct idmap *new_idmap = idmap_new(fsm->opt->alloc);

	if (new_idmap == NULL) {
		return 0;
	}

	env.ok = 1;
	env.dst = new_idmap;
	env.mapping = mapping;
	env.orig_statecount = orig_statecount;

	idmap_iter(old_idmap, copy_with_mapping_cb, &env);
	if (!env.ok) {
		idmap_free(new_idmap);
		return 0;
	}

	idmap_free(old_idmap);
	fsm->capture_info->end_capture_map = new_idmap;

	return 1;
}

int
fsm_capture_program_association_compact(struct fsm *fsm, const fsm_state_t *mapping,
    size_t orig_statecount)
{
	struct capture_idmap_compact_env env;
	struct idmap *old_idmap = fsm->capture_info->end_capvm_program_map;
	struct idmap *new_idmap = idmap_new(fsm->opt->alloc);

	if (new_idmap == NULL) {
		return 0;
	}

	env.ok = 1;
	env.dst = new_idmap;
	env.mapping = mapping;
	env.orig_statecount = orig_statecount;

	idmap_iter(old_idmap, copy_with_mapping_cb, &env);
	if (!env.ok) {
		idmap_free(new_idmap);
		return 0;
	}

	idmap_free(old_idmap);
	fsm->capture_info->end_capvm_program_map = new_idmap;

	return 1;
}

void
fsm_capture_update_max_capture_id(struct fsm_capture_info *ci,
	unsigned capture_id)
{
	assert(ci != NULL);
	if (capture_id >= ci->max_capture_id) {
		ci->max_capture_id = capture_id;
	}
}

int
fsm_capture_add_program(struct fsm *fsm,
	struct capvm_program *program, uint32_t *prog_id)
{
	assert(program != NULL);
	assert(prog_id != NULL);

	struct fsm_capture_info *ci = fsm->capture_info;

	if (ci->programs.used == ci->programs.ceil) {
		const size_t nceil = (ci->programs.ceil == 0
		    ? DEF_PROGRAMS_CEIL
		    : 2*ci->programs.ceil);
		assert(nceil > ci->programs.ceil);
		struct capvm_program **nset = f_realloc(fsm->opt->alloc,
		    ci->programs.set, nceil * sizeof(nset[0]));
		if (nset == NULL) {
			return 0;
		}

		ci->programs.ceil = nceil;
		ci->programs.set = nset;
	}
	assert(ci->programs.used < ci->programs.ceil);

	const unsigned max_prog_capture_id = fsm_capvm_program_get_max_capture_id(program);
	if (max_prog_capture_id > ci->max_capture_id) {
		fsm_capture_update_max_capture_id(ci, max_prog_capture_id);
	}

	*prog_id = ci->programs.used;
	ci->programs.set[ci->programs.used] = program;
	ci->programs.used++;
	return 1;
}

const struct capvm_program *
fsm_capture_get_program_by_id(const struct fsm *fsm, uint32_t prog_id)
{
	struct fsm_capture_info *ci = fsm->capture_info;
	if (prog_id >= ci->programs.used) {
		return NULL;
	}
	return ci->programs.set[prog_id];
}

int
fsm_capture_associate_program_with_end_state(struct fsm *fsm,
	uint32_t prog_id, fsm_state_t end_state)
{
	struct fsm_capture_info *ci = fsm->capture_info;
	assert(end_state < fsm->statecount);
	assert(prog_id < ci->programs.used);

	if (!idmap_set(ci->end_capvm_program_map, end_state, prog_id)) {
		return 0;
	}
	return 1;
}

struct capture_resolve_env {
	const struct fsm_capture_info *ci;
	const unsigned char *input;
	const size_t length;

	int res;
	struct fsm_capture *captures;
	size_t captures_len;
};

static int
exec_capvm_program_cb(fsm_state_t state_id, unsigned prog_id, void *opaque)
{
	struct capture_resolve_env *env = opaque;
	(void)state_id;

	/* TODO: idmap_iter could take a halt return value */
	if (env->res != 1) { return 0; }

	assert(prog_id < env->ci->programs.used);
	struct capvm_program *p = env->ci->programs.set[prog_id];

	LOG(5 - LOG_EVAL, "%s: evaluating prog_id %u for state %d\n",
	    __func__, prog_id, state_id);

#define EXEC_COUNT 1		/* can be increased for benchmarking */

	for (size_t i = 0; i < EXEC_COUNT; i++) {
		const enum fsm_capvm_program_exec_res exec_res =
		    fsm_capvm_program_exec(p,
			(const uint8_t *)env->input, env->length,
			env->captures, env->captures_len);
		if (exec_res != FSM_CAPVM_PROGRAM_EXEC_SOLUTION_WRITTEN) {
			env->res = 0;
			return 0;
		}
	}
	return 1;
}

int
fsm_capture_resolve_during_exec(const struct fsm *fsm,
	fsm_state_t end_state, const unsigned char *input, size_t input_offset,
	struct fsm_capture *captures, size_t captures_len)
{
	assert(fsm != NULL);
	assert(input != NULL);
	assert(captures != NULL);

	const struct fsm_capture_info *ci = fsm->capture_info;

	struct capture_resolve_env capture_env = {
		.res = 1,
		.ci = ci,
		.input = input,
		.length = input_offset,
		.captures = captures,
		.captures_len = captures_len,
	};

	LOG(5 - LOG_EVAL, "%s: ended on state %d\n",
	    __func__, end_state);
	idmap_iter_for_state(ci->end_capvm_program_map,
	    end_state, exec_capvm_program_cb, &capture_env);

	return capture_env.res;
}
