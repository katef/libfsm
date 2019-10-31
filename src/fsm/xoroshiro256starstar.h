#ifndef XOROSHIRO256STARSTAR_H
#define XOROSHIRO256STARSTAR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct prng_state {
	uint64_t s[4];
};

uint64_t prng_next(struct prng_state *st);

void prng_seed_array(struct prng_state *st, unsigned char bits[32]);
void prng_seed(struct prng_state *st, uint64_t x);

static inline double u64_to_f64(uint64_t x) {
	uint64_t v = ((uint64_t)0x3ff << 52) | (x >> 12);
	double d;
	memcpy(&d, &v, sizeof d);

	return d - 1.0;
}

static inline double prng_double(struct prng_state *st) {
	return u64_to_f64(prng_next(st));
}

#endif /* XOROSHIRO256STARSTAR_H */

