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

#ifndef IPRANGE_BUILDER_H
#define IPRANGE_BUILDER_H

#include <stddef.h>
#include <stdint.h>

#include <fsm/fsm.h>

#include "iprange.h"

struct edge_run {
	unsigned char lo;
	unsigned char hi;
	uint32_t target;
};

struct reg_state {
	struct edge_run runs[256];
	unsigned nruns;
};

struct memo_state {
	uint32_t run_offset;
	uint16_t nruns;
	uint32_t hash;
};

/*
 * Single-pass minimal acyclic DFA builder for sorted byte-prefix rules.
 *
 * regs[0 .. nregs - 1] tracks the open states along the active byte prefix
 * (reg_byte[0 .. nregs - 2]). Once a prefix byte advances, deeper register
 * states are permanently finished, memoized by their outgoing byte-run table,
 * and linked into their parent register state.
 */
struct dfa_builder {
	unsigned nbytes;

	struct reg_state regs[16];
	unsigned char reg_byte[16];
	unsigned nregs;

	struct edge_run *run_pool;
	size_t pool_count;
	size_t pool_cap;

	struct memo_state *mstates;
	uint32_t nmemo;
	uint32_t memo_cap;

	uint32_t *buckets;
	size_t bucket_cap;
};

void
dfa_builder_init(struct dfa_builder *b, unsigned nbytes);

void
dfa_builder_destroy(struct dfa_builder *b);

int
dfa_builder_emit_prefix(struct dfa_builder *b, unsigned prefix_len,
	unsigned char range_start, unsigned char range_end,
	const unsigned char *octets, unsigned label_id);

enum iprange_err
dfa_builder_finish(struct dfa_builder *b, unsigned nrecords,
	struct fsm **fsm_out);

#endif
