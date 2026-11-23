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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include "iprange.h"

#define VALUE_SEP ','

static void
usage(void)
{
	fprintf(stderr, "iprange -[46] [-s] [-f <file>] -l fmt\n"
	    "\t-4\t\tIPv4\n"
	    "\t-6\t\tIPv6\n"
	    "\t-s\t\tStream pre-sorted, conflict-free input\n"
	    "\t-f <file>\tUse <file> as input\n"
	    "\t-l fmt\t\tOutput format (c, dot, fsm)\n");
	exit(EXIT_FAILURE);
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
report_conflict(const char * const *labels, const unsigned *ids,
	size_t count, const unsigned char *example_ip, void *opaque)
{
	const struct iprange *ipr = opaque;
	char ipbuf[INET6_ADDRSTRLEN];
	int af;

	(void) ids;

	fprintf(stderr, "ambiguous matches for ");

	for (size_t i = 0; i < count; i++) {
		fprintf(stderr, "%s", labels[i] != NULL ? labels[i] : "?");
		if (i + 1 < count) {
			fprintf(stderr, ", ");
		}
	}

	if (example_ip != NULL && ipr != NULL) {
		af = (iprange_get_family(ipr) == IPRANGE_IPV4)
			? AF_INET : AF_INET6;
		if (inet_ntop(af, example_ip, ipbuf, sizeof ipbuf) != NULL) {
			fprintf(stderr, "; for example on input '%s'", ipbuf);
		}
	}

	fprintf(stderr, "\n");
}

static int
accept_dot(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	const struct iprange *ipr = hook_opaque;
	fsm_state_t s;

	assert(f != NULL);
	assert(state_metadata != NULL);

	s = * (fsm_state_t *) lang_opaque;

	fprintf(f, "label = <");

	if (!opt->anonymous_states) {
		fprintf(f, "%u", s);

		if (state_metadata->end_id_count > 0) {
			fprintf(f, "<BR/>");
		}
	}

	for (size_t i = 0; i < state_metadata->end_id_count; i++) {
		const char *label = iprange_label(ipr, state_metadata->end_ids[i]);

		fprintf(f, "%s", label != NULL ? label : "?"); /* XXX: escape */

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
	const struct iprange *ipr = hook_opaque;
	unsigned id;
	const char *label;

	assert(f != NULL);
	assert(state_metadata != NULL);
	assert(state_metadata->end_id_count == 1);

	(void) lang_opaque;

	id = state_metadata->end_ids[0];
	label = iprange_label(ipr, id);

	fprintf(f, "return 0x%x;", id);
	if (opt->comments && label != NULL) {
		fprintf(f, " /* %s */", label);
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
	enum iprange_family family = IPRANGE_IPV4;
	enum fsm_print_lang lang = FSM_PRINT_NONE;
	struct fsm_options opt;
	struct fsm_hooks hooks;
	struct iprange *ipr;
	struct fsm *fsm = NULL;
	char *buf = NULL;
	size_t cap = 0;
	ssize_t len;
	int streaming = 0;
	int progress = 0;
	FILE *fd = NULL;
	int c;
	enum iprange_err err;

	memset(&opt, 0, sizeof opt);
	memset(&hooks, 0, sizeof hooks);

	opt.ambig             = AMBIG_ERROR;
	opt.prefix            = NULL;
	opt.always_hex        = 1;
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.case_ranges       = 1;

	while (c = getopt(argc, argv, "46f:l:sQ"), c != -1) {
		switch (c) {
		case '4': family = IPRANGE_IPV4; break;
		case '6': family = IPRANGE_IPV6; break;
		case 's': streaming = 1; break;

		case 'l':
			if (strcmp(optarg, "dot") == 0) {
				lang = FSM_PRINT_DOT;
			} else if (strcmp(optarg, "c") == 0) {
				lang = FSM_PRINT_C;
			} else if (strcmp(optarg, "fsm") == 0) {
				lang = FSM_PRINT_FSM;
			} else {
				fprintf(stderr, "Invalid language '%s'\n", optarg);
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

	if (lang == FSM_PRINT_NONE) {
		usage();
	}

	if (fd == NULL) {
		fd = stdin;
	}

	ipr = iprange_new(family);
	if (ipr == NULL) {
		perror("iprange_new");
		if (fd != stdin) {
			fclose(fd);
		}
		return EXIT_FAILURE;
	}

	if (streaming) {
		err = iprange_enable_streaming(ipr);
		if (err != IPRANGE_OK) {
			fprintf(stderr, "iprange_enable_streaming: %s\n",
				iprange_strerror(err));
			goto fail;
		}
	}

	unsigned line = 0;
	time_t tstart = 0, tend;
	if (progress) {
		tstart = time(NULL);
		fprintf(stderr, "Reading lines... ");
		fflush(stderr);
	}

	while (len = getline(&buf, &cap, fd), len > 0) {
		char *start = buf;
		char *end = memchr(start, VALUE_SEP, (size_t) len);

		if (end == NULL) {
			fprintf(stderr, "Invalid data: %s\n", buf);
			goto fail;
		}
		*end = '\0';
		end++;
		len -= end - start;

		char *rec = memchr(end, VALUE_SEP, (size_t) len);
		if (rec == NULL) {
			fprintf(stderr, "Invalid data: %s\n", buf);
			goto fail;
		}
		*rec = '\0';
		rec++;

		rec[strcspn(rec, "\r\n")] = '\0';

		err = iprange_add_str(ipr, start, end, rec, NULL);
		if (err != IPRANGE_OK) {
			fprintf(stderr, "Line %u (%s-%s): %s\n",
				line + 1, start, end, iprange_strerror(err));
			goto fail;
		}

		if (progress) {
			tend = time(NULL);
			if (important(line)) {
				fprintf(stderr, "l%u [%lds] ", line,
					(long) (tend - tstart));
			}
			tstart = tend;
		}
		line++;
	}

	free(buf);
	buf = NULL;
	if (fd != stdin) {
		fclose(fd);
		fd = NULL;
	}

	if (progress) {
		fprintf(stderr, "\ncompiling %u records...",
			iprange_record_count(ipr));
		fflush(stderr);
		tstart = time(NULL);
	}

	err = iprange_compile(ipr, &fsm, report_conflict, ipr);
	if (err != IPRANGE_OK) {
		if (err != IPRANGE_ERR_CONFLICT) {
			fprintf(stderr, "iprange_compile: %s\n",
				iprange_strerror(err));
		}
		goto fail;
	}

	if (progress) {
		tend = time(NULL);
		fprintf(stderr, " done [%lds]\n", (long) (tend - tstart));
	}

	hooks.hook_opaque = ipr;

	if (lang == FSM_PRINT_C) {
		opt.fragment = 1;
		opt.comments = 1;
		hooks.accept = accept_c;
		hooks.reject = reject_c;
	} else if (lang == FSM_PRINT_DOT) {
		hooks.accept = accept_dot;
	}

	if (fsm_print(stdout, fsm, &opt, &hooks, lang) != 0) {
		perror("fsm_print");
		goto fail;
	}

	fsm_free(fsm);
	iprange_free(ipr);
	return EXIT_SUCCESS;

fail:
	free(buf);
	if (fd != NULL && fd != stdin) {
		fclose(fd);
	}
	if (fsm != NULL) {
		fsm_free(fsm);
	}
	iprange_free(ipr);
	return EXIT_FAILURE;
}
