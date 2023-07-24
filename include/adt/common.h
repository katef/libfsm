#ifndef LIBFSM_COMMON_H
#define LIBFSM_COMMON_H

/* Internal definitions shared between adt, libfsm, and libre */

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

/* If set to non-zero, do extra intensive integrity checks, often inside
 * some inner loops, which are far too expensive for production. */
#ifndef EXPENSIVE_CHECKS
#define EXPENSIVE_CHECKS 0
#endif

#if EXPENSIVE_CHECKS && NDEBUG
#error NDEBUG with EXPENSIVE_CHECKS
#endif

/* If set to non-zero, this build should reject inputs as unsupported
 * that lead to uninteresting failures while fuzzing -- for example,
 * it's not surprising that `(some regex){1000000}` can hit the
 * allocator limit, but once the fuzzer finds that it will produce
 * variants of the failure over and over. */
#ifndef BUILD_FOR_FUZZER
#define BUILD_FOR_FUZZER 0
#endif

/* If non-zero, expand the timer macros defined below, otherwise
 * they compile away. */
#ifndef TRACK_TIMES
#define TRACK_TIMES 0
#endif

#if EXPENSIVE_CHECKS && TRACK_TIMES
#error benchmarking with EXPENSIVE_CHECKS
#endif

#if TRACK_TIMES
#include <sys/time.h>
#define TIMER_LOG_THRESHOLD 100

#define INIT_TIMERS() struct timeval pre, post
#define INIT_TIMERS_NAMED(PREFIX) struct timeval PREFIX ## _pre, PREFIX ## _post
#define TIME(T)								\
	if (gettimeofday(T, NULL) == -1) { assert(!"gettimeofday"); }
#define DIFF_MSEC(LABEL, PRE, POST, ACCUM)				\
	do {								\
		size_t *accum = ACCUM;					\
		const size_t diff_usec =				\
		    (1000000*(POST.tv_sec - PRE.tv_sec)			\
			+ (POST.tv_usec - PRE.tv_usec));		\
		const size_t diff_msec = diff_usec/1000;		\
		if (diff_msec >= TIMER_LOG_THRESHOLD			\
		    || TRACK_TIMES > 1) {				\
			fprintf(stderr, "%s: %zu msec%s\n", LABEL,	\
			    diff_msec,					\
			    diff_msec >= 100 ? " #### OVER 100" : "");	\
		}							\
		if (accum != NULL) {					\
			(*accum) += diff_usec;				\
		}							\
	} while(0)
#define DIFF_MSEC_ALWAYS(LABEL, PRE, POST, ACCUM)			\
	do {								\
		size_t *accum = ACCUM;					\
		const size_t diff_usec =				\
		    (1000000*(POST.tv_sec - PRE.tv_sec)			\
			+ (POST.tv_usec - PRE.tv_usec));		\
		const size_t diff_msec = diff_usec/1000;		\
		fprintf(stderr, "%s: %zu msec%s\n", LABEL, diff_msec,	\
		    diff_msec >= 100 ? " #### OVER 100" : "");		\
		if (accum != NULL) {					\
			(*accum) += diff_usec;				\
		}							\
	} while(0)

#define DIFF_USEC_ALWAYS(LABEL, PRE, POST, ACCUM)			\
	do {								\
		size_t *accum = ACCUM;					\
		const size_t diff_usec =				\
		    (1000000*(POST.tv_sec - PRE.tv_sec)			\
			+ (POST.tv_usec - PRE.tv_usec));		\
		fprintf(stderr, "%s: %zu usec\n", LABEL, diff_usec);	\
		if (accum != NULL) {					\
			(*accum) += diff_usec;				\
		}							\
	} while(0)

#else
#define INIT_TIMERS()
#define INIT_TIMERS_NAMED(PREFIX)
#define TIME(T)
#define DIFF_MSEC(A, B, C, D)
#define DIFF_MSEC_ALWAYS(A, B, C, D)
#define DIFF_USEC_ALWAYS(A, B, C, D)
#endif

#endif
