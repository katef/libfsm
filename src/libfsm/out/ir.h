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
 * But this is not neccessarily a minimal DFA.Bby this stage,
 * we're done with graph algorithmics.
 */

enum ir_strategy {
	IR_NONE = 1 << 0,
	IR_SAME = 1 << 1,
	IR_MANY = 1 << 2,
	IR_MODE = 1 << 3,
	IR_JUMP = 1 << 4
};

struct ir_range {
	unsigned char start;
	unsigned char end;
	const struct ir_state *to;
};

struct ir_state {
	const char *example;
	unsigned int isend:1;

	void *opaque;

	enum ir_strategy strategy;
	union {
		struct {
			const struct ir_state *to;
		} same;

		struct {
			size_t n;
			struct ir_range *ranges; /* array */
		} many;

		struct {
			const struct ir_state *mode;
			size_t n;
			struct ir_range *ranges; /* array */
		} mode;

		struct {
			const struct ir_state *to[UCHAR_MAX];
		} jump;
	} u;
};

struct ir {
	size_t n;
	const struct ir_state *start;
	struct ir_state *states; /* array */
};

/* TODO: can pass in mask of allowed strategies */
struct ir *
make_ir(const struct fsm *fsm);

void
free_ir(struct ir *ir);

#endif

