#ifndef FSM_WALK2_H
#define FSM_WALK2_H

/* Bit values for walking two fsms together.  Used to constrain what
 * edges will be followed and what combined states can be considered end
 * states.
 *
 * There are four bits because there are four possible states (NEITHER,
 * ONLYA, ONLYB, BOTH).  We use these bits as a fast table lookup.
 *
 * When walking two graphs, the caller orders them (A,B).  This is
 * important for some non-commutative binary operations, like
 * subtraction, where A-B != B-A.
 *
 * The names below assume that the order is (A,B), but make no
 * assumptions about what that means.
 *
 * These values can be bitwise OR'd together.  So, to find the states
 * for subtraction A-B, the mask would be:
 *      FSM_WALK2_ONLYA | FSM_WALK2_BOTH
 */
enum {
	FSM_WALK2_NEITHER = 1 << 0x0,   /* To complete the four possible states */
	FSM_WALK2_ONLYB   = 1 << 0x1,   /* Follow edges that B has and A does not */
	FSM_WALK2_ONLYA   = 1 << 0x2,   /* Follow edges that A has and B does not */
	FSM_WALK2_BOTH    = 1 << 0x3    /* Follow edges that both A and B have */
};

/* Constraints for walking edges for union (A|B), intersection (A&B), and
 * subtraction (A-B)
 */
enum {
	FSM_WALK2_EDGE_UNION     = FSM_WALK2_ONLYA | FSM_WALK2_ONLYB | FSM_WALK2_BOTH,
	FSM_WALK2_EDGE_INTERSECT = FSM_WALK2_BOTH,
	FSM_WALK2_EDGE_SUBTRACT  = FSM_WALK2_ONLYA | FSM_WALK2_BOTH
};

/* Constraints for end states for union (A|B), intersection (A&B), and
 * subtraction (A-B)
 *
 * Notice that the edge constraint for subtraction is different from the
 * end state constraint.  With subtraction, you want to follow edges
 * that are either ONLYA or BOTH, but valid end states must be ONLYA.
 */
enum {
	FSM_WALK2_END_UNION     = FSM_WALK2_ONLYA | FSM_WALK2_ONLYB | FSM_WALK2_BOTH,
	FSM_WALK2_END_INTERSECT = FSM_WALK2_BOTH,
	FSM_WALK2_END_SUBTRACT  = FSM_WALK2_ONLYA
};

struct fsm_walk2_tuple_pool;

/* Tuple (a,b,comb) for walking the two DFAs.  a & b are the states of
 * the original FSMs.  comb is the state of the combined FSM.
 */
struct fsm_walk2_tuple {
	struct fsm_state *a;
	struct fsm_state *b;
	struct fsm_state *comb;
};

struct fsm_walk2_data {
	struct fsm_walk2_tuple_pool *head;
	size_t top;

	struct fsm *new;
	struct set *states;

	/* table for which combinations are valid bits.
	 * There are four combinations:
	 *
	 *   a_end  b_end    AB		bit	endmask
	 *   false  false    00		0	0x1
	 *   false  true     01		1	0x2
	 *   true   false    10		2	0x4
	 *   true   true     11		3	0x8
	 *
	 * Here bit is the bit that expresses whether that combination
	 * is a valid end state or not.  We need four bits.
	 *
	 * Operation	Requirement			endmask
	 * intersect    both true			0x8
	 * subtract     first true, second false	0x4
	 * union	either true			0xE
	 */
	unsigned endmask:4;  /* bit table for what states are end states in the combined graph */
	unsigned edgemask:4; /* bit table for which edges should be followed */
};

int
fsm_walk2_edges(struct fsm_walk2_data *data, struct fsm *a, struct fsm *b, struct fsm_walk2_tuple *start);

struct fsm_walk2_tuple *
fsm_walk2_tuple_new(struct fsm_walk2_data *data, struct fsm_state *a, struct fsm_state *b);

void
fsm_walk2_data_free(struct fsm_walk2_data *data);

/* Sets all of the equiv members of the states of the fsm to NULL */
void
fsm_walk2_mark_equiv_null(struct fsm *fsm);

#endif /* WALK2_H */

