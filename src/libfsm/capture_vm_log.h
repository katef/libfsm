/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CAPTURE_VM_LOG_H
#define CAPTURE_VM_LOG_H

#include <stdio.h>

#define LOG_CAPVM (1+0)
#define LOG(LEVEL, ...)							\
	do {								\
		if ((LEVEL) <= LOG_CAPVM) {				\
			fprintf(stderr, __VA_ARGS__);			\
		}							\
	} while(0)


#endif
