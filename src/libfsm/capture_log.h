/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CAPTURE_LOG_H
#define CAPTURE_LOG_H

/* Log levels */
#define LOG_CAPTURE 0
#define LOG_CAPTURE_COMBINING_ANALYSIS 0
#define LOG_EVAL 0
#define LOG_APPEND_ACTION 0
#define LOG_PRINT_FSM 0
#define LOG_MARK_PATH 0

#include <stdio.h>

#define LOG(LEVEL, ...)							\
	do {								\
		if ((LEVEL) <= LOG_CAPTURE) {				\
			fprintf(stderr, __VA_ARGS__);			\
		}							\
	} while(0)

#endif
