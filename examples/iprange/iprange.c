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
	size_t seq;
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
	enum iprange_overlap_mode overlap_mode;
	struct recmap recmap;
	struct record **byid;
	size_t rec_cap;
	unsigned nrecords;

	struct ip_interval *ranges;
	size_t nranges;
	size_t range_cap;
	size_t next_seq;

	int streaming;
	int has_pending;
	unsigned char last_start[16];
	struct ip_interval pending;
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
	ipr->overlap_mode = IPRANGE_OVERLAP_ERROR;
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

enum iprange_err
iprange_set_overlap_mode(struct iprange *ipr, enum iprange_overlap_mode mode)
{
	if (ipr == NULL || ipr->compiled || ipr->streaming) {
		errno = EINVAL;
		return IPRANGE_ERR_RANGE;
	}

	switch (mode) {
	case IPRANGE_OVERLAP_ERROR:
	case IPRANGE_OVERLAP_MOST_SPECIFIC:
	case IPRANGE_OVERLAP_FIRST:
	case IPRANGE_OVERLAP_LAST:
		ipr->overlap_mode = mode;
		return IPRANGE_OK;
	}

	errno = EINVAL;
	return IPRANGE_ERR_RANGE;
}

enum iprange_err
iprange_enable_streaming(struct iprange *ipr)
{
	if (ipr == NULL || ipr->compiled || ipr->nranges > 0 ||
	    ipr->overlap_mode != IPRANGE_OVERLAP_ERROR) {
		errno = EINVAL;
		return IPRANGE_ERR_RANGE;
	}

	ipr->streaming = 1;
	return IPRANGE_OK;
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
	case IPRANGE_ERR_ORDER:    return "streaming input is not sorted";
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

static int
addr_dec(unsigned char *addr, unsigned nbytes)
{
	for (unsigned i = nbytes; i > 0; i--) {
		if (addr[i - 1] > 0) {
			addr[i - 1]--;
			return 1;
		}
		addr[i - 1] = 255;
	}
	return 0;
}

static void
addr_diff(const unsigned char *end, const unsigned char *start,
	unsigned nbytes, unsigned char *diff_out)
{
	unsigned borrow = 0;

	for (unsigned i = nbytes; i > 0; i--) {
		int d = (int) end[i - 1] - (int) start[i - 1] - (int) borrow;
		if (d < 0) {
			d += 256;
			borrow = 1;
		} else {
			borrow = 0;
		}
		diff_out[i - 1] = (unsigned char) d;
	}
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

	if (ipr->streaming) {
		if (!ipr->has_pending) {
			memset(&ipr->pending, 0, sizeof ipr->pending);
			memcpy(ipr->last_start, start, nbytes);
			memcpy(ipr->pending.start, start, nbytes);
			memcpy(ipr->pending.end, end, nbytes);
			ipr->pending.id = r->id;
			ipr->has_pending = 1;
			return IPRANGE_OK;
		}

		if (memcmp(start, ipr->last_start, nbytes) < 0) {
			return IPRANGE_ERR_ORDER;
		}

		if (r->id == ipr->pending.id) {
			unsigned char next_after_prev[16];
			int contiguous = 0;

			if (memcmp(start, ipr->pending.end, nbytes) <= 0) {
				contiguous = 1;
			} else {
				memcpy(next_after_prev, ipr->pending.end, nbytes);
				if (addr_inc(next_after_prev, nbytes) &&
				    memcmp(start, next_after_prev, nbytes) == 0) {
					contiguous = 1;
				}
			}

			if (contiguous) {
				memcpy(ipr->last_start, start, nbytes);
				if (memcmp(end, ipr->pending.end, nbytes) > 0) {
					memcpy(ipr->pending.end, end, nbytes);
				}
				return IPRANGE_OK;
			}
		} else if (memcmp(start, ipr->pending.end, nbytes) <= 0) {
			return IPRANGE_ERR_CONFLICT;
		}

		err = decompose_interval(&ipr->builder, ipr->pending.start,
			ipr->pending.end, ipr->pending.id);
		if (err != IPRANGE_OK) {
			return err;
		}

		memcpy(ipr->last_start, start, nbytes);
		memcpy(ipr->pending.start, start, nbytes);
		memcpy(ipr->pending.end, end, nbytes);
		ipr->pending.id = r->id;
		return IPRANGE_OK;
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
	iv->seq = ipr->next_seq++;

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
interval_beats(const struct ip_interval *cand, const struct ip_interval *best,
	enum iprange_overlap_mode mode, unsigned nbytes)
{
	if (mode == IPRANGE_OVERLAP_FIRST) {
		return cand->seq < best->seq;
	}
	if (mode == IPRANGE_OVERLAP_LAST) {
		return cand->seq > best->seq;
	}
	if (mode == IPRANGE_OVERLAP_MOST_SPECIFIC) {
		unsigned char span_c[16], span_b[16];
		int cmp;

		addr_diff(cand->end, cand->start, nbytes, span_c);
		addr_diff(best->end, best->start, nbytes, span_b);
		cmp = memcmp(span_c, span_b, nbytes);
		if (cmp != 0) {
			return cmp < 0;
		}
		return cand->seq > best->seq;
	}

	return 0;
}

/*
 * Resolve overlapping intervals according to ipr->overlap_mode via an endpoint
 * sweep, producing a sorted, disjoint, maximally coalesced interval array.
 */
static enum iprange_err
resolve_overlaps_sweep(struct iprange *ipr)
{
	unsigned nbytes = (unsigned) ipr->family;
	size_t m = ipr->nranges;
	size_t nev = m * 2;
	struct sweep_event *evs = NULL;
	size_t *active = NULL;
	size_t nactive = 0;
	struct ip_interval *out = NULL;
	size_t nout = 0, out_cap = 0;

	evs = malloc(nev * sizeof *evs);
	active = malloc(m * sizeof *active);
	if (evs == NULL || active == NULL) {
		free(evs);
		free(active);
		return IPRANGE_ERR_NOMEM;
	}

	for (size_t i = 0; i < m; i++) {
		const struct ip_interval *iv = &ipr->ranges[i];
		struct sweep_event *ea = &evs[2 * i];
		struct sweep_event *eb = &evs[2 * i + 1];

		memset(ea, 0, sizeof *ea);
		memcpy(ea->pos, iv->start, nbytes);
		ea->iv_idx = i;
		ea->overflow = 0;
		ea->is_add = 1;

		memset(eb, 0, sizeof *eb);
		memcpy(eb->pos, iv->end, nbytes);
		eb->iv_idx = i;
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
			size_t idx = evs[j].iv_idx;

			if (evs[j].is_add) {
				active[nactive++] = idx;
			} else {
				size_t pos = 0;
				while (pos < nactive && active[pos] != idx) {
					pos++;
				}
				assert(pos < nactive);
				active[pos] = active[--nactive];
			}
			j++;
		}

		if (j < nev && nactive > 0 && !evs[i].overflow) {
			unsigned char slice_end[16];
			size_t best_idx = active[0];

			for (size_t k = 1; k < nactive; k++) {
				if (interval_beats(&ipr->ranges[active[k]],
					&ipr->ranges[best_idx],
					ipr->overlap_mode, nbytes)) {
					best_idx = active[k];
				}
			}

			if (evs[j].overflow) {
				memcpy(slice_end, ones, nbytes);
			} else {
				memcpy(slice_end, evs[j].pos, nbytes);
				(void) addr_dec(slice_end, nbytes);
			}

			unsigned win_id = ipr->ranges[best_idx].id;

			if (nout > 0 && out[nout - 1].id == win_id) {
				unsigned char next_after[16];
				memcpy(next_after, out[nout - 1].end, nbytes);
				if (addr_inc(next_after, nbytes) &&
				    memcmp(next_after, evs[i].pos, nbytes) == 0) {
					memcpy(out[nout - 1].end, slice_end, nbytes);
					i = j;
					continue;
				}
			}

			if (nout == out_cap) {
				size_t nc = (out_cap == 0) ? m : out_cap * 2;
				struct ip_interval *tmp = realloc(out,
					nc * sizeof *tmp);
				if (tmp == NULL) {
					free(out);
					free(active);
					free(evs);
					return IPRANGE_ERR_NOMEM;
				}
				out = tmp;
				out_cap = nc;
			}

			memset(&out[nout], 0, sizeof out[nout]);
			memcpy(out[nout].start, evs[i].pos, nbytes);
			memcpy(out[nout].end, slice_end, nbytes);
			out[nout].id = win_id;
			nout++;
		}

		i = j;
	}

	free(active);
	free(evs);
	free(ipr->ranges);
	ipr->ranges = out;
	ipr->nranges = nout;
	ipr->range_cap = out_cap;
	return IPRANGE_OK;
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

	if (ipr->overlap_mode != IPRANGE_OVERLAP_ERROR) {
		return resolve_overlaps_sweep(ipr);
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

	if (ipr->streaming) {
		if (ipr->has_pending) {
			err = decompose_interval(&ipr->builder,
				ipr->pending.start, ipr->pending.end,
				ipr->pending.id);
			if (err != IPRANGE_OK) {
				return err;
			}
		}
		return dfa_builder_finish(&ipr->builder, ipr->nrecords, fsm_out);
	}

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
