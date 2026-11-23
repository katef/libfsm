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

#ifndef IPRANGE_H
#define IPRANGE_H

#include <stddef.h>

#include <fsm/fsm.h>

enum iprange_family {
	IPRANGE_IPV4 = 4,
	IPRANGE_IPV6 = 16
};

enum iprange_err {
	IPRANGE_OK = 0,
	IPRANGE_ERR_NOMEM,
	IPRANGE_ERR_PARSE,
	IPRANGE_ERR_RANGE,
	IPRANGE_ERR_ORDER,
	IPRANGE_ERR_CONFLICT,
	IPRANGE_ERR_FSM
};

struct iprange;

typedef void iprange_conflict_fn(
	const char * const *labels,
	const unsigned *ids,
	size_t count,
	const unsigned char *example_ip,
	void *opaque);

struct iprange *
iprange_new(enum iprange_family family);

void
iprange_free(struct iprange *ipr);

/*
 * Enable O(1)-buffer streaming ingestion for pre-sorted, cross-label disjoint
 * inputs. Consecutive overlapping or back-to-back ranges with the same label
 * are coalesced on the fly and emitted directly into the minimal DFA builder.
 * Out-of-order inputs return IPRANGE_ERR_ORDER; cross-label overlaps return
 * IPRANGE_ERR_CONFLICT.
 */
enum iprange_err
iprange_enable_streaming(struct iprange *ipr);

enum iprange_err
iprange_add(struct iprange *ipr,
	const unsigned char *start, const unsigned char *end,
	const char *label, unsigned *id_out);

enum iprange_err
iprange_add_str(struct iprange *ipr,
	const char *start_ip, const char *end_ip,
	const char *label, unsigned *id_out);

enum iprange_err
iprange_compile(struct iprange *ipr, struct fsm **fsm_out,
	iprange_conflict_fn *on_conflict, void *conflict_opaque);

const char *
iprange_label(const struct iprange *ipr, unsigned id);

unsigned
iprange_record_count(const struct iprange *ipr);

enum iprange_family
iprange_get_family(const struct iprange *ipr);

const char *
iprange_strerror(enum iprange_err err);

/*
 * Match an IPv4 (4-byte) or IPv6 (16-byte) binary address in network byte
 * order against a shortest-prefix leaf DFA produced by iprange_compile().
 *
 * Returns 1 on match (writing the matched record ID to *id_out and the
 * number of prefix bytes consumed to *bytes_consumed, if non-NULL),
 * 0 on non-match, or -1 on error.
 */
int
iprange_match(const struct fsm *fsm, enum iprange_family family,
	const unsigned char *addr, unsigned *id_out, size_t *bytes_consumed);

#endif
