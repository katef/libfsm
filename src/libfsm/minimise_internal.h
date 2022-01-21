#ifndef MINIMISE_INTERNAL_H
#define MINIMISE_INTERNAL_H

#define NO_ID ((fsm_state_t)-1)

/* If set to non-zero, do extra intensive
 * integrity checks inside some inner loops. */
#define EXPENSIVE_INTEGRITY_CHECKS 0

#define DEF_INITIAL_COUNT_CEIL 8

struct min_env {
	const struct fsm *fsm;

	/* Counters, for benchmarking. */
	size_t iter;		/* passes through the main loop */
	size_t steps;		/* partition attempts */

	/* ID for a special dead state. Since the DFA may not be
	 * complete (with all states containing edges for every label in
	 * use), any labels not present lead to an internal dead state,
	 * which is non-final and only has edges to itself. */
	fsm_state_t dead_state;

	/* There are one or more equivalence classes (hereafter, ECs)
	 * initially, which represent all the end states and (if
	 * present) all the non-end states. If there are no end states,
	 * the graph is already empty, so this shouldn't run.
	 *
	 * ecs[N] contains the state ID for the first state in EC group
	 * N's linked list, then jump[ID] contains the next ID within
	 * that EC, or NO_ID for the end of the list. state_ecs[ID]
	 * contains the state's current EC group number.
	 *
	 * In other words, state_ecs[ID] = EC, and env.ecs[EC] = X,
	 * where either X is ID or following the linked list in
	 * env.jump[X] eventually leads to ID. Ordering within the
	 * list does not matter. */
	size_t ec_map_count;
	fsm_state_t *state_ecs;
	fsm_state_t *ecs;
	fsm_state_t *jump;

	/* How many ECs appear in ecs[]. This increases as more states
	 * are partitioned into independent ECs, but can never be larger
	 * than the number of states in the DFA input. */
	fsm_state_t ec_count;

	/* Any ECs from ecs[done_ec_offset] to the end are done -- they
	 * have less than two elements and can't be partitioned further,
	 * so it would only waste time to check them. If partitioning an
	 * EC creates a new EC with less than two elements, the offset
	 * is updated and/or the ECs are swapped so that the new EC is
	 * linked after the done offset. The first two ECs can start
	 * with less than two states (for example, if all states are
	 * final), so the loop body still needs to check and skip ECs
	 * with <2 states, but this will skip scanning the majority of
	 * new ECs.
	 *
	 * Skipping the ECs after done_ec_offset improves performance
	 * signifcantly, with far less complexity or overhead compared
	 * to the splitter queue management in Hopcroft's algorithm
	 * (which becomes more expensive as label_count increases). */
	fsm_state_t done_ec_offset;

	/* The set of labels that appear through the entire DFA. */
	const unsigned char *dfa_labels;
	size_t dfa_label_count;
};

/* An iterator, used to try partitioning on either:
 * - every label appearing throughout the entire DFA.
 * - every label appearing within a specific EC.
 *
 * While attempting to partition an EC, the states split into the new
 * ECs are counted. If either EC has less than SMALL_EC_THRESHOLD
 * states, a flag is set indicating that the iterator should collect the
 * labels that actually appear in the EC and just check those. This
 * significantly reduces the overhead of checking for possible
 * partitions in a large DFA label set when the EC's states only have a
 * small number of edges. */
struct min_label_iterator {
	unsigned i;
	unsigned limit;
	unsigned char has_labels_for_EC;
	unsigned char labels[256];
};

/* Note that the ordering here is significant -- the
 * not-final EC may be skipped, so INIT_EC_NOT_FINAL
 * must be first. */
enum init_ec { INIT_EC_NOT_FINAL, INIT_EC_FINAL };

/* We only know the state count for an EC immediately after an
 * attempted partition. When the ECs have less than SMALL_EC_THRESHOLD
 * states, then set the most significant bit to mark that the EC is
 * small. In that case, it's probably more efficient to do an extra
 * pass and collect which labels are actually used, then just check
 * for possible partitions on those, rather than trying every label
 * used in the entire DFA.
 *
 * Setting SMALL_EC_THRESHOLD to 0 disables the optimization. */
#define SMALL_EC_FLAG (UINT_MAX ^ (UINT_MAX >> 1))
#define SMALL_EC_THRESHOLD 16	/* a guess -- needs experimentation */
#define DFA_LABELS_THRESHOLD 4
#define SET_SMALL_EC_FLAG(STATE_ID) (STATE_ID | SMALL_EC_FLAG)
#define MASK_EC_HEAD(EC) (EC &~ SMALL_EC_FLAG)

static void
collect_labels(const struct fsm *fsm,
    unsigned char *labels, size_t *label_count);

static int
build_minimised_mapping(const struct fsm *fsm,
    const unsigned char *dfa_labels, size_t dfa_label_count,
    const unsigned *shortest_end_distance,
    fsm_state_t *mapping, size_t *minimized_state_count);

static void
dump_ecs(FILE *f, const struct min_env *env);

static int
populate_initial_ecs(struct min_env *env, const struct fsm *fsm,
	const unsigned *shortest_end_distance);

#if EXPENSIVE_INTEGRITY_CHECKS
static void
check_done_ec_offset(const struct min_env *env);
#endif

static int
update_after_partition(struct min_env *env,
    const size_t *partition_counts,
    fsm_state_t ec_src, fsm_state_t ec_dst);

static void
update_ec_links(struct min_env *env, fsm_state_t ec_dst);

static void
init_label_iterator(const struct min_env *env,
	fsm_state_t ec_i, int special_handling,
	struct min_label_iterator *li);

static int
try_partition(struct min_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
    size_t partition_counts[2], uint64_t checked_labels[256/64]);

static int
label_sets_match(const uint64_t a[256/64], const uint64_t b[256/64]);

#endif
