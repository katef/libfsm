/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PV_H
#define PV_H

/* Permutation vector construction. */

/* For offsetof. */
#include <stddef.h>

#include <adt/alloc.h>

#define PV_H_LOG 0

#if PV_H_LOG
#include <stdio.h>
#endif

/* Use counting sort to construct a permutation vector -- this is an
 * array of offsets into in[N] such that in[pv[0..N]] would give the
 * values of in[] in ascending order (but don't actually rearrange it,
 * just get the offsets). This is O(n). */
static __inline__ unsigned *
permutation_vector_with_size_and_offset(const struct fsm_alloc *alloc,
    size_t length, size_t max_value, void *in, size_t struct_size, size_t offset)
{
	unsigned *out = NULL;
	unsigned *counts = NULL;
	size_t i;
	const uint8_t *raw8 = in;

	/* offset must be a multiple of sizeof(unsigned) and
	 * fit within struct_size. */
	assert((offset & (sizeof(unsigned) - 1)) == 0);
	assert(struct_size >= sizeof(unsigned));

	out = f_malloc(alloc,
	    /* appease Electric Fence, which flags zero-byte allocations. */
	    (length == 0 ? 1 : length) * sizeof(*out));

	if (out == NULL) {
		goto cleanup;
	}
	counts = f_calloc(alloc, max_value + 1, sizeof(*out));
	if (counts == NULL) {
		goto cleanup;
	}

	/* Count each distinct value */
	for (i = 0; i < length; i++) {
		const unsigned *vp = (const unsigned *)&raw8[i*struct_size + offset];
		const unsigned value = *vp;
		counts[value]++;
#if PV_H_LOG
		fprintf(stderr, "pvwsao: in[%zu] -> %u (count %u)\n", i, value, counts[value]);
#endif
	}

	/* Convert to cumulative counts, so counts[v] stores the upper
	 * bound for where sorting would place each distinct value. */
	for (i = 1; i <= max_value; i++) {
		counts[i] += counts[i - 1];
	}

	/* Sweep backwards through the input array, placing each value
	 * according to the cumulative count. Decrement the count so
	 * progressively earlier instances of the same value will
	 * receive earlier offsets in out[]. */
	for (i = 0; i < length; i++) {
	        const unsigned pos = length - i - 1;
		const unsigned *vp = (const unsigned *)&raw8[pos*struct_size + offset];
		const unsigned value = *vp;
		const unsigned count = --counts[value];
		out[count] = pos;
#if PV_H_LOG
		fprintf(stderr, "pvwsao: out[%u] <- %u (value %u)\n", count, pos, value);
#endif
	}

	f_free(alloc, counts);
	return out;

cleanup:
	if (out != NULL) {
		f_free(alloc, out);
	}
	if (counts != NULL) {
		f_free(alloc, counts);
	}
	return NULL;
}

/* Build a permutation vector from a bare array of unsigned ints. */
static __inline__ unsigned *
permutation_vector(const struct fsm_alloc *alloc,
    size_t length, size_t max_value, unsigned *in)
{
	return permutation_vector_with_size_and_offset(alloc,
	    length, max_value, in, sizeof(in[0]), 0);
}

#endif
