/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <adt/idmap.h>

#define DEF_LIMIT 10
#define DEF_SEED 0

/* Thes numbers were chose to get a reasonable variety,
 * but also some duplicated values as the input grows. */
#define MAX_GEN_VALUES 23
#define ID_MASK ((1 << 9) - 1)
#define VALUE_MASK ((1 << 10) - 1)

static void
dump_cb(fsm_state_t state_id, unsigned value, void *opaque)
{
	/* fprintf(stderr, " -- state %d, value %u\n", state_id, value); */
	assert(state_id <= ID_MASK);
	assert(value <= VALUE_MASK);
	(void)opaque;
}

static int
cmp_u(const void *pa, const void *pb)
{
	const unsigned a = *(unsigned *)pa;
	const unsigned b = *(unsigned *)pb;
	return a < b ? -1 : a > b ? 1 : 0;
}

int main(int argc, char **argv) {
	const size_t limit = (argc > 1 ? atoi(argv[1]) : DEF_LIMIT);
	const unsigned seed = (argc > 2 ? atoi(argv[2]) : DEF_SEED);

	(void)argc;
	(void)argv;
	struct idmap *m = idmap_new(NULL);

	srandom(seed);

	/* Fill the table with random data */
	for (size_t id_i = 0; id_i < limit; id_i++) {
		const fsm_state_t id = (fsm_state_t)(random() & ID_MASK);
		const size_t value_count = random() % MAX_GEN_VALUES;

		for (size_t v_i = 0; v_i < value_count; v_i++) {
			const unsigned v = random() & VALUE_MASK;
			if (!idmap_set(m, id, v)) {
				assert(!"failed to set");
			}
		}
	}

	idmap_iter(m, dump_cb, NULL);

	srandom(seed);

	size_t got_buf_ceil = MAX_GEN_VALUES;
	unsigned *got_buf = malloc(got_buf_ceil * sizeof(got_buf[0]));
	assert(got_buf != NULL);

	/* Reset the PRNG and read back the same data. */
	for (size_t id_i = 0; id_i < limit; id_i++) {
		const fsm_state_t id = (fsm_state_t)(random() & ID_MASK);
		const size_t generated_value_count = random() % MAX_GEN_VALUES;

		/* Note: This can occasionally differ from
		 * generated_value_count, because the same id or values
		 * may have been generated more than once. As long as
		 * all the values match, it's fine. */
		const size_t value_count = idmap_get_value_count(m, id);

		if (value_count > got_buf_ceil) {
			size_t nceil = got_buf_ceil;
			while (nceil <= value_count) {
				nceil *= 2;
			}
			free(got_buf);
			got_buf = malloc(nceil * sizeof(got_buf[0]));
			assert(got_buf != NULL);
			got_buf_ceil = nceil;
		}

		size_t written;
		if (!idmap_get(m, id,
			got_buf_ceil * sizeof(got_buf[0]), got_buf,
			&written)) {
			assert(!"failed to get");
		}
		assert(written == value_count);

		unsigned gen_buf[MAX_GEN_VALUES];

		for (size_t v_i = 0; v_i < generated_value_count; v_i++) {
			const unsigned v = random() & VALUE_MASK;
			gen_buf[v_i] = v;
		}
		qsort(gen_buf, generated_value_count, sizeof(gen_buf[0]), cmp_u);

		/* Every generated value should appear in the buffer.
		 * There may be more in the buffer; ignore them. */
		size_t v_i = 0;
		for (size_t gen_i = 0; gen_i < generated_value_count; gen_i++) {
			int found = 0;
			const unsigned gv = gen_buf[gen_i];
			assert(value_count <= got_buf_ceil);
			/* got_buf should be sorted, so we can pick up where we left off */
			while (v_i < value_count) {
				if (gv == got_buf[v_i]) {
					/* Intentionally don't increment v_i on match,
					 * because gen_buf can repeat values. */
					found = 1;
					break;
				}
				v_i++;
			}
			if (!found) {
				fprintf(stderr, "NOT FOUND: state %d -- value: %u\n",
				    id, gv);
				return EXIT_FAILURE;
			}
		}
	}

	free(got_buf);
	idmap_free(m);
	return EXIT_SUCCESS;
}
