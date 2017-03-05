#define _GNU_SOURCE
#include <arpa/inet.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <adt/set.h> /* XXX */
#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/out.h>
#include <fsm/options.h>
#include <fsm/pred.h>

#include "../../src/libfsm/out.h" /* XXX */
#include "../../src/libfsm/internal.h" /* XXX */

#include "tree.h"

#define VALUE_SEP ','

static struct fsm *fsm;
static struct fsm_state *fsmstart;

static unsigned char zeroes[16];
static unsigned char ones[16];

static unsigned ipv;

struct record {
	const char *rec;
	size_t len;
	RB_ENTRY(record) entry;
	unsigned id;

	struct fsm *fsm;
	struct fsm_state *start;
	struct fsm_state *end;
	struct {
		struct fsm_state *s;
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

static struct record *
get_id(char *rec, size_t reclen)
{
	struct record *r, s;

	s.rec = rec;
	s.len = reclen;
	r = RB_FIND(recmap, &recmap, &s);
	if (r == NULL) {
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
		r->id = nrecords++;
		r->fsm = fsm_new(&opt);
		if (r->fsm == NULL) {
			perror("fsm_new");
			exit(-1);
		}

		r->start = fsm_addstate(r->fsm);
		if (r->start == NULL) {
			perror("fsm_addstate");
			exit(-1);
		}
		fsm_setstart(r->fsm, r->start);

		r->end = fsm_addstate(r->fsm);
		if (r->end == NULL) {
			perror("fsm_addstate");
			exit(-1);
		}
		fsm_setend(r->fsm, r->end, 1);
		fsm_setopaque(r->fsm, r->end, r);

		memset(r->regs, 0, sizeof r->regs);

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
	}

	return r;
}

static void
usage(void)
{
	fprintf(stderr, "ip2fsm -[46] [-f <file>] -l fmt\n"
	    "\t-4\t\tIPv4\n"
	    "\t-6\t\tIPv6\n"
	    "\t-f <file>\tuse <file> as input\n"
	    "\t-l fmt\t\tOutput format (c, dot)\n");
	exit(-1);
}

struct fsm_state *
get_from(struct record *r, unsigned oct)
{

	if (oct == 0) return r->start;
	return r->regs[oct - 1].s;
}

struct fsm_state *
get_to(struct record *r, unsigned oct, unsigned noct, const unsigned char *octs)
{
	if (r->regs[oct].s != NULL && r->regs[oct].c == octs[oct]) {
		return r->regs[oct].s;
	}

	struct fsm_state *to = fsm_addstate(r->fsm);
	if (to == NULL) {
		perror("fsm_addstate");
		exit(-1);
	}

	r->regs[oct].s = to;
	r->regs[oct].c = octs[oct];

	for (int i = oct + 1; i < noct; i++) {
		r->regs[i].s = NULL;
	}

	return to;
}

static void
gen_edge_range(struct record *r, unsigned oct, const unsigned char *octs,
    unsigned range_start, unsigned range_end, unsigned noct, unsigned final)
{
	struct fsm_state *from, *to;
	struct fsm_edge *e;

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

	for (int i = range_start; i <= range_end; i++) {
		e = fsm_addedge_literal(r->fsm, from, to, i);
		if (e == NULL) {
			perror("fsm_addedge_literal");
			exit(-1);
		}
	}
}

static void
gen_range(struct record *r, unsigned ipv, int prefix_octs, unsigned char range_start,
    unsigned char range_end, unsigned char *octets)
{
	int noct;

	if (ipv == 4) {
		noct = 3;
	} else {
		noct = 15;
	}

	if (prefix_octs < 0) {
		gen_edge_range(r, 0, octets, range_start, range_end, noct + 1, 1);
		return;
	}

	for (int i = 0; i <= prefix_octs; i++) {
		int final = 0;

		if ((i == prefix_octs && range_start == 0 && range_end == 255) ||
		    (i == noct)) {
			final = 1;
		}
		gen_edge_range(r, i, octets, octets[i], octets[i], noct + 1, final);

		if (final) {
			return;
		}
	}

	if (prefix_octs != noct) {
		gen_edge_range(r, prefix_octs + 1, octets, range_start, range_end, noct, 1);
	}
}

static void
handle_line(char *start, char *end, char *rec, size_t reclen)
{
	unsigned char *socts, *eocts;
	struct in6_addr s6, e6;
	struct in_addr s4, e4;
	struct record *r;
	unsigned noct;

	r = get_id(rec, reclen);

	if (ipv == 4) {
		if (inet_pton(AF_INET, start, &s4) != 1 ||
		    inet_pton(AF_INET, end, &e4) != 1) {
			perror("inet_pton");
			exit(-1);
		}

		socts = (unsigned char *)&s4.s_addr;
		eocts = (unsigned char *)&e4.s_addr;
		noct = 3;
	} else {
		if (inet_pton(AF_INET6, start, &s6) != 1 ||
		    inet_pton(AF_INET6, end, &e6) != 1) {
			perror("inet_pton");
			exit(-1);
		}

		socts = (unsigned char *)&s6.s6_addr;
		eocts = (unsigned char *)&e6.s6_addr;
		noct = 15;
	}

	/*
	 * If we have a /32 or a /128, the shortest path is the whole address.
	 */
	if (memcmp(socts, eocts, noct + 1) == 0) {
		gen_range(r, ipv, noct, 0, 0, socts);
		return;
	}

	/*
	 * Find the first differing byte in the address.
	 */
	unsigned dbyte = 0;
	for (int i = 0; i <= noct; i++) {
		if (socts[i] != eocts[i]) {
			dbyte = i;
			break;
		}
	}

	/*
	 * If the addresses differ at the final byte of the address, the
	 * shortest path still requires all octets. So the only thing we
	 * can do is complete the range from start to end.
	 */
	if (dbyte == noct) {
		gen_range(r, ipv, noct - 1, socts[noct], eocts[noct], socts);
		return;
	}

	while (dbyte < noct) {
		int szeroes = memcmp(&socts[dbyte], &zeroes, (noct - dbyte) + 1);
		int eones = memcmp(&eocts[dbyte], &ones, (noct - dbyte) + 1);

		if (szeroes == 0 && eones == 0) {
			/*
			 * If the final differing bytes are all zeroes in the starting
			 * range, and the final differing bytes are all ones, we complete
			 * the entire range.
			 */
			gen_range(r, ipv, dbyte - 1, 0, 255, socts);
			return;
		} else {
			int tzeroes = memcmp(&socts[dbyte + 1], &zeroes, noct - dbyte);
			int tones = memcmp(&eocts[dbyte + 1], &ones, noct - dbyte);
			if (tzeroes == 0 && tones == 0) {
				gen_range(r, ipv, dbyte - 1, socts[dbyte], eocts[dbyte], socts);
				return;
			}

			/*
			 * This means that we have some byte position _after_ the differing
			 * byte in the start range that is non-zero, or we have some
			 * differing byte end the end range that is not 255. We want to
			 * reduce our starting range first, because this guarantees us
			 * shortest matches later.
			 */
			if (szeroes != 0) {
				int spos = noct;

				/*
				 * When our start position isn't all zeroes, we need to
				 * finish a starting range, and we need to finish the longest
				 * range. For example 1.0.1.0 - 1.2.255.255 must first
				 * generate `1.0.[1-255]`.
				 */
				for (int i = noct; i >= dbyte; i--) {
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
					gen_range(r, ipv, spos-1, socts[spos], eocts[spos] - 1, socts);
					for (int i = spos + 1; i <= noct; i++) {
						socts[i] = 0;
					}

					socts[spos] = eocts[spos];
				} else {
					gen_range(r, ipv, spos - 1, socts[spos], 255, socts);
		
					for (int i = spos; i <= noct; i++) {
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
				for (int i = 0; i <= noct; i++) {
					if (socts[i] != eocts[i]) {
						dbyte = i;
						break;
					}
				}

			} else {
				/*
				 * Our start position is all zeroes, but our end position doesn't
				 * consist of ones. This case is complicated, because it includes
				 * things like 1.0.0.0 - 1.2.255.254, 1.0.0.0 - 1.1.1.1.
				 */
				gen_range(r, ipv, dbyte - 1, socts[dbyte], eocts[dbyte] - 1, socts);
				socts[dbyte] = eocts[dbyte];	
				dbyte++;
				for (int i = dbyte; i <= noct; i++) {
					if (socts[i] != eocts[i]) {
						dbyte = i;
						break;
					}
				}
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

static void
carryopaque(struct set *set, struct fsm *fsm, struct fsm_state *st)
{
	struct fsm_state *s;
	struct set_iter it;
	void *o = NULL;

	/* XXX: This should be possible without adt/set.h */
	for (s = set_first(set, &it); s != NULL; s = set_next(&it)) {
		if (!fsm_isend(fsm, s)) {
			continue;
		}

		if (o == NULL) {
			o = fsm_getopaque(fsm, s);
			fsm_setopaque(fsm, st, o);
			continue;
		} else {
			assert(o == fsm_getopaque(fsm, s));
		}
	}
}

const char *
nl(struct fsm_state *s)
{
	struct record *r;
	struct fsm hack;

	r = fsm_getopaque(&hack, s);
	return r->rec;
}

static int
leaf(FILE *f, const struct fsm *fsm, const struct fsm_state *state,
	const void *opaque)
{
	const struct record *r;

	(void) opaque;

	if (!fsm_isend(fsm, state)) {
		fprintf(f, "return -1;");
		return 0;
	}

	r = fsm_getopaque(fsm, state);
	assert(r != NULL);

	fprintf(f, "return 0x%u; /* %s */", r->id, r->rec);

	return 0;
}

int
main(int argc, char **argv)
{
	int progress = 0;
	FILE *fd = NULL;
	int odot = 0;
	int oc = 0;
	int c;

	opt.prefix            = NULL;
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.case_ranges       = 1;
	opt.opaque_string     = nl;

	while (c = getopt(argc, argv, "46f:l:Q"), c != -1) {
		switch (c) {
		case '4':
			ipv = 4;
			break;
		case '6':
			ipv = 6;
			break;
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

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		perror("fsm_new");
		return -1;
	}

	fsmstart = fsm_addstate(fsm);
	if (fsmstart == NULL) {
		perror("fsm_addstate");
		return -1;
	}
	fsm_setstart(fsm, fsmstart);

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

	while ((len = getline(&buf, &cap, fd)) > 0) {
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

		handle_line(start, end, rec, len);

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
		if (fsm_minimise_opaque(r->fsm, carryopaque) == 0) {
			perror("fsm_minimise_opaque");
			exit(-1);
		}

		if (!fsm_union(fsm, r->fsm)) {
			perror("fsm_union");
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

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", tend - tstart);
		fprintf(stderr, "converting to DFA...");
		fflush(stderr);
		tstart = time(NULL);
	}

	struct fsm_determinise_cache *cache = NULL;
	if (fsm_determinise_cacheopaque(fsm, carryopaque, &cache) == 0) {
		perror("fsm_determinise_cacheopaque");
		exit(-1);
	}
	fsm_determinise_freecache(fsm, cache);

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", tend - tstart);
	}

	if (oc) {
		static const char *cp = "c";
		/* XXX: This should be possible without private out.h. */
		fsm_out_cfrag(fsm, stdout, cp, leaf, NULL);
	} else if (odot) {
		fsm_print(fsm, stdout, FSM_OUT_DOT);
	}
}
