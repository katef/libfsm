/*
 * Copyright 2017 Devon H. O'Dell
 * Copyright 2017 Katherine Flavel
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>

#include "builder.h"

/*
 * High bit marks leaf accepting targets (LEAF_FLAG | record_id);
 * values without LEAF_FLAG index memoized internal states 0 .. nmemo - 1.
 */
#define LEAF_FLAG    0x80000000u
#define BUCKET_EMPTY 0xffffffffu

void
dfa_builder_init(struct dfa_builder *b, unsigned nbytes)
{
	memset(b, 0, sizeof *b);
	b->nbytes = nbytes;
	b->nregs = 1; /* regs[0] is the open root state */
}

void
dfa_builder_destroy(struct dfa_builder *b)
{
	free(b->run_pool);
	free(b->mstates);
	free(b->buckets);
	memset(b, 0, sizeof *b);
}

static uint32_t
fnv_step(uint32_t h, uint32_t byte)
{
	return (uint32_t) (((uint64_t) (h ^ byte) * 16777619ULL) & 0xffffffffULL);
}

static uint32_t
hash_edge_runs(const struct edge_run *runs, unsigned nruns)
{
	uint32_t h = 2166136261u;

	for (unsigned i = 0; i < nruns; i++) {
		h = fnv_step(h, runs[i].lo);
		h = fnv_step(h, runs[i].hi);
		h = fnv_step(h, runs[i].target & 0xffu);
		h = fnv_step(h, (runs[i].target >> 8) & 0xffu);
		h = fnv_step(h, (runs[i].target >> 16) & 0xffu);
		h = fnv_step(h, (runs[i].target >> 24) & 0xffu);
	}

	return h;
}

static int
rehash_buckets(struct dfa_builder *b, size_t new_cap)
{
	uint32_t *nb;
	size_t mask = new_cap - 1;

	nb = malloc(new_cap * sizeof *nb);
	if (nb == NULL) {
		return 0;
	}

	for (size_t i = 0; i < new_cap; i++) {
		nb[i] = BUCKET_EMPTY;
	}

	for (uint32_t id = 0; id < b->nmemo; id++) {
		size_t idx = b->mstates[id].hash & mask;
		while (nb[idx] != BUCKET_EMPTY) {
			idx = (idx + 1) & mask;
		}
		nb[idx] = id;
	}

	free(b->buckets);
	b->buckets = nb;
	b->bucket_cap = new_cap;
	return 1;
}

static int
memoize_state(struct dfa_builder *b, const struct reg_state *st,
	uint32_t *id_out)
{
	uint32_t h;
	size_t mask, idx;
	uint32_t new_id;

	assert(st->nruns > 0 && st->nruns <= 256);

	if (b->bucket_cap == 0) {
		if (!rehash_buckets(b, 256)) {
			return 0;
		}
	} else if ((size_t) (b->nmemo + 1) * 10 >= b->bucket_cap * 7) {
		if (!rehash_buckets(b, b->bucket_cap * 2)) {
			return 0;
		}
	}

	h = hash_edge_runs(st->runs, st->nruns);
	mask = b->bucket_cap - 1;
	idx = h & mask;

	while (b->buckets[idx] != BUCKET_EMPTY) {
		uint32_t cand = b->buckets[idx];
		const struct memo_state *ms = &b->mstates[cand];

		if (ms->hash == h && ms->nruns == st->nruns &&
		    memcmp(&b->run_pool[ms->run_offset], st->runs,
			st->nruns * sizeof *st->runs) == 0) {
			*id_out = cand;
			return 1;
		}
		idx = (idx + 1) & mask;
	}

	if (b->nmemo >= (LEAF_FLAG - 1)) {
		return 0;
	}

	if (b->pool_count + st->nruns > b->pool_cap) {
		size_t nc = (b->pool_cap == 0) ? 1024 : b->pool_cap * 2;
		struct edge_run *tmp;

		while (nc < b->pool_count + st->nruns) {
			nc *= 2;
		}
		tmp = realloc(b->run_pool, nc * sizeof *tmp);
		if (tmp == NULL) {
			return 0;
		}
		b->run_pool = tmp;
		b->pool_cap = nc;
	}

	if (b->nmemo == b->memo_cap) {
		uint32_t nc = (b->memo_cap == 0) ? 256 : b->memo_cap * 2;
		struct memo_state *tmp = realloc(b->mstates, nc * sizeof *tmp);
		if (tmp == NULL) {
			return 0;
		}
		b->mstates = tmp;
		b->memo_cap = nc;
	}

	new_id = b->nmemo++;
	b->mstates[new_id].run_offset = (uint32_t) b->pool_count;
	b->mstates[new_id].nruns = (uint16_t) st->nruns;
	b->mstates[new_id].hash = h;

	memcpy(&b->run_pool[b->pool_count], st->runs,
		st->nruns * sizeof *st->runs);
	b->pool_count += st->nruns;

	b->buckets[idx] = new_id;
	*id_out = new_id;
	return 1;
}

static void
append_run(struct reg_state *st, unsigned char lo, unsigned char hi,
	uint32_t target)
{
	assert(lo <= hi);

	if (st->nruns > 0) {
		struct edge_run *last = &st->runs[st->nruns - 1];
		assert(last->hi < lo);
		if ((unsigned) last->hi + 1u == (unsigned) lo &&
		    last->target == target) {
			last->hi = hi;
			return;
		}
	}

	assert(st->nruns < 256);
	st->runs[st->nruns].lo = lo;
	st->runs[st->nruns].hi = hi;
	st->runs[st->nruns].target = target;
	st->nruns++;
}

static int
close_regs_to(struct dfa_builder *b, unsigned target_len)
{
	while (b->nregs > target_len) {
		uint32_t child_id;
		unsigned char edge_byte;

		b->nregs--;
		if (!memoize_state(b, &b->regs[b->nregs], &child_id)) {
			return 0;
		}
		edge_byte = b->reg_byte[b->nregs - 1];
		append_run(&b->regs[b->nregs - 1],
			edge_byte, edge_byte, child_id);
	}
	return 1;
}

/*
 * Emit a shortest-prefix transition rule into the shared register stack:
 *   octets[0 .. prefix_len - 1] . [range_start .. range_end] -> leaf(label_id)
 */
int
dfa_builder_emit_prefix(struct dfa_builder *b, unsigned prefix_len,
	unsigned char range_start, unsigned char range_end,
	const unsigned char *octets, unsigned label_id)
{
	unsigned max_lcp = prefix_len;
	unsigned lcp = 0;

	assert(prefix_len < b->nbytes);
	assert(range_start <= range_end);

	if (b->nregs - 1 < max_lcp) {
		max_lcp = b->nregs - 1;
	}
	while (lcp < max_lcp && b->reg_byte[lcp] == octets[lcp]) {
		lcp++;
	}

	if (!close_regs_to(b, lcp + 1)) {
		return 0;
	}

	for (unsigned i = lcp; i < prefix_len; i++) {
		b->reg_byte[i] = octets[i];
		b->regs[b->nregs].nruns = 0;
		b->nregs++;
	}
	assert(b->nregs == prefix_len + 1);

	append_run(&b->regs[prefix_len], range_start, range_end,
		LEAF_FLAG | (uint32_t) label_id);
	return 1;
}

static fsm_state_t
map_target(uint32_t raw_target, const fsm_state_t *leaf_state)
{
	if (raw_target & LEAF_FLAG) {
		return leaf_state[raw_target & ~LEAF_FLAG];
	}
	return (fsm_state_t) (1u + raw_target);
}

enum iprange_err
dfa_builder_finish(struct dfa_builder *b, unsigned nrecords,
	struct fsm **fsm_out)
{
	struct fsm *fsm = NULL;
	fsm_state_t *leaf_state = NULL;
	unsigned char *leaf_used = NULL;
	size_t used_leaves = 0, total_states;

	if (!close_regs_to(b, 1)) {
		return IPRANGE_ERR_NOMEM;
	}

	/* Empty language: 1-state empty minimal DFA */
	if (b->regs[0].nruns == 0) {
		fsm_state_t start;

		fsm = fsm_new_statealloc(NULL, 1);
		if (fsm == NULL) {
			return IPRANGE_ERR_FSM;
		}
		if (!fsm_addstate(fsm, &start)) {
			fsm_free(fsm);
			return IPRANGE_ERR_FSM;
		}
		fsm_setstart(fsm, start);
		*fsm_out = fsm;
		return IPRANGE_OK;
	}

	leaf_used = calloc(nrecords, sizeof *leaf_used);
	leaf_state = malloc(nrecords * sizeof *leaf_state);
	if (leaf_used == NULL || leaf_state == NULL) {
		free(leaf_used);
		free(leaf_state);
		return IPRANGE_ERR_NOMEM;
	}

	for (unsigned i = 0; i < b->regs[0].nruns; i++) {
		uint32_t t = b->regs[0].runs[i].target;
		if (t & LEAF_FLAG) {
			leaf_used[t & ~LEAF_FLAG] = 1;
		}
	}
	for (size_t i = 0; i < b->pool_count; i++) {
		uint32_t t = b->run_pool[i].target;
		if (t & LEAF_FLAG) {
			leaf_used[t & ~LEAF_FLAG] = 1;
		}
	}

	for (unsigned id = 0; id < nrecords; id++) {
		if (leaf_used[id]) {
			leaf_state[id] = (fsm_state_t) (1u + b->nmemo + used_leaves);
			used_leaves++;
		} else {
			leaf_state[id] = (fsm_state_t) -1;
		}
	}

	total_states = 1u + (size_t) b->nmemo + used_leaves;
	fsm = fsm_new_statealloc(NULL, total_states);
	if (fsm == NULL) {
		free(leaf_used);
		free(leaf_state);
		return IPRANGE_ERR_FSM;
	}

	if (!fsm_addstate_bulk(fsm, total_states)) {
		fsm_free(fsm);
		free(leaf_used);
		free(leaf_state);
		return IPRANGE_ERR_FSM;
	}

	/* State 0 is the root start state */
	fsm_setstart(fsm, 0);

	for (unsigned id = 0; id < nrecords; id++) {
		if (leaf_used[id]) {
			fsm_setend(fsm, leaf_state[id], 1);
			if (!fsm_endid_set(fsm, leaf_state[id], id)) {
				fsm_free(fsm);
				free(leaf_used);
				free(leaf_state);
				return IPRANGE_ERR_FSM;
			}
		}
	}

	for (unsigned i = 0; i < b->regs[0].nruns; i++) {
		const struct edge_run *er = &b->regs[0].runs[i];
		fsm_state_t dst = map_target(er->target, leaf_state);
		if (!fsm_addedge_range(fsm, 0, dst,
			(char) er->lo, (char) er->hi)) {
			fsm_free(fsm);
			free(leaf_used);
			free(leaf_state);
			return IPRANGE_ERR_FSM;
		}
	}

	for (uint32_t k = 0; k < b->nmemo; k++) {
		const struct memo_state *ms = &b->mstates[k];
		fsm_state_t from = (fsm_state_t) (1u + k);

		for (unsigned i = 0; i < ms->nruns; i++) {
			const struct edge_run *er = &b->run_pool[ms->run_offset + i];
			fsm_state_t dst = map_target(er->target, leaf_state);
			if (!fsm_addedge_range(fsm, from, dst,
				(char) er->lo, (char) er->hi)) {
				fsm_free(fsm);
				free(leaf_used);
				free(leaf_state);
				return IPRANGE_ERR_FSM;
			}
		}
	}

	free(leaf_used);
	free(leaf_state);
	*fsm_out = fsm;
	return IPRANGE_OK;
}
