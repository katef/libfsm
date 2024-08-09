/*
 * Copyright 2023 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/u64bitset.h>

#include <assert.h>

#include "internal.h"
#include "capture.h"

#define LOG_NAIVE_MIN 0

static fsm_state_t
find_dst(const struct fsm *fsm,
    fsm_state_t from, unsigned char label)
{
	const size_t state_count = fsm_countstates(fsm);
	if (from == state_count) {
		goto dead_state;
	}
	assert(from < state_count);

	struct fsm_edge e;
	if (edge_set_find(fsm->states[from].edges, label, &e)) {
		return e.state;
	}

	/* edge not found for label */
dead_state:
	return state_count;	/* dead state */
}

struct fsm *
fsm_minimise_test_oracle(const struct fsm *fsm)
{
	assert(fsm != NULL);
	if (fsm_capture_has_captures(fsm)) {
		assert(!"cannot be used with captures");
		return NULL;
	}

	const size_t state_count = fsm_countstates(fsm);
	{
		fsm_state_t start;
		if (state_count == 0 && !fsm_getstart(fsm, &start)) {
			return fsm_clone(fsm);
		}
	}

	struct fsm *res = NULL;

	/* Fixpoint-based algorithm for calculating equivalent states
	 * in a DFA, from pages 68-69 ("A minimization algorithm") of
	 * _Introduction to Automata Theory, Languages, and Computation_
	 * by Hopcroft & Ullman.
	 *
	 * Allocate an NxN bit table, used to track which states have
	 * been proven distinguishable.
	 *
	 * For two states A and B, only use the half of the table where
	 * A <= B, to avoid redundantly comparing B with A. In other
	 * words, only the bits below the diagonal are actually used.
	 *
	 * An extra state is added to internally represent a complete
	 * graph -- for any state that does not have a particular label
	 * used elsewhere, treat it as if it has that label leading to
	 * an extra dead state, which is non-accepting and only has
	 * edges leading to itself.
	 *
	 * To start, mark all pairs of states in the table where one
	 * state is an end state and the other is not. This indicates
	 * that they are distinguishable by their result. Then, using
	 * the Myhill-Nerode theorem, mark all pairs of states where
	 * following the same label leads each to a pair of states that
	 * have already been marked (and are thus transitively
	 * distinguishable). Repeat until no further progress can be
	 * made. Any pairs of states still unmarked can be combined into
	 * a single state, because no possible input leads them to
	 * observably distinguishable results.
	 *
	 * This method scales poorly as the number of states increases
	 * (since it's comparing everything against everything else over
	 * and over, for every label present), but it's straightforward
	 * to implement and to check its results by hand, so it works
	 * well as a test oracle. */
	const fsm_state_t dead_state = state_count;
	const size_t table_states = (state_count + 1 /* dead state */);
	const size_t row_words = u64bitset_words(table_states);

	uint64_t *table = NULL;
	fsm_state_t *tmp_map = NULL;
	fsm_state_t *mapping = NULL;

	/* endid_group_assignments[X] = Y: state X is in endid group Y
	 * endid_group_leaders[X] = Y: see end state Y for endid group X */
	unsigned *endid_group_assignments = NULL;
	size_t endid_group_count = 1; /* group 0 is the empty set */
	unsigned *endid_group_leaders = NULL;
	fsm_end_id_t *ids_a = NULL;
	fsm_end_id_t *ids_b = NULL;

	table = calloc(row_words * table_states, sizeof(table[0]));
	if (table == NULL) { goto cleanup; }

	tmp_map = malloc(state_count * sizeof(tmp_map[0]));
	if (tmp_map == NULL) { goto cleanup; }

	mapping = malloc(state_count * sizeof(mapping[0]));
	if (mapping == NULL) { goto cleanup; }

	endid_group_assignments = calloc(state_count, sizeof(tmp_map[0]));
	if (endid_group_assignments == NULL) { goto cleanup; }

	endid_group_leaders = calloc(state_count, sizeof(tmp_map[0]));
	if (endid_group_leaders == NULL) { goto cleanup; }

	/* macros for NxN bit table */
#define POS(X,Y) ((X*table_states) + Y)
#define MARK(X,Y)  u64bitset_set(table, POS(X,Y))
#define CLEAR(X,Y) u64bitset_clear(table, POS(X,Y))
#define CHECK(X,Y) u64bitset_get(table, POS(X,Y))

	size_t max_endid_count = 0;

	/* Mark all pairs of states where one is final and one is not.
	 * This includes the dead state. */
	for (size_t i = 0; i < table_states; i++) {
		if (i < state_count && fsm_isend(fsm, i)) {
			const size_t count = fsm_endid_count(fsm, i);
			if (count > max_endid_count) {
				max_endid_count = count;
			}
		}

		for (size_t j = 0; j < i; j++) {
			const bool end_i = i == dead_state
			    ? false : fsm_isend(fsm, i);
			const bool end_j = j == dead_state
			    ? false : fsm_isend(fsm, j);
			if (end_i != end_j) {
				if (LOG_NAIVE_MIN > 0) {
					fprintf(stderr,
					    "naive: setup mark %ld,%ld\n",
					    i, j);
				}
				MARK(i, j);
			}
		}
	}

	ids_a = malloc(max_endid_count * sizeof(ids_a[0]));
	if (ids_a == NULL) { goto cleanup; }
	ids_b = malloc(max_endid_count * sizeof(ids_b[0]));
	if (ids_b == NULL) { goto cleanup; }

	/* For every end state, check if it has endids. If not, assign it
	 * to endid group 0 (none). Otherwise, check if its endids match
	 * any of the other end states. If so, assign it to the same endid
	 * group, otherwise assign a new one and mark it as the leader. */
	for (size_t i = 0; i < state_count; i++) {
		if (!fsm_isend(fsm, i)) {
			endid_group_assignments[i] = 0; /* none */
			continue;
		}

		size_t count_a = fsm_endid_count(fsm, i);
		assert(count_a <= max_endid_count);
		if (count_a == 0) {
			continue;
		}

		int eres = fsm_endid_get(fsm, i, count_a, ids_a);
		assert(eres == 1);

		bool found = false;
		/* note: skipping eg 0 here since that's the empty set */
		for (size_t eg_i = 1; eg_i < endid_group_count; eg_i++) {
			size_t count_b = fsm_endid_count(fsm, endid_group_leaders[eg_i]);
			if (count_b != count_a) {
				continue;
			}

			assert(count_b > 0);
			assert(count_b <= max_endid_count);
			eres = fsm_endid_get(fsm, endid_group_leaders[eg_i],
			    count_b, ids_b);
			assert(eres == 1);

			if (0 == memcmp(ids_a, ids_b, count_a * sizeof(ids_a[0]))) {
				found = true;
				endid_group_assignments[i] = eg_i;
				break;
			}
		}

		if (!found) {
			endid_group_assignments[i] = endid_group_count;
			endid_group_leaders[endid_group_count] = i;
			endid_group_count++;
		}
	}

	/* Any end states that have not been assigned the same endid
	 * group must be distinguishable. */
	for (size_t i = 0; i < state_count; i++) {
		if (fsm_isend(fsm, i)) {
			const unsigned i_group = endid_group_assignments[i];
			for (size_t j = 0; j < i; j++) {
				if (fsm_isend(fsm, j)) {
					const unsigned j_group = endid_group_assignments[j];
					if (i_group != j_group) {
						MARK(i, j);
					}
				}
			}
		}
	}

	bool changed;
	do {			/* run until fixpoint */
		changed = false;
		/* For every combination of states that are still
		 * considered potentially indistinguishable, check
		 * whether the current label leads both to a pair of
		 * distinguishable states. If so, then they are
		 * transitively distinguishable, so mark them and
		 * set changed to note that another iteration may be
		 * necessary. */
		if (LOG_NAIVE_MIN > 1) {
			fprintf(stderr, "==== table\n");
			for (size_t i = 0; i < table_states; i++) {
				fprintf(stderr, "%-6zu%s ", i, "|");
				for (size_t j = 0; j < i; j++) {
					fprintf(stderr, "%-8d",
					    CHECK(i, j) != 0);
				}
				fprintf(stderr, "\n");
			}

			fprintf(stderr, "%6s", " ");
			for (size_t i = 0; i < table_states; i++) {
				fprintf(stderr, "--------");
			}
			fprintf(stderr, "\n%8s", " ");
			for (size_t i = 0; i < table_states; i++) {
				fprintf(stderr, "%-8zu", i);
			}
			fprintf(stderr, "\n");

			fprintf(stderr, "====\n");
		}

		for (size_t i = 0; i < table_states; i++) {
			for (size_t j = 0; j < i; j++) {
				if (LOG_NAIVE_MIN > 0) {
					fprintf(stderr, "naive: fix [%ld,%ld]: %d\n",
					    i, j, 0 != CHECK(i,j));
				}

				if (CHECK(i, j)) { continue; }
				for (size_t label_i = 0; label_i < FSM_SIGMA_COUNT; label_i++) {
					const unsigned char label = (unsigned char)label_i;
					const fsm_state_t to_i = find_dst(fsm, i, label);
					const fsm_state_t to_j = find_dst(fsm, j, label);
					bool tbval;
					if (to_i >= to_j) {
						tbval = 0 != CHECK(to_i, to_j);
					} else {
						tbval = 0 != CHECK(to_j, to_i);
					}

					if (LOG_NAIVE_MIN > 0) {
						fprintf(stderr,
						    "naive: label 0x%x, i %ld -> %d, j %ld -> %d => tbs %d\n",
						    label, i, to_i, j, to_j, tbval);
					}

					if (tbval) {
						if (LOG_NAIVE_MIN > 0) {
							fprintf(stderr,
							    "naive: MARKING %ld,%ld\n",
							    i, j);
						}

						MARK(i, j);
						changed = true;
						break;
					}
				}
			}
		}
	} while (changed);

#define NO_ID (fsm_state_t)-1

	/* Initialize to (NO_ID) */
	for (size_t i = 0; i < state_count; i++) {
		tmp_map[i] = NO_ID;
	}

	/* For any state that cannot be proven distinguishable from an
	 * earlier state, map it back to the earlier one. */
	for (size_t i = 1; i < state_count; i++) {
		for (size_t j = 0; j < i; j++) {
			if (!CHECK(i, j)) {
				tmp_map[i] = j;
				if (LOG_NAIVE_MIN > 0) {
					fprintf(stderr, "naive: merging %ld with %ld\n",
					    i, j);
				}
			}
		}
	}

	/* Set IDs for the states, mapping some back to earlier states
	 * (combining them).
	 *
	 * Note: this does not count the dead state. */
	fsm_state_t new_id = 0;
	for (size_t i = 0; i < state_count; i++) {
		if (tmp_map[i] != NO_ID) {
			fsm_state_t prev = mapping[tmp_map[i]];

			mapping[i] = prev;
		} else {
			mapping[i] = new_id;
			new_id++;
		}
	}

	if (LOG_NAIVE_MIN > 1) {
		for (size_t i = 0; i < state_count; i++) {
			fprintf(stderr, "mapping[%zu]: %d\n", i, mapping[i]);
		}
	}

	/* Make a copy of the FSM and consolidate it with the minimal mapping. */
	res = fsm_consolidate(fsm, mapping, state_count);
	if (res == NULL) { goto cleanup; }

	free(table);
	free(tmp_map);
	free(mapping);
	free(endid_group_assignments);
	free(endid_group_leaders);
	free(ids_a);
	free(ids_b);

	return res;

cleanup:
	if (table != NULL) { free(table); }
	if (tmp_map != NULL) { free(tmp_map); }
	if (mapping != NULL) { free(mapping); }
	if (endid_group_assignments != NULL) { free(endid_group_assignments); }
	if (endid_group_leaders != NULL) { free(endid_group_leaders); }
	if (ids_a != NULL) { free(ids_a); }
	if (ids_b != NULL) { free(ids_b); }
	if (res != NULL) { fsm_free(res); }
	return NULL;
}
