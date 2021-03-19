/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef LIBFSM_CHECK_H
#define LIBFSM_CHECK_H

#if defined(__clang__)
/* Newer versions of clang's UBSan emit warnings about *all* unsigned
 * integer overflows. While they are defined behavior, overflow can
 * cause bugs. This macro ignores them for a particular function.
 * Overflow/rollover is expected in when hashing, for example. */
#define SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()		\
	__attribute__((no_sanitize("integer")))
#else
#define SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
#endif

#endif
