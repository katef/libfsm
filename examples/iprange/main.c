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

#define _GNU_SOURCE
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
	struct record *r;

	/* XXX: this is a crime, we have a tree.
	 * we should be able to RB_FIND() */
	RB_FOREACH(r, recmap, &recmap) {
		if (r->id == id) {
			return r;
		}
	}

	assert(!"unreached");
	abort();
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

		byid[r->id] = r;
	} else if (r->id == rec_cap) {
		rec_cap *= 2;
		void *f = realloc(byid, rec_cap * sizeof (*byid));
		if (f == NULL) {
			perror("realloc");
			exit(-1);
		}
		byid = f;
	}

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

	for (unsigned i = range_start; i <= range_end; i++) {
		if (!fsm_addedge_literal(r->fsm, from, to, (unsigned char) i)) {
			perror("fsm_addedge_literal");
			exit(-1);
		}
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
handle_line(unsigned char *socts, unsigned char *eocts, unsigned noct,
	char *rec, size_t reclen)
{
	struct record *r;
	unsigned dbyte;

	assert(socts != NULL);
	assert(eocts != NULL);

	r = get_id(rec, reclen);

	/*
	 * If we have a /32 or a /128, the shortest path is the whole address.
	 */
	if (memcmp(socts, eocts, noct + 1) == 0) {
		gen_range(r, noct, noct, 0, 0, socts);
		return;
	}

	/*
	 * Find the first differing byte in the address.
	 */
	dbyte = 0;
	for (unsigned i = 0; i <= noct; i++) {
		if (socts[i] != eocts[i]) {
			dbyte = i;
			break;
		}
	}

	while (dbyte <= noct) {
		int szeroes, eones;
		int tzeroes, tones;

		szeroes = eones = -1;
		if (dbyte < noct) {
			szeroes = memcmp(&socts[dbyte], &zeroes, (noct - dbyte) + 1);
			eones   = memcmp(&eocts[dbyte], &ones,   (noct - dbyte) + 1);
		}

		if (szeroes == 0 && eones == 0) {
			/*
			 * If the final differing bytes are all zeroes in the starting
			 * range, and the final differing bytes are all ones, we complete
			 * the entire range.
			 */
			gen_range(r, noct, dbyte - 1, 0, 255, socts);
			return;
		}

		tzeroes = tones = -1;
		if (dbyte + 1 < noct) {
			tzeroes = memcmp(&socts[dbyte + 1], &zeroes, noct - dbyte);
			tones   = memcmp(&eocts[dbyte + 1], &ones,   noct - dbyte);
		}

		if (tzeroes == 0 && tones == 0) {
			gen_range(r, noct, dbyte - 1, socts[dbyte], eocts[dbyte], socts);
			return;
		}

		if (szeroes == 0) {
			/*
			 * our start position is all zeroes, but our end position doesn't
			 * consist of ones. this case is complicated, because it includes
			 * things like 1.0.0.0 - 1.2.255.254, 1.0.0.0 - 1.1.1.1.
			 */
			gen_range(r, noct, dbyte - 1, socts[dbyte], eocts[dbyte] - 1, socts);
			socts[dbyte] = eocts[dbyte];
			dbyte++;
			for (unsigned i = dbyte; i <= noct; i++) {
				if (socts[i] != eocts[i]) {
					dbyte = i;
					break;
				}
			}

			continue;
		}

		/*
		 * Here we have some byte position _after_ the differing
		 * byte in the start range that is non-zero, or we have some
		 * differing byte end the end range that is not 255. We want to
		 * reduce our starting range first, because this guarantees us
		 * shortest matches later.
		 */

		unsigned spos = noct;

		/*
		 * When our start position isn't all zeroes, we need to
		 * finish a starting range, and we need to finish the longest
		 * range. For example 1.0.1.0 - 1.2.255.255 must first
		 * generate `1.0.[1-255]`.
		 */
		for (unsigned i = noct; i >= dbyte; i--) {
			if (socts[i] != 0) {
				spos = i;
				break;
			}
		}

		/*
		 * We complete this range. If we complete a range starting at
		 * the initial octet, this is special because we need to move
		 * our range forward only to the ending octet (instead of 255).
		 */
		if (spos == dbyte) {
			gen_range(r, noct, spos - 1, socts[spos], eocts[spos] - 1, socts);
			for (unsigned i = spos + 1; i <= noct; i++) {
				socts[i] = 0;
			}

			socts[spos] = eocts[spos];

			// XXX: not sure about this
			break;
		} else {
			gen_range(r, noct, spos - 1, socts[spos], 255, socts);

			for (unsigned i = spos; i <= noct; i++) {
				socts[i] = 0;
			}

			socts[spos - 1]++;
		}

		/*
		 * Now, if we had e.g. 1.0.1.0, we have generated a range of
		 * `1.0.[1-255]` and updated socts to be `1.1.0.0`. Our range
		 * in our example would now be 1.1.0.0 - 1.2.255.255. We reset
		 * our dbyte calculation, since the differing byte could now
		 * be elsewhere. But it can't be before the original differing
		 * byte, so we start there.
		 */
		for (unsigned i = 0; i <= noct; i++) {
			if (socts[i] != eocts[i]) {
				dbyte = i;
				break;
			}
		}
	}
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
conflict(FILE *f, const struct fsm_options *opt,
	const fsm_end_id_t *ids, size_t count,
	const char *example, void *hook_opaque)
{
	size_t i;

	(void) f;
	(void) hook_opaque;
	(void) opt;

	fprintf(stderr, "ambiguous matches for ");

	for (i = 0; i < count; i++) {
		const struct record *r;

		r = (const void *) (intptr_t) ids[i]; /* XXX */

		fprintf(stderr, "%s", r->rec);

		if (i + 1 < count) {
			fprintf(stderr, ", ");
		}
	}

	if (example != NULL) {
		fprintf(stderr, "; for example on input '%s'", example);
	}

	fprintf(stderr, "\n");

	return 0;
}

static int
accept_dot(FILE *f, const struct fsm_options *opt,
	const fsm_end_id_t *ids, size_t count,
	void *lang_opaque, void *hook_opaque)
{
	fsm_state_t s;

	assert(f != NULL);

	(void) hook_opaque;

	s = * (fsm_state_t *) lang_opaque;

	fprintf(f, "label = <");

	if (!opt->anonymous_states) {
		fprintf(f, "%u", s);

		if (count > 0) {
			fprintf(f, "<BR/>");
		}
	}

	for (size_t i = 0; i < count; i++) {
		const struct record *r;

		r = find_id(ids[i]);

		fprintf(f, "%s", r->rec); /* XXX: escape */

		if (i + 1 < count) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, ">");

	return 0;
}

static int
comment_c(FILE *f, const struct fsm_options *opt,
	const fsm_end_id_t *ids, size_t count,
	void *hook_opaque)
{
	assert(f != NULL);

	(void) opt;
	(void) hook_opaque;

	fprintf(f, "/* ");

	for (size_t i = 0; i < count; i++) {
		const struct record *r;

		r = find_id(ids[i]);

		fprintf(f, "%s", r->rec); /* XXX: escape */

		if (i + 1 < count) {
			fprintf(f, ", ");
		}
	}

	fprintf(f, " */\n");

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

	hooks.conflict = conflict;

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

	unsigned line = 0;
	time_t tstart, tend;
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
		char *end = memchr(start, VALUE_SEP, len);

		if (end == NULL) {
			fprintf(stderr, "Invalid data: %s\n", buf);
			return -1;
		}
		*end = '\0';
		end++;
		len -= end - start;

		char *rec = memchr(end, VALUE_SEP, len);
		if (rec == NULL) {
			fprintf(stderr, "Invalid data: %s\n", buf);
			return -1;
		};
		*rec = '\0';
		rec++;
		len -= rec - end;

		if (strrchr(rec, '\n')) {
			*strrchr(rec, '\n') = '\0';
		}

		{
			struct in6_addr s6, e6;
			struct in_addr s4, e4;
			unsigned char *socts, *eocts;
			unsigned noct;

			switch (ipv) {
			case IPV4:
				if (inet_pton(AF_INET, start, &s4) != 1 ||
					inet_pton(AF_INET, end,   &e4) != 1) {
					perror("inet_pton");
					exit(-1);
				}

				socts = (unsigned char *) &s4.s_addr;
				eocts = (unsigned char *) &e4.s_addr;
				noct  = 3;
				break;

			case IPV6:
				if (inet_pton(AF_INET6, start, &s6) != 1 ||
					inet_pton(AF_INET6, end,   &e6) != 1) {
					perror("inet_pton");
					exit(-1);
				}

				socts = (unsigned char *) &s6.s6_addr;
				eocts = (unsigned char *) &e6.s6_addr;
				noct  = 15;
				break;
			}

			handle_line(socts, eocts, noct, rec, len);
		}

		if (progress) {
			tend = time(NULL);
			if (important(line)) {
				fprintf(stderr, "l%d [%lus] ", line, tend - tstart);
			}
			tstart = tend;
			line++;
		}
	}

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
			perror("fsm_determinse");
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

		(void) ci;

		if (!fsm_addedge_epsilon(fsm, fsm_start, start)) {
			perror("fsm_addedge_epsilon");
			exit(-1);
		}

		if (progress) {
			tend = time(NULL);
			if (important(line)) {
				fprintf(stderr, "r%d [%lds] ", line, tend - tstart);
			}
			tstart = tend;
			line++;
		}
	}

	fsm_setstart(fsm, fsm_start);

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", tend - tstart);
		fprintf(stderr, "converting to DFA...");
		fflush(stderr);
		tstart = time(NULL);
	}

	if (!fsm_determinise(fsm)) {
		perror("fsm_determinise");
		exit(-1);
	}

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", tend - tstart);
	}

	if (oc) {
		opt.fragment = 1;
		opt.comments = 1;
		hooks.comment = comment_c;
		fsm_print(stdout, fsm, &opt, &hooks, FSM_PRINT_C);
	} else if (odot) {
		hooks.accept = accept_dot;
		fsm_print(stdout, fsm, &opt, &hooks, FSM_PRINT_DOT);
	}
}

