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

#if !defined(__APPLE__) && !defined(__MACH__)
#define _POSIX_C_SOURCE 200809L
#endif
#include <arpa/inet.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsm/fsm.h>

#include "iprange.h"
#include "builder.h"
#include "tree.h"

static const unsigned char zeroes[16] = { 0 };
static const unsigned char ones[16] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

struct record {
	char *rec;
	size_t len;
	RB_ENTRY(record) entry;
	unsigned id;
};

struct ip_interval {
	unsigned char start[16];
	unsigned char end[16];
	unsigned id;
};

RB_HEAD(recmap, record);

static int
recmap_cmp(const struct record *a, const struct record *b)
{
	if (a->len != b->len) {
		return (a->len > b->len) - (a->len < b->len);
	}

	return memcmp(a->rec, b->rec, a->len);
}

RB_GENERATE_STATIC(recmap, record, entry, recmap_cmp)

struct iprange {
	enum iprange_family family;
	struct recmap recmap;
	struct record **byid;
	size_t rec_cap;
	unsigned nrecords;

	struct ip_interval *ranges;
	size_t nranges;
	size_t range_cap;

	struct dfa_builder builder;

	int compiled;
};

static char *
dup_str(const char *s, size_t len)
{
	char *copy = malloc(len + 1);
	if (copy == NULL) {
		return NULL;
	}

	memcpy(copy, s, len);
	copy[len] = '\0';
	return copy;
}

static enum iprange_err
decompose_interval(struct dfa_builder *b,
	const unsigned char *start, const unsigned char *end,
	unsigned label_id)
{
	unsigned char s[16];
	unsigned nbytes = b->nbytes;

	memcpy(s, start, nbytes);

	for (;;) {
		unsigned d = 0;
		int tzeroes, tones;

		if (memcmp(s, end, nbytes) == 0) {
			if (!dfa_builder_emit_prefix(b, nbytes - 1,
				s[nbytes - 1], s[nbytes - 1], s, label_id)) {
				return IPRANGE_ERR_NOMEM;
			}
			return IPRANGE_OK;
		}

		for (unsigned i = 0; i < nbytes; i++) {
			if (s[i] != end[i]) {
				d = i;
				break;
			}
		}
		assert(s[d] < end[d]);

		tzeroes = (d + 1 == nbytes) ||
			(memcmp(&s[d + 1], zeroes, nbytes - 1 - d) == 0);
		tones = (d + 1 == nbytes) ||
			(memcmp(&end[d + 1], ones, nbytes - 1 - d) == 0);

		if (tzeroes && tones) {
			if (s[d] == 0 && end[d] == 255 && d > 0) {
				if (!dfa_builder_emit_prefix(b, d - 1,
					s[d - 1], s[d - 1], s, label_id)) {
					return IPRANGE_ERR_NOMEM;
				}
			} else {
				if (!dfa_builder_emit_prefix(b, d,
					s[d], end[d], s, label_id)) {
					return IPRANGE_ERR_NOMEM;
				}
			}
			return IPRANGE_OK;
		}

		if (tzeroes) {
			if (!dfa_builder_emit_prefix(b, d,
				s[d], (unsigned char) (end[d] - 1), s,
				label_id)) {
				return IPRANGE_ERR_NOMEM;
			}
			s[d] = end[d];
			continue;
		}

		unsigned spos = nbytes - 1;
		for (unsigned i = nbytes - 1; i > d; i--) {
			if (s[i] != 0) {
				spos = i;
				break;
			}
		}

		if (!dfa_builder_emit_prefix(b, spos, s[spos], 255, s, label_id)) {
			return IPRANGE_ERR_NOMEM;
		}

		s[spos] = 0;
		for (unsigned k = spos; k > 0; k--) {
			if (s[k - 1] < 255) {
				s[k - 1]++;
				break;
			}
			s[k - 1] = 0;
		}
	}
}

struct iprange *
iprange_new(enum iprange_family family)
{
	struct iprange *ipr;

	if (family != IPRANGE_IPV4 && family != IPRANGE_IPV6) {
		errno = EINVAL;
		return NULL;
	}

	ipr = calloc(1, sizeof *ipr);
	if (ipr == NULL) {
		return NULL;
	}

	ipr->family = family;
	RB_INIT(&ipr->recmap);
	dfa_builder_init(&ipr->builder, (unsigned) family);
	return ipr;
}

void
iprange_free(struct iprange *ipr)
{
	struct record *r, *next;

	if (ipr == NULL) {
		return;
	}

	RB_FOREACH_SAFE(r, recmap, &ipr->recmap, next) {
		RB_REMOVE(recmap, &ipr->recmap, r);
		free(r->rec);
		free(r);
	}

	dfa_builder_destroy(&ipr->builder);
	free(ipr->ranges);
	free(ipr->byid);
	free(ipr);
}

const char *
iprange_label(const struct iprange *ipr, unsigned id)
{
	assert(ipr != NULL);

	if (id >= ipr->nrecords) {
		return NULL;
	}

	return ipr->byid[id]->rec;
}

unsigned
iprange_record_count(const struct iprange *ipr)
{
	assert(ipr != NULL);
	return ipr->nrecords;
}

enum iprange_family
iprange_get_family(const struct iprange *ipr)
{
	assert(ipr != NULL);
	return ipr->family;
}

const char *
iprange_strerror(enum iprange_err err)
{
	switch (err) {
	case IPRANGE_OK:           return "ok";
	case IPRANGE_ERR_NOMEM:    return "out of memory";
	case IPRANGE_ERR_PARSE:    return "invalid IP address";
	case IPRANGE_ERR_RANGE:    return "range start exceeds range end";
	case IPRANGE_ERR_CONFLICT: return "overlapping ranges with distinct labels";
	case IPRANGE_ERR_FSM:      return "FSM operation failed";
	}

	return "unknown error";
}

static enum iprange_err
get_record(struct iprange *ipr, const char *label, size_t reclen,
	struct record **out)
{
	struct record *r, key;

	key.rec = (char *) label;
	key.len = reclen;
	r = RB_FIND(recmap, &ipr->recmap, &key);
	if (r != NULL) {
		*out = r;
		return IPRANGE_OK;
	}

	if (ipr->nrecords == ipr->rec_cap) {
		size_t new_cap = (ipr->rec_cap == 0) ? 64 : ipr->rec_cap * 2;
		struct record **new_byid = realloc(ipr->byid,
			new_cap * sizeof *new_byid);
		if (new_byid == NULL) {
			return IPRANGE_ERR_NOMEM;
		}
		ipr->byid = new_byid;
		ipr->rec_cap = new_cap;
	}

	r = calloc(1, sizeof *r);
	if (r == NULL) {
		return IPRANGE_ERR_NOMEM;
	}

	r->rec = dup_str(label, reclen);
	if (r->rec == NULL) {
		free(r);
		return IPRANGE_ERR_NOMEM;
	}

	r->len = reclen;
	r->id = ipr->nrecords++;
	ipr->byid[r->id] = r;
	RB_INSERT(recmap, &ipr->recmap, r);

	*out = r;
	return IPRANGE_OK;
}

static int
addr_inc(unsigned char *addr, unsigned nbytes)
{
	for (unsigned i = nbytes; i > 0; i--) {
		if (addr[i - 1] < 255) {
			addr[i - 1]++;
			return 1;
		}
		addr[i - 1] = 0;
	}
	return 0;
}

enum iprange_err
iprange_add(struct iprange *ipr,
	const unsigned char *start, const unsigned char *end,
	const char *label, unsigned *id_out)
{
	unsigned nbytes;
	struct record *r;
	struct ip_interval *iv;
	enum iprange_err err;

	if (ipr == NULL || start == NULL || end == NULL || label == NULL ||
	    ipr->compiled) {
		errno = EINVAL;
		return IPRANGE_ERR_RANGE;
	}

	nbytes = (unsigned) ipr->family;

	if (memcmp(start, end, nbytes) > 0) {
		return IPRANGE_ERR_RANGE;
	}

	err = get_record(ipr, label, strlen(label), &r);
	if (err != IPRANGE_OK) {
		return err;
	}

	if (id_out != NULL) {
		*id_out = r->id;
	}

	if (ipr->nranges == ipr->range_cap) {
		size_t new_cap = (ipr->range_cap == 0) ? 128 : ipr->range_cap * 2;
		struct ip_interval *tmp = realloc(ipr->ranges,
			new_cap * sizeof *tmp);
		if (tmp == NULL) {
			return IPRANGE_ERR_NOMEM;
		}
		ipr->ranges = tmp;
		ipr->range_cap = new_cap;
	}

	iv = &ipr->ranges[ipr->nranges++];
	memset(iv, 0, sizeof *iv);
	memcpy(iv->start, start, nbytes);
	memcpy(iv->end, end, nbytes);
	iv->id = r->id;

	return IPRANGE_OK;
}

enum iprange_err
iprange_add_str(struct iprange *ipr,
	const char *start_ip, const char *end_ip,
	const char *label, unsigned *id_out)
{
	unsigned char s[16], e[16];
	int af;

	if (ipr == NULL || start_ip == NULL || end_ip == NULL || label == NULL) {
		return IPRANGE_ERR_PARSE;
	}

	af = (ipr->family == IPRANGE_IPV4) ? AF_INET : AF_INET6;
	if (inet_pton(af, start_ip, s) != 1 ||
	    inet_pton(af, end_ip,   e) != 1) {
		return IPRANGE_ERR_PARSE;
	}

	return iprange_add(ipr, s, e, label, id_out);
}

static int
cmp_by_id_then_start(const void *pa, const void *pb)
{
	const struct ip_interval *a = pa;
	const struct ip_interval *b = pb;
	int c;

	if (a->id != b->id) {
		return (a->id > b->id) - (a->id < b->id);
	}
	c = memcmp(a->start, b->start, sizeof a->start);
	if (c != 0) {
		return c;
	}
	return memcmp(a->end, b->end, sizeof a->end);
}

static int
cmp_by_start_then_end(const void *pa, const void *pb)
{
	const struct ip_interval *a = pa;
	const struct ip_interval *b = pb;
	int c;

	c = memcmp(a->start, b->start, sizeof a->start);
	if (c != 0) {
		return c;
	}
	c = memcmp(a->end, b->end, sizeof a->end);
	if (c != 0) {
		return c;
	}
	return (a->id > b->id) - (a->id < b->id);
}

struct sweep_event {
	unsigned char pos[16];
	size_t iv_idx;
	int overflow;
	int is_add;
};

static int
cmp_sweep_event(const void *pa, const void *pb)
{
	const struct sweep_event *a = pa;
	const struct sweep_event *b = pb;
	int c;

	if (a->overflow != b->overflow) {
		return a->overflow - b->overflow;
	}
	if (!a->overflow) {
		c = memcmp(a->pos, b->pos, sizeof a->pos);
		if (c != 0) {
			return c;
		}
	}
	if (a->is_add != b->is_add) {
		return a->is_add - b->is_add;
	}
	return (a->iv_idx > b->iv_idx) - (a->iv_idx < b->iv_idx);
}

struct reported_set {
	unsigned *ids;
	size_t count;
};

static enum iprange_err
report_all_conflicts(const struct iprange *ipr,
	iprange_conflict_fn *on_conflict, void *conflict_opaque)
{
	unsigned nbytes = (unsigned) ipr->family;
	size_t nev = ipr->nranges * 2;
	struct sweep_event *evs = NULL;
	unsigned *active = NULL;
	size_t nactive = 0;
	struct reported_set *seen = NULL;
	size_t nseen = 0, seen_cap = 0;
	enum iprange_err ret = IPRANGE_OK;

	evs = malloc(nev * sizeof *evs);
	active = malloc(ipr->nrecords * sizeof *active);
	if (evs == NULL || active == NULL) {
		ret = IPRANGE_ERR_NOMEM;
		goto out;
	}

	for (size_t i = 0; i < ipr->nranges; i++) {
		const struct ip_interval *iv = &ipr->ranges[i];
		struct sweep_event *ea = &evs[2 * i];
		struct sweep_event *eb = &evs[2 * i + 1];

		memset(ea, 0, sizeof *ea);
		memcpy(ea->pos, iv->start, nbytes);
		ea->iv_idx = iv->id;
		ea->overflow = 0;
		ea->is_add = 1;

		memset(eb, 0, sizeof *eb);
		memcpy(eb->pos, iv->end, nbytes);
		eb->iv_idx = iv->id;
		eb->overflow = !addr_inc(eb->pos, nbytes);
		eb->is_add = 0;
	}

	qsort(evs, nev, sizeof *evs, cmp_sweep_event);

	size_t i = 0;
	while (i < nev) {
		size_t j = i;

		while (j < nev &&
		    evs[j].overflow == evs[i].overflow &&
		    (evs[i].overflow ||
		     memcmp(evs[j].pos, evs[i].pos, nbytes) == 0)) {
			unsigned id = (unsigned) evs[j].iv_idx;

			if (evs[j].is_add) {
				size_t pos = 0;
				while (pos < nactive && active[pos] < id) {
					pos++;
				}
				assert(pos == nactive || active[pos] != id);
				memmove(&active[pos + 1], &active[pos],
					(nactive - pos) * sizeof *active);
				active[pos] = id;
				nactive++;
			} else {
				size_t pos = 0;
				while (pos < nactive && active[pos] != id) {
					pos++;
				}
				assert(pos < nactive);
				memmove(&active[pos], &active[pos + 1],
					(nactive - pos - 1) * sizeof *active);
				nactive--;
			}
			j++;
		}

		if (j < nev && nactive >= 2 && !evs[i].overflow) {
			int dup = 0;
			for (size_t s = 0; s < nseen; s++) {
				if (seen[s].count == nactive &&
				    memcmp(seen[s].ids, active,
					nactive * sizeof *active) == 0) {
					dup = 1;
					break;
				}
			}

			if (!dup) {
				const char **labels;
				unsigned *copy_ids;

				if (nseen == seen_cap) {
					size_t nc = (seen_cap == 0) ? 16 : seen_cap * 2;
					struct reported_set *tmp = realloc(seen,
						nc * sizeof *tmp);
					if (tmp == NULL) {
						ret = IPRANGE_ERR_NOMEM;
						goto out;
					}
					seen = tmp;
					seen_cap = nc;
				}

				copy_ids = malloc(nactive * sizeof *copy_ids);
				labels = malloc(nactive * sizeof *labels);
				if (copy_ids == NULL || labels == NULL) {
					free(copy_ids);
					free(labels);
					ret = IPRANGE_ERR_NOMEM;
					goto out;
				}

				memcpy(copy_ids, active, nactive * sizeof *active);
				seen[nseen].ids = copy_ids;
				seen[nseen].count = nactive;
				nseen++;

				for (size_t k = 0; k < nactive; k++) {
					labels[k] = iprange_label(ipr, active[k]);
				}

				on_conflict(labels, active, nactive,
					evs[i].pos, conflict_opaque);
				free(labels);
			}
		}

		i = j;
	}

out:
	for (size_t s = 0; s < nseen; s++) {
		free(seen[s].ids);
	}
	free(seen);
	free(active);
	free(evs);
	return ret;
}

static int
is_sorted_and_cross_disjoint(const struct iprange *ipr)
{
	unsigned nbytes = (unsigned) ipr->family;
	unsigned run_id = ipr->ranges[0].id;
	unsigned char run_max_end[16];

	memcpy(run_max_end, ipr->ranges[0].end, nbytes);

	for (size_t i = 1; i < ipr->nranges; i++) {
		const struct ip_interval *cur = &ipr->ranges[i];

		if (memcmp(cur->start, ipr->ranges[i - 1].start, nbytes) < 0) {
			return 0;
		}

		if (cur->id == run_id) {
			if (memcmp(cur->end, run_max_end, nbytes) > 0) {
				memcpy(run_max_end, cur->end, nbytes);
			}
		} else {
			if (memcmp(cur->start, run_max_end, nbytes) <= 0) {
				return 0;
			}
			run_id = cur->id;
			memcpy(run_max_end, cur->end, nbytes);
		}
	}

	return 1;
}

static void
coalesce_adjacent_same_id(struct iprange *ipr)
{
	unsigned nbytes = (unsigned) ipr->family;
	size_t dst = 0;

	for (size_t i = 1; i < ipr->nranges; i++) {
		struct ip_interval *prev = &ipr->ranges[dst];
		const struct ip_interval *cur = &ipr->ranges[i];

		if (prev->id == cur->id) {
			unsigned char next_after_prev[16];
			int contiguous = 0;

			if (memcmp(cur->start, prev->end, nbytes) <= 0) {
				contiguous = 1;
			} else {
				memcpy(next_after_prev, prev->end, nbytes);
				if (addr_inc(next_after_prev, nbytes) &&
				    memcmp(cur->start, next_after_prev, nbytes) == 0) {
					contiguous = 1;
				}
			}

			if (contiguous) {
				if (memcmp(cur->end, prev->end, nbytes) > 0) {
					memcpy(prev->end, cur->end, nbytes);
				}
				continue;
			}
		}

		dst++;
		if (dst != i) {
			ipr->ranges[dst] = *cur;
		}
	}

	ipr->nranges = dst + 1;
}

static enum iprange_err
normalize_intervals(struct iprange *ipr,
	iprange_conflict_fn *on_conflict, void *conflict_opaque)
{
	unsigned nbytes = (unsigned) ipr->family;
	int has_conflict = 0;

	if (ipr->nranges <= 1) {
		return IPRANGE_OK;
	}

	/*
	 * Fast path O(M): if the input is already ordered and cross-record
	 * disjoint, coalesce consecutive same-record overlapping/adjacent
	 * intervals in a single linear pass without sorting.
	 */
	if (is_sorted_and_cross_disjoint(ipr)) {
		coalesce_adjacent_same_id(ipr);
		return IPRANGE_OK;
	}

	/* 1. Sort by (id, start, end) and merge overlapping/adjacent per id */
	qsort(ipr->ranges, ipr->nranges, sizeof *ipr->ranges,
		cmp_by_id_then_start);
	coalesce_adjacent_same_id(ipr);

	/* 2. Sort across all records by (start, end, id) to detect conflicts */
	qsort(ipr->ranges, ipr->nranges, sizeof *ipr->ranges,
		cmp_by_start_then_end);

	{
		const unsigned char *max_end = ipr->ranges[0].end;
		for (size_t i = 1; i < ipr->nranges; i++) {
			if (memcmp(ipr->ranges[i].start, max_end, nbytes) <= 0) {
				has_conflict = 1;
				break;
			}
			if (memcmp(ipr->ranges[i].end, max_end, nbytes) > 0) {
				max_end = ipr->ranges[i].end;
			}
		}
	}

	if (has_conflict) {
		if (on_conflict != NULL) {
			enum iprange_err err = report_all_conflicts(ipr,
				on_conflict, conflict_opaque);
			if (err != IPRANGE_OK) {
				return err;
			}
		}
		return IPRANGE_ERR_CONFLICT;
	}

	return IPRANGE_OK;
}

enum iprange_err
iprange_compile(struct iprange *ipr, struct fsm **fsm_out,
	iprange_conflict_fn *on_conflict, void *conflict_opaque)
{
	enum iprange_err err;

	if (ipr == NULL || fsm_out == NULL || ipr->compiled) {
		errno = EINVAL;
		return IPRANGE_ERR_FSM;
	}

	*fsm_out = NULL;
	ipr->compiled = 1;

	err = normalize_intervals(ipr, on_conflict, conflict_opaque);
	if (err != IPRANGE_OK) {
		return err;
	}

	/*
	 * Stream the sorted, pairwise-disjoint, maximally-coalesced intervals
	 * directly through the shared register stack and memoized state table,
	 * constructing the minimal DFA in one O(M * W) pass.
	 */
	for (size_t i = 0; i < ipr->nranges; i++) {
		const struct ip_interval *iv = &ipr->ranges[i];
		err = decompose_interval(&ipr->builder, iv->start, iv->end, iv->id);
		if (err != IPRANGE_OK) {
			return err;
		}
	}

	free(ipr->ranges);
	ipr->ranges = NULL;
	ipr->nranges = 0;
	ipr->range_cap = 0;

	return dfa_builder_finish(&ipr->builder, ipr->nrecords, fsm_out);
}

struct byte_slice {
	const unsigned char *p;
	size_t rem;
};

static int
slice_getc(void *opaque)
{
	struct byte_slice *s = opaque;

	if (s->rem == 0) {
		return EOF;
	}

	s->rem--;
	return (int) (*s->p++);
}

int
iprange_match(const struct fsm *fsm, enum iprange_family family,
	const unsigned char *addr, unsigned *id_out, size_t *bytes_consumed)
{
	size_t nbytes;

	if (fsm == NULL || addr == NULL ||
	    (family != IPRANGE_IPV4 && family != IPRANGE_IPV6)) {
		errno = EINVAL;
		return -1;
	}

	nbytes = (size_t) family;
	for (size_t len = 1; len <= nbytes; len++) {
		struct byte_slice s = { addr, len };
		fsm_state_t end_state;
		int r;

		r = fsm_exec(fsm, slice_getc, &s, &end_state, NULL);
		if (r < 0) {
			return -1;
		}
		if (r == 1) {
			fsm_end_id_t id = 0;

			if (fsm_endid_count(fsm, end_state) != 1 ||
			    !fsm_endid_get(fsm, end_state, 1, &id)) {
				errno = EINVAL;
				return -1;
			}
			if (id_out != NULL) {
				*id_out = id;
			}
			if (bytes_consumed != NULL) {
				*bytes_consumed = len;
			}
			return 1;
		}
	}

	return 0;
}
