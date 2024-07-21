#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <errno.h>

#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#include "endids_utils.h"

// Number of end ids needs to exceed DEF_BUCKET_ID_COUNT (currently 16)
// to exercise reallocs in the ID list
//
// Each pattern gets 5 end ids
// Note: These are unanchored and therefore partially overlap
static const char *patterns[] = {
	"abc",		//  1-5
	"def",		//  6-10
	"abc.def",	// 11-15
	"abc_def",	// 16-20
	"foo",		// 21-25
	"bar",		// 26-30
};

enum {
	NUM_PATTERNS           = sizeof patterns / sizeof patterns[0],
	NUM_ENDIDS_PER_PATTERN = 5,
	NUM_ENDIDS_TOTAL       = NUM_PATTERNS * NUM_ENDIDS_PER_PATTERN
};

struct example {
	size_t pattern;
	char example[256];
};

struct example_list {
	struct example *examples;
	size_t len;
	size_t cap;
};

enum { DEFAULT_EXAMPLE_CAP = 16 };

void init_examples(struct example_list *l)
{
	l->examples = calloc(DEFAULT_EXAMPLE_CAP, sizeof l->examples[0]);
	assert(l->examples != NULL);

	l->len = 0;
	l->cap = DEFAULT_EXAMPLE_CAP;
}

void finalize_examples(struct example_list *l)
{
    free(l->examples);
    l->examples = NULL;
    l->len = l->cap = 0;
}

struct example *add_example(struct example_list *l)
{
	if (l->len == l->cap) {
		struct example *new_list;
		size_t new_cap;

		new_cap = 2*l->cap;
		new_list = realloc(l->examples, new_cap * sizeof new_list[0]);
		assert(new_list != NULL);

		l->examples = new_list;
		l->cap = new_cap;
	}

	assert(l->len < l->cap);

	return &l->examples[l->len++];
}

size_t generate_examples(struct example_list *l, const struct fsm *fsm, size_t pattern)
{
	unsigned i, n;

	n = fsm_countstates(fsm);

	size_t first_index = SIZE_MAX;

	for (i=0; i < n; i++) {
		if (fsm_isend(fsm, i)) {
			int ret;
			struct example *ex;

			if (first_index == SIZE_MAX) {
				first_index = l->len;
			}

			ex = add_example(l);

			ex->pattern = pattern;
			ret = fsm_example(fsm, i, &ex->example[0], sizeof ex->example);
			assert(ret > 0);

			ret = match_string(fsm, ex->example, NULL, NULL, NULL);
			if (ret != 1) {
				fprintf(stderr, "example \"%s\" for pattern /%s/ did not match pattern!\n", ex->example, patterns[pattern]);
				abort();
			}

		}
	}

	return (first_index != SIZE_MAX) ? first_index : 0;
}

/* test that ids correctly propagate through union, determinise, and minimise */
int main(void)
{
	struct fsm *fsm;
	fsm_end_id_t all_ids[NUM_PATTERNS];
	size_t nstates, state_ind;
	size_t i;
	int ret;
	
	struct example_list example_list;

	memset(all_ids, 0, sizeof all_ids);

	init_examples(&example_list);

	size_t pattern_example[NUM_PATTERNS];

	fsm = NULL;
	for (i=0; i < NUM_PATTERNS; i++) {
		struct fsm *new;
		int j, ret;

		char buf[256];
		char *bufp;

		strncpy(&buf[0], patterns[i], sizeof buf);

		bufp = &buf[0];
		new = re_comp(RE_NATIVE, fsm_sgetc, &bufp, NULL, 0, NULL);
		assert(new != NULL);

		ret = fsm_determinise(new);
		assert(ret == 1);

		ret = fsm_minimise(new);
		assert(ret == 1);

		pattern_example[i] = generate_examples(&example_list, new, i);

		for (j=0; j < 5; j++) {
			ret = fsm_setendid(new, (fsm_end_id_t) (NUM_ENDIDS_PER_PATTERN*i+j+1));
			assert(ret == 1);
		}

		if (fsm == NULL) {
			fsm = new;
		} else {
			fsm = fsm_union(fsm, new, NULL);
			assert(fsm != NULL);
		}
	}

	ret = fsm_determinise(fsm);
	assert(ret != 0);

	// find end states, make sure we have multiple end states and they each have ids
	nstates = fsm_countstates(fsm);

	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			int tested_pattern[NUM_PATTERNS];
			fsm_end_id_t ids[NUM_ENDIDS_TOTAL];
			size_t count, j;
			int ret;

			memset(&ids[0], 0, sizeof ids);

			count = fsm_endid_count(fsm, state_ind);
			assert(count > 0 && count <= sizeof ids/sizeof ids[0]);

			ret = fsm_endid_get(fsm, state_ind, count, &ids[0]);
			assert(ret == 1);

			memset(&tested_pattern[0], 0, sizeof tested_pattern);

			for (j=0; j < count; j++) {
				size_t k, pattern_index;

				pattern_index = (ids[j] - 1)/NUM_ENDIDS_PER_PATTERN;
				assert(pattern_index < NUM_PATTERNS);

				if (tested_pattern[pattern_index]) {
					continue;
				}

				tested_pattern[pattern_index] = 1;

				for (k=pattern_example[pattern_index]; k < example_list.len; k++) {
					const struct example *ex = &example_list.examples[k];
					if (ex->pattern != pattern_index) {
						break;
					}

					ret = match_string(fsm, ex->example, NULL, NULL, NULL);
					if (ret) {
#if 0
						printf("end state %zu (end id %u) matches example \"%s\" from pattern /%s/\n",
							state_ind, ids[j], ex->example, patterns[pattern_index]);
#endif /* 0 */
					} else {
						printf("end state %zu (end id %u) does NOT match example \"%s\" from pattern /%s/\n",
							state_ind, ids[j], ex->example, patterns[pattern_index]);
						abort();
					}
				}
			}
		}
	}

	ret = fsm_minimise(fsm);
	assert(ret != 0);

	/* fsm_minimise should not collapse all the end states to a
	 * single end state, because they have distinct ids. */
	assert( fsm_count(fsm, fsm_isend) > 1);

	nstates = fsm_countstates(fsm);
	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			fsm_end_id_t ids[NUM_ENDIDS_TOTAL];
			size_t count;
			int ret;

			memset(&ids[0], 0, sizeof ids);

			count = fsm_endid_count(fsm, state_ind);
			assert(count <= NUM_ENDIDS_TOTAL);

			ret = fsm_endid_get(fsm, state_ind, count, &ids[0]);
			assert(ret == 1);
		}
	}

        fsm_free(fsm);
        finalize_examples(&example_list);

	printf("ok\n");

	return EXIT_SUCCESS;
}


