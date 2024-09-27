/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_IR_H
#define FSM_INTERNAL_IR_H

/*
 * Codegen IR intended for C-like switch statements
 *
 * The fsm_state structs don't exist from this IR onwards.
 * These nodes are always DFA. No epsilons, No duplicate edge labels,
 * so this IR takes advantage of that.
 *
 * But this is not neccessarily a minimal DFA. By this stage,
 * we're done with graph algorithmics.
 */

struct fsm_options;

enum ir_strategy {
	IR_NONE     = 1 << 0,
	IR_SAME     = 1 << 1,
	IR_COMPLETE = 1 << 2,
	IR_PARTIAL  = 1 << 3,
	IR_DOMINANT = 1 << 4,
	IR_ERROR    = 1 << 5,
	IR_TABLE    = 1 << 6
};

/* ranges are inclusive */
struct ir_range {
	unsigned char start;
	unsigned char end;
};

/*
 * Each group represents a single destination state, and the same state
 * cannot appear in multiple groups.
 */
struct ir_group {
	unsigned to;
	size_t n;
	const struct ir_range *ranges; /* array */
};

struct ir_error {
	size_t n;
	const struct ir_range *ranges; /* array */
};

struct ir_state {
	const char *example;

	struct ir_state_endids {
		fsm_end_id_t *ids; /* NULL -> 0 */
		size_t count;
	} endids;

	unsigned int isend:1;

	enum ir_strategy strategy;
	union {
		struct {
			unsigned to;
		} same;

		struct {
			size_t n;
			const struct ir_group *groups; /* array */
		} complete;

		struct {
			size_t n;
			const struct ir_group *groups; /* array */
		} partial;

		struct {
			unsigned mode;
			size_t n;
			const struct ir_group *groups; /* array */
		} dominant;

		struct {
			unsigned mode;
			struct ir_error error;
			size_t n;
			const struct ir_group *groups; /* array */
		} error;

		struct {
			/* Note: This is allocated separately, to avoid
			 * making the union significantly larger. */
			struct ir_state_table {
				unsigned to[FSM_SIGMA_COUNT];
			} *table;
		} table;
	} u;
};

struct ir {
	size_t n;
	unsigned start;
	struct ir_state *states; /* array */
};

/* caller frees */
int
make_example(const struct fsm *fsm, fsm_state_t s, char **example);

/* TODO: can pass in mask of allowed strategies */
struct ir *
make_ir(const struct fsm *fsm, const struct fsm_options *opt);

void
free_ir(const struct fsm *fsm, struct ir *ir);

#endif

