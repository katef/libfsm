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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/print.h>

#include "tree.h"

#define VALUE_SEP ','

static struct fsm *fsm;
static fsm_state_t fsm_start;
static fsm_state_t fsm_none = (fsm_state_t) -1;

static unsigned char zeroes[16];
static unsigned char ones[16];

struct record {
	const char *rec;
	size_t len;
	RB_ENTRY(record) entry;
	unsigned id;

	struct fsm *fsm;
	fsm_state_t start;
	fsm_state_t end;
	struct {
		fsm_state_t s;
		unsigned char c;
	} regs[16];
};

static RB_HEAD(recmap, record) recmap = RB_INITIALIZER(recmap);
static struct record **byid = NULL;
static size_t rec_cap = 1000000;

static int
recmap_cmp(const struct record *a, const struct record *b)
{
	long dlen = a->len - b->len;

	if (dlen) return (dlen > 0) - (dlen < 0);
	return strcmp(a->rec, b->rec);
}

RB_GENERATE_STATIC(recmap, record, entry, recmap_cmp)

static unsigned nrecords;

static struct fsm_options opt;
static struct fsm_hooks hooks;

static struct record *
find_id(unsigned id)
{
	assert(id < nrecords);
	return byid[id];
}

static struct record *
get_id(char *rec, size_t reclen)
{
	struct record *r, s;

	s.rec = rec;
	s.len = reclen;
	r = RB_FIND(recmap, &recmap, &s);
	if (r != NULL) {
		return r;
	}

	r = malloc(sizeof *r);
	if (r == NULL) {
		perror("malloc");
		exit(-1);
	}

	r->rec = strdup(rec);
	if (r->rec == NULL) {
		perror("strdup");
		exit(-1);
	}

	r->len = reclen;
	r->id  = nrecords++;
	r->fsm = fsm_new(NULL);
	if (r->fsm == NULL) {
		perror("fsm_new");
		exit(-1);
	}

	if (!fsm_addstate(r->fsm, &r->start)) {
		perror("fsm_addstate");
		exit(-1);
	}
	fsm_setstart(r->fsm, r->start);

	if (!fsm_addstate(r->fsm, &r->end)) {
		perror("fsm_addstate");
		exit(-1);
	}
	fsm_setend(r->fsm, r->end, 1);

	for (size_t i = 0; i < sizeof r->regs / sizeof *r->regs; i++) {
		r->regs[i].c = '\0';
		r->regs[i].s = fsm_none;
	}

	RB_INSERT(recmap, &recmap, r);

	if (byid == NULL) {
		byid = malloc(rec_cap * sizeof (*byid));
		if (byid == NULL) {
			perror("malloc");
			exit(-1);
		}
	} else if (r->id == rec_cap) {
		rec_cap *= 2;
		void *f = realloc(byid, rec_cap * sizeof (*byid));
		if (f == NULL) {
			perror("realloc");
			exit(-1);
		}
		byid = f;
	}
	byid[r->id] = r;

	return r;
}

static void
usage(void)
{
	fprintf(stderr, "iprange -[46] [-f <file>] -l fmt\n"
	    "\t-4\t\tIPv4\n"
	    "\t-6\t\tIPv6\n"
	    "\t-f <file>\tuse <file> as input\n"
	    "\t-l fmt\t\tOutput format (c, dot)\n");
	exit(-1);
}

static fsm_state_t
get_from(struct record *r, unsigned oct)
{
	if (oct == 0) {
		return r->start;
	}

	return r->regs[oct - 1].s;
}

static fsm_state_t
get_to(struct record *r, unsigned oct, unsigned noct, const unsigned char *octs)
{
	fsm_state_t to;

	if (r->regs[oct].s != fsm_none && r->regs[oct].c == octs[oct]) {
		return r->regs[oct].s;
	}

	if (!fsm_addstate(r->fsm, &to)) {
		perror("fsm_addstate");
		exit(-1);
	}

	r->regs[oct].s = to;
	r->regs[oct].c = octs[oct];

	for (unsigned i = oct + 1; i < noct; i++) {
		r->regs[i].s = fsm_none;
	}

	return to;
}

static void
gen_edge_range(struct record *r, unsigned oct, const unsigned char *octs,
    unsigned range_start, unsigned range_end, unsigned noct, unsigned final)
{
	fsm_state_t from, to;

	from = get_from(r, oct);
	if (oct > 0) {
		assert(r->regs[oct - 1].c == octs[oct - 1]);
	} else {
		assert(from == r->start);
	}

	if (final) {
		to = r->end;
	} else {
		to = get_to(r, oct, noct, octs);
		assert(r->regs[oct].c == octs[oct]);
	}

	if (!fsm_addedge_range(r->fsm, from, to, range_start, range_end)) {
		perror("fsm_addedge_range");
		exit(-1);
	}
}

static void
gen_range(struct record *r, unsigned noct, int prefix_octs, unsigned char range_start,
    unsigned char range_end, unsigned char *octets)
{
	if (prefix_octs < 0) {
		gen_edge_range(r, 0, octets, range_start, range_end, noct + 1, 1);
		return;
	}

	for (unsigned i = 0; (int) i <= prefix_octs; i++) {
		int final = 0;

		if (((int) i == prefix_octs && range_start == 0 && range_end == 255) ||
		    (i == noct)) {
			final = 1;
		}
		gen_edge_range(r, i, octets, octets[i], octets[i], noct + 1, final);

		if (final) {
			return;
		}
	}

	if (prefix_octs != (int) noct) {
		gen_edge_range(r, prefix_octs + 1, octets, range_start, range_end, noct, 1);
	}
}

static void
emit_interval(struct record *r, const unsigned char *start,
	const unsigned char *eocts, unsigned noct)
{
	unsigned char socts[16];
	unsigned nbytes = noct + 1;

	assert(start != NULL);
	assert(eocts != NULL);

	memcpy(socts, start, nbytes);

	for (;;) {
		unsigned dbyte = 0;
		int szeroes, eones;
		int tzeroes, tones;

		/*
		 * If we have a /32 or a /128 (or advanced to match eocts),
		 * the shortest path is the whole address.
		 */
		if (memcmp(socts, eocts, nbytes) == 0) {
			gen_range(r, noct, (int) noct, 0, 0, socts);
			return;
		}

		/*
		 * Find the first differing byte in the address.
		 */
		for (unsigned i = 0; i <= noct; i++) {
			if (socts[i] != eocts[i]) {
				dbyte = i;
				break;
			}
		}
		assert(socts[dbyte] < eocts[dbyte]);

		szeroes = memcmp(&socts[dbyte], zeroes, (noct - dbyte) + 1);
		eones   = memcmp(&eocts[dbyte], ones,   (noct - dbyte) + 1);

		if (szeroes == 0 && eones == 0) {
			/*
			 * If the final differing bytes are all zeroes in the starting
			 * range, and the final differing bytes are all ones, we complete
			 * the entire range.
			 */
			gen_range(r, noct, (int) dbyte - 1, 0, 255, socts);
			return;
		}

		tzeroes = (dbyte == noct) ? 0
			: memcmp(&socts[dbyte + 1], zeroes, noct - dbyte);
		tones   = (dbyte == noct) ? 0
			: memcmp(&eocts[dbyte + 1], ones,   noct - dbyte);

		if (tzeroes == 0 && tones == 0) {
			gen_range(r, noct, (int) dbyte - 1, socts[dbyte], eocts[dbyte], socts);
			return;
		}

		if (tzeroes == 0) {
			/*
			 * Trailing bytes after dbyte in socts are all zeroes,
			 * while eocts does not end in all ones (e.g.
			 * 1.0.0.0 - 1.2.255.254 or 1.1.0.0 - 1.2.0.0).
			 */
			gen_range(r, noct, (int) dbyte - 1, socts[dbyte],
				(unsigned char) (eocts[dbyte] - 1), socts);
			socts[dbyte] = eocts[dbyte];
			continue;
		}

		/*
		 * Here we have some byte position _after_ the differing
		 * byte in the start range that is non-zero. Reduce the
		 * starting range from right to left to align to zeroes.
		 */
		unsigned spos = noct;
		for (unsigned i = noct; i > dbyte; i--) {
			if (socts[i] != 0) {
				spos = i;
				break;
			}
		}

		gen_range(r, noct, (int) spos - 1, socts[spos], 255, socts);
		socts[spos] = 0;

		for (unsigned k = spos; k > 0; k--) {
			if (socts[k - 1] < 255) {
				socts[k - 1]++;
				break;
			}
			socts[k - 1] = 0;
		}
	}
}

struct ip_interval {
	unsigned char start[16];
	unsigned char end[16];
	unsigned id;
};

static struct ip_interval *ranges;
static size_t nranges;
static size_t range_cap;

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
	unsigned id;
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
	return (a->id > b->id) - (a->id < b->id);
}

static void
report_conflicts(unsigned nbytes, int af)
{
	size_t nev = nranges * 2;
	struct sweep_event *evs = malloc(nev * sizeof *evs);
	unsigned *active = malloc(nrecords * sizeof *active);
	size_t nactive = 0;

	if (evs == NULL || active == NULL) {
		perror("malloc");
		exit(-1);
	}

	for (size_t i = 0; i < nranges; i++) {
		memset(&evs[2 * i], 0, sizeof evs[2 * i]);
		memcpy(evs[2 * i].pos, ranges[i].start, nbytes);
		evs[2 * i].id = ranges[i].id;
		evs[2 * i].is_add = 1;

		memset(&evs[2 * i + 1], 0, sizeof evs[2 * i + 1]);
		memcpy(evs[2 * i + 1].pos, ranges[i].end, nbytes);
		evs[2 * i + 1].id = ranges[i].id;
		evs[2 * i + 1].overflow = !addr_inc(evs[2 * i + 1].pos, nbytes);
		evs[2 * i + 1].is_add = 0;
	}

	qsort(evs, nev, sizeof *evs, cmp_sweep_event);

	for (size_t i = 0, j; i < nev; i = j) {
		j = i;
		while (j < nev &&
		    evs[j].overflow == evs[i].overflow &&
		    (evs[i].overflow || memcmp(evs[j].pos, evs[i].pos, nbytes) == 0)) {
			unsigned id = evs[j].id;
			if (evs[j].is_add) {
				size_t pos = 0;
				while (pos < nactive && active[pos] < id) {
					pos++;
				}
				memmove(&active[pos + 1], &active[pos],
					(nactive - pos) * sizeof *active);
				active[pos] = id;
				nactive++;
			} else {
				size_t pos = 0;
				while (pos < nactive && active[pos] != id) {
					pos++;
				}
				memmove(&active[pos], &active[pos + 1],
					(nactive - pos - 1) * sizeof *active);
				nactive--;
			}
			j++;
		}

		if (j < nev && nactive >= 2 && !evs[i].overflow) {
			char ipbuf[INET6_ADDRSTRLEN];

			fprintf(stderr, "ambiguous matches for ");
			for (size_t k = 0; k < nactive; k++) {
				fprintf(stderr, "%s%s", find_id(active[k])->rec,
					(k + 1 < nactive) ? ", " : "");
			}
			if (inet_ntop(af, evs[i].pos, ipbuf, sizeof ipbuf) != NULL) {
				fprintf(stderr, "; for example on input '%s'", ipbuf);
			}
			fprintf(stderr, "\n");
		}
	}

	free(active);
	free(evs);
}

static void
coalesce_same_id(unsigned nbytes)
{
	size_t dst = 0;

	for (size_t i = 1; i < nranges; i++) {
		struct ip_interval *prev = &ranges[dst];
		const struct ip_interval *cur = &ranges[i];

		if (prev->id == cur->id) {
			unsigned char next_ip[16];
			int contiguous = (memcmp(cur->start, prev->end, nbytes) <= 0);

			if (!contiguous) {
				memcpy(next_ip, prev->end, nbytes);
				if (addr_inc(next_ip, nbytes) &&
				    memcmp(cur->start, next_ip, nbytes) == 0) {
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
			ranges[dst] = *cur;
		}
	}
	nranges = dst + 1;
}

static void
normalize_ranges(unsigned nbytes, int af)
{
	int sorted_disjoint = 1;

	if (nranges <= 1) {
		return;
	}

	{
		unsigned run_id = ranges[0].id;
		unsigned char run_max_end[16];

		memcpy(run_max_end, ranges[0].end, nbytes);
		for (size_t i = 1; i < nranges; i++) {
			if (memcmp(ranges[i].start, ranges[i - 1].start, nbytes) < 0) {
				sorted_disjoint = 0;
				break;
			}
			if (ranges[i].id == run_id) {
				if (memcmp(ranges[i].end, run_max_end, nbytes) > 0) {
					memcpy(run_max_end, ranges[i].end, nbytes);
				}
			} else {
				if (memcmp(ranges[i].start, run_max_end, nbytes) <= 0) {
					sorted_disjoint = 0;
					break;
				}
				run_id = ranges[i].id;
				memcpy(run_max_end, ranges[i].end, nbytes);
			}
		}
	}

	if (sorted_disjoint) {
		coalesce_same_id(nbytes);
		return;
	}

	qsort(ranges, nranges, sizeof *ranges, cmp_by_id_then_start);
	coalesce_same_id(nbytes);

	qsort(ranges, nranges, sizeof *ranges, cmp_by_start_then_end);
	{
		const unsigned char *max_end = ranges[0].end;
		for (size_t i = 1; i < nranges; i++) {
			if (memcmp(ranges[i].start, max_end, nbytes) <= 0) {
				report_conflicts(nbytes, af);
				exit(EXIT_FAILURE);
			}
			if (memcmp(ranges[i].end, max_end, nbytes) > 0) {
				max_end = ranges[i].end;
			}
		}
	}
}

static void
handle_line(unsigned char *socts, unsigned char *eocts, unsigned noct,
	char *rec, size_t reclen)
{
	struct record *r;
	unsigned nbytes = noct + 1;
	struct ip_interval *iv;

	if (memcmp(socts, eocts, nbytes) > 0) {
		fprintf(stderr, "range start exceeds range end\n");
		exit(EXIT_FAILURE);
	}

	r = get_id(rec, reclen);

	if (nranges == range_cap) {
		size_t nc = (range_cap == 0) ? 128 : range_cap * 2;
		struct ip_interval *tmp = realloc(ranges, nc * sizeof *tmp);
		if (tmp == NULL) {
			perror("realloc");
			exit(-1);
		}
		ranges = tmp;
		range_cap = nc;
	}

	iv = &ranges[nranges++];
	memset(iv, 0, sizeof *iv);
	memcpy(iv->start, socts, nbytes);
	memcpy(iv->end, eocts, nbytes);
	iv->id = r->id;
}

static int
important(unsigned n)
{
	for (;;) {
		if (n < 10) {
			return 1;
		}

		if ((n % 10) != 0) {
			return 0;
		}

		n /= 10;
	}
}

static int
accept_dot(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	fsm_state_t s;

	assert(f != NULL);
	assert(state_metadata != NULL);

	(void) hook_opaque;

	s = * (fsm_state_t *) lang_opaque;

	fprintf(f, "label = <");

	if (!opt->anonymous_states) {
		fprintf(f, "%u", s);

		if (state_metadata->end_id_count > 0) {
			fprintf(f, "<BR/>");
		}
	}

	for (size_t i = 0; i < state_metadata->end_id_count; i++) {
		const struct record *r;

		r = find_id(state_metadata->end_ids[i]);

		fprintf(f, "%s", r->rec); /* XXX: escape */

		if (i + 1 < state_metadata->end_id_count) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, ">");

	return 0;
}

static int
accept_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	unsigned id;
	const struct record *r;

	assert(f != NULL);
	assert(state_metadata != NULL);
	assert(state_metadata->end_id_count == 1);

	(void) lang_opaque;
	(void) hook_opaque;

	id = state_metadata->end_ids[0];
	r = find_id(id);

	fprintf(f, "return 0x%x;", id);
	if (opt->comments) {
		fprintf(f, " /* %s */", r->rec);
	}

	return 0;
}

static int
reject_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	if (state_metadata != NULL && state_metadata->end_id_count > 0) {
		return accept_c(f, opt, state_metadata, lang_opaque, hook_opaque);
	}

	fprintf(f, "return -1;");
	return 0;
}

int
main(int argc, char **argv)
{
	static enum { IPV4, IPV6 } ipv;
	int progress = 0;
	FILE *fd = NULL;
	int odot = 0;
	int oc = 0;
	int c;

	opt.ambig             = AMBIG_ERROR;
	opt.prefix            = NULL;
	opt.always_hex        = 1;
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.case_ranges       = 1;

	while (c = getopt(argc, argv, "46f:l:Q"), c != -1) {
		switch (c) {
		case '4': ipv = IPV4; break;
		case '6': ipv = IPV6; break;

		case 'l':
			if (strcmp(optarg, "dot") == 0) {
				odot = 1;
			} else if (strcmp(optarg, "c") == 0) {
				oc = 1;
			} else {
				fprintf(stderr, "Invalid language '%s'", optarg);
				usage();
			}

			break;

		case 'f':
			fd = fopen(optarg, "r");
			if (fd == NULL) {
				perror("fopen");
				usage();
			}
			break;

		case 'Q':
			progress = 1;
			break;

		default:
			usage();
		}
	}

	if (fd == NULL) {
		fd = stdin;
	}

	if (oc == 0 && odot == 0) {
		usage();
	}

	memset(ones, 0xff, sizeof ones);

	fsm = fsm_new(NULL);
	if (fsm == NULL) {
		perror("fsm_new");
		return -1;
	}

	if (!fsm_addstate(fsm, &fsm_start)) {
		perror("fsm_addstate");
		return -1;
	}

	unsigned noct = (ipv == IPV4) ? 3 : 15;
	int af = (ipv == IPV4) ? AF_INET : AF_INET6;
	unsigned line = 0;
	time_t tstart = 0, tend;
	if (progress) {
		tstart = time(NULL);
		fprintf(stderr, "Reading lines... ");
		fflush(stderr);
	}

	char *buf = NULL;
	size_t cap = 0;
	ssize_t len;

	while (len = getline(&buf, &cap, fd), len > 0) {
		char *start = buf;
		char *end = memchr(start, VALUE_SEP, (size_t) len);

		if (end == NULL) {
			fprintf(stderr, "Invalid data: %s\n", buf);
			return -1;
		}
		*end = '\0';
		end++;
		len -= end - start;

		char *rec = memchr(end, VALUE_SEP, (size_t) len);
		if (rec == NULL) {
			fprintf(stderr, "Invalid data: %s\n", buf);
			return -1;
		}
		*rec = '\0';
		rec++;

		rec[strcspn(rec, "\r\n")] = '\0';

		{
			struct in6_addr s6, e6;
			struct in_addr s4, e4;
			unsigned char *socts = NULL, *eocts = NULL;

			switch (ipv) {
			case IPV4:
				if (inet_pton(AF_INET, start, &s4) != 1 ||
					inet_pton(AF_INET, end,   &e4) != 1) {
					perror("inet_pton");
					exit(-1);
				}

				socts = (unsigned char *) &s4.s_addr;
				eocts = (unsigned char *) &e4.s_addr;
				break;

			case IPV6:
				if (inet_pton(AF_INET6, start, &s6) != 1 ||
					inet_pton(AF_INET6, end,   &e6) != 1) {
					perror("inet_pton");
					exit(-1);
				}

				socts = (unsigned char *) &s6.s6_addr;
				eocts = (unsigned char *) &e6.s6_addr;
				break;
			}

			handle_line(socts, eocts, noct, rec, strlen(rec));
		}

		if (progress) {
			tend = time(NULL);
			if (important(line)) {
				fprintf(stderr, "l%u [%lds] ", line, (long) (tend - tstart));
			}
			tstart = tend;
			line++;
		}
	}

	free(buf);
	if (fd != stdin) {
		fclose(fd);
	}

	normalize_ranges(noct + 1, af);

	for (size_t i = 0; i < nranges; i++) {
		emit_interval(find_id(ranges[i].id),
			ranges[i].start, ranges[i].end, noct);
	}
	free(ranges);

	if (progress) {
		fprintf(stderr, "\nminimizing/unioning %u records...", nrecords);
		fflush(stderr);

		tstart = time(NULL);
		line = 0;
	}

	struct record *r;
	RB_FOREACH(r, recmap, &recmap) {
		struct fsm_combine_info ci;
		fsm_state_t start;

		if (!fsm_determinise(r->fsm)) {
			perror("fsm_determinise");
			exit(-1);
		}

		if (!fsm_minimise(r->fsm)) {
			perror("fsm_minimise");
			exit(-1);
		}

		fsm_setendid(r->fsm, r->id);

		(void) fsm_getstart(r->fsm, &start);

		fsm = fsm_merge(fsm, r->fsm, &ci);
		if (fsm == NULL) {
			perror("fsm_merge");
			exit(-1);
		}

		fsm_start += ci.base_a;
		start     += ci.base_b;

		if (!fsm_addedge_epsilon(fsm, fsm_start, start)) {
			perror("fsm_addedge_epsilon");
			exit(-1);
		}

		if (progress) {
			tend = time(NULL);
			if (important(line)) {
				fprintf(stderr, "r%u [%lds] ", line, (long) (tend - tstart));
			}
			tstart = tend;
			line++;
		}
	}

	fsm_setstart(fsm, fsm_start);

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", (long) (tend - tstart));
		fprintf(stderr, "converting to DFA...");
		fflush(stderr);
		tstart = time(NULL);
	}

	if (!fsm_determinise(fsm)) {
		perror("fsm_determinise");
		exit(-1);
	}

	if (!fsm_minimise(fsm)) {
		perror("fsm_minimise");
		exit(-1);
	}

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", (long) (tend - tstart));
	}

	if (oc) {
		opt.fragment = 1;
		opt.comments = 1;
		hooks.accept = accept_c;
		hooks.reject = reject_c;
		fsm_print(stdout, fsm, &opt, &hooks, FSM_PRINT_C);
	} else if (odot) {
		hooks.accept = accept_dot;
		fsm_print(stdout, fsm, &opt, &hooks, FSM_PRINT_DOT);
	}

	return 0;
}

