/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef BENCH_LIBFSM_H
#define BENCH_LIBFSM_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <err.h>
#include <errno.h>

#include <getopt.h>

#include <sys/time.h>

#include <fsm/bool.h>
#include <fsm/cost.h>
#include <fsm/fsm.h>
#include <fsm/out.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <re/re.h>

enum mode {
    MODE_DETERMINISE,
    MODE_MINIMISE,
    MODE_REVERSE,
    MODE_TRIM,
    MODE_COMPLEMENT,

    MODE_MATCH,
};

struct config {
    enum mode mode;
    enum re_dialect dialect;
    int verbosity;
    size_t iterations;
    bool keep_nfa;
    const char *re;
    const char *input;
};

#endif
