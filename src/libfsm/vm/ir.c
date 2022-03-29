/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#include <fsm/fsm.h>
#include <fsm/vm.h>

#include "libfsm/internal.h"

#include "vm.h"

#include "print/ir.h"

// VM intermediate representation
//
//   each opcode represents a change to the VM state.  Each opcode should
//   loosely translate into a single VM instruction, and should map fairly
//   directly to machine code instructions.
//
// Current IR opcodes:
//
//   FETCH succ:BOOL
//     fetches the next byte in the string buffer.  SP is advanced, SB is updated.
//
//     If the string buffer is empty, FETCH will attempt to fill it.  If the
//     buffer cannot be filled with any bytes, matching will end.
//
//     If the 'succ' parameter is true, an empty buffer is treated as a successful
//     match.  Otherwise it's treated as a failed match.
//
//     When FETCH completes the PC is advanced to the next instruction.
//
//   STOP cond:COND arg:BYTE succ:BOOL
//
//     Stops matching if cond(SB,arg) is true.
//
//     If the 'succ' parameter is true, the match is successful, otherwise the
//     match fails.
//
//     When STOP completes, the PC is advanced to the next instruction.  If STOP
//     'cond' is true, STOP never completes.
//
//   BRANCH cond:COND arg:BYTE dest:ADDR
//
//     If cond(SB,arg) is true, sets the PC to the instruction at 'dest'.
//     Otherwise advances the PC to the next instruction.
//
// Potential future opcodes:
//
//   FINDB arg:BYTE succ:BOOL
//
//     Searches the buffer for the first a byte equal to 'arg'.
//
//     If a byte is found, advances SP to its position, sets SB to that byte, and
//     advanced PC to the next instruction.
//
//     Until a byte is found equal to 'arg', FINDB will attempt to refill the
//     buffer and continue the search.
//
//     If the buffer cannot be filled:
//       if 'succ' is true, the match is considered successful
//       otherwise, the match is considered a failure
//
//   FINDS str:ADDR succ:BOOL
//
//     Like FINDB, but searches the buffer for the string 'str' instead of a byte.
//     String is length-encoded.
//
//   MATCHS str:ADDR
//
//     Matches the bytes given by 'str' with the current bytes in the buffer.
//     SB is set to the number of bytes that match.
//
//     Note that 'str' is limited to 255 bytes.
//
//     This instruction must be followed by BRANCH instructions to decode where
//     the match failed.
//
//  TBRANCH table:ADDR
//
//     This is a "table branch" instruction.  The table should have 256
//     addresses (0-255).  This instruction sets PC to the address at table[SB]

#define ARRAYLEN(a) (sizeof (a) / sizeof ((a)[0]))

struct dfavm_op_ir_pool {
	struct dfavm_op_ir_pool *next;

	unsigned int top;
	struct dfavm_op_ir ops[1024];
};

struct dfa_table {
	long tbl[FSM_SIGMA_COUNT];

	const struct ir_state *ir_state;

	int nerr;
	int ndst;

	int dst_ind_lo;
	int dst_ind_hi;

	struct {
		long to;
		struct ir_range syms;
	} longest_run;

	struct {
		unsigned char sym[FSM_SIGMA_COUNT];
		int count;
	} discontig;

	struct {
		long to;
		int count;
	} mode;
};

static struct dfavm_op_ir_pool *
new_op_pool(struct dfavm_op_ir_pool *pcurr)
{
	static const struct dfavm_op_ir_pool zero;

	struct dfavm_op_ir_pool *p;

	p = malloc(sizeof *p);
	if (p != NULL) {
		*p = zero;
		p->next = pcurr;
	}

	return p;
}

static struct dfavm_op_ir *
pool_newop(struct dfavm_op_ir_pool **poolp)
{
	struct dfavm_op_ir_pool *p;

	assert(poolp != NULL);

	p = *poolp;

	if (p == NULL || p->top >= ARRAYLEN(p->ops)) {
		p = new_op_pool(*poolp);
		if (p == NULL) {
			return NULL;
		}

		*poolp = p;
	}

	return &p->ops[p->top++];
}

static void
print_op_ir(FILE *f, struct dfavm_op_ir *op)
{
	const char *cmp;
	int nargs = 0;

	cmp = cmp_name(op->cmp);

	// fprintf(f, "[%4" PRIu32 "] %1u\t", op->offset, op->num_encoded_bytes);

	char opstr_buf[128];
	size_t nop = sizeof opstr_buf;
	char *opstr = &opstr_buf[0];
	size_t n;

	switch (op->instr) {
	case VM_OP_FETCH:
		n = snprintf(opstr, nop, "FETCH%c%s",
			(op->u.fetch.end_bits == VM_END_FAIL) ? 'F' : 'S',
			cmp);
		break;

	case VM_OP_STOP:
		n = snprintf(opstr, nop, "STOP%c%s",
			(op->u.stop.end_bits == VM_END_FAIL) ? 'F' : 'S',
			cmp);
		break;

	case VM_OP_BRANCH:
		{
			n = snprintf(opstr, nop, "BR%s", cmp);
		}
		break;

	default:
		n = snprintf(opstr, nop, "UNK_%d_%s", (int)op->instr, cmp);
	}

	opstr += n;
	nop   -= n;

	if (op->cmp != VM_CMP_ALWAYS) {
		if (isprint(op->cmp_arg)) {
			n = snprintf(opstr, nop, " '%c'", op->cmp_arg);
		} else {
			n = snprintf(opstr, nop, " 0x%02x",(unsigned)op->cmp_arg);
		}

		opstr += n;
		nop -= n;

		nargs++;
	}

	if (op->instr == VM_OP_BRANCH) {
		n = snprintf(opstr, nop, "%s [st=%u]", ((nargs>0) ? "," : " "),
			op->u.br.dest_state);

		opstr += n;
		nop   -= n;

		nargs++;
	}

	char comment[128];
	opstr = &comment[0];
	nop = sizeof comment;

	n = snprintf(opstr, nop, "; incoming: %u", op->num_incoming);
	opstr += n;
	nop   -= n;

	switch (op->instr) {
	case VM_OP_FETCH:
		n = snprintf(opstr, nop, ", state: %u", op->u.fetch.state);
		break;

	case VM_OP_BRANCH:
		n = snprintf(opstr, nop, ", dest: %u (%p)",
			op->u.br.dest_arg->index, (void *)op->u.br.dest_arg);
		break;

	default:
		n = 0;
		break;
	}

	fprintf(f, "%-40s %s\n", opstr_buf, comment);
}

static void
opasm_free(struct dfavm_assembler_ir *a, struct dfavm_op_ir *op)
{
	static const struct dfavm_op_ir zero;

	*op = zero;
	op->next = a->freelist;
	a->freelist = op;
}

static void
opasm_free_list(struct dfavm_assembler_ir *a, struct dfavm_op_ir *op)
{
	struct dfavm_op_ir *next;
	while (op != NULL) {
		next = op->next;
		opasm_free(a,op);
		op = next;
	}
}

static struct dfavm_op_ir *
opasm_new(struct dfavm_assembler_ir *a, enum dfavm_op_instr instr, enum dfavm_op_cmp cmp, unsigned char arg,
	const struct ir_state *ir_state)
{
	static const struct dfavm_op_ir zero;

	struct dfavm_op_ir *op;

	if (a->freelist != NULL) {
		op = a->freelist;
		a->freelist = op->next;
	} else {
		op = pool_newop(&a->pool);
	}

	if (op != NULL) {
		*op = zero;

		op->asm_index = a->count++;
		op->index = 0;

		op->cmp   = cmp;
		op->instr = instr;

		op->cmp_arg = arg;
	}

	op->ir_state = ir_state;

	return op;
}

static struct dfavm_op_ir *
opasm_new_fetch(struct dfavm_assembler_ir *a, unsigned state, enum dfavm_op_end end,
	const struct ir_state *ir_state)
{
	struct dfavm_op_ir *op;

	op = opasm_new(a, VM_OP_FETCH, VM_CMP_ALWAYS, 0, ir_state);
	if (op != NULL) {
		op->u.fetch.state    = state;
		op->u.fetch.end_bits = end;
	}

	return op;
}

static struct dfavm_op_ir *
opasm_new_stop(struct dfavm_assembler_ir *a, enum dfavm_op_cmp cmp, unsigned char arg, enum dfavm_op_end end,
	const struct ir_state *ir_state)
{
	struct dfavm_op_ir *op;

	op = opasm_new(a, VM_OP_STOP, cmp, arg, ir_state);
	if (op != NULL) {
		op->u.stop.end_bits = end;
	}

	return op;
}

static struct dfavm_op_ir *
opasm_new_branch(struct dfavm_assembler_ir *a, enum dfavm_op_cmp cmp, unsigned char arg, uint32_t dest_state,
	const struct ir_state *ir_state)
{
	struct dfavm_op_ir *op;

	assert(dest_state < a->nstates);

	op = opasm_new(a, VM_OP_BRANCH, cmp, arg, ir_state);
	if (op != NULL) {
		// op->u.br.dest  = VM_DEST_FAR;  // start with all branches as 'far'
		op->u.br.dest_state = dest_state;
	}

	return op;
}

void
dfavm_opasm_finalize_op(struct dfavm_assembler_ir *a)
{
	struct dfavm_op_ir_pool *pool_curr, *pool_next;

	if (a == NULL) {
		return;
	}

	free(a->ops);

	for (pool_curr = a->pool; pool_curr != NULL; pool_curr = pool_next) {
		pool_next = pool_curr->next;
		free(pool_curr);
	}
}

static int
cmp_mode_dests(const void *a, const void *b) {
	const long *va = a;
	const long *vb = b;

	return (*va > *vb) - (*va < *vb);
}

static void
analyze_table(struct dfa_table *table)
{
	static const struct dfa_table zero;

	int i, lo, nerr, ndst, max_run;
	int run_lo, run_hi;
	int dst_ind_lo, dst_ind_hi;
	long dlo, run_to;

	long mode_dests[FSM_SIGMA_COUNT];

	lo = 0;
	dlo = table->tbl[0];

	nerr = (dlo == -1) ? 1 : 0;
	ndst = 1-nerr;

	dst_ind_lo = (dlo == -1) ? -1 : 0;
	dst_ind_hi = dst_ind_lo;

	max_run = 0;
	run_lo = -1;
	run_hi = -1;
	run_to = -1;

	mode_dests[0] = dlo;
	table->discontig = zero.discontig;

	for (i=1; i < FSM_SIGMA_COUNT+1; i++) {
		long dst = -1;

		if (i < FSM_SIGMA_COUNT) {
			dst = table->tbl[i];
			nerr += (dst == -1);
			ndst += (dst != -1);

			if (dst >= 0) {
				dst_ind_hi = i;
				if (dst_ind_lo < 0) {
					dst_ind_lo = i;
				}
			}

			mode_dests[i] = dst;
		}

		if (i == FSM_SIGMA_COUNT || dst != dlo) {
			int len = i - lo;

			if (len > max_run) {
				max_run = len;
				run_lo  = lo;
				run_hi  = i-1;
				run_to  = dlo;
			}

			if (len == 1) {
				table->discontig.sym[table->discontig.count++] = lo;
			}

			lo = i;
			dlo = dst;
		}
	}

	table->nerr = nerr;
	table->ndst = ndst;

	table->dst_ind_lo = dst_ind_lo;
	table->dst_ind_hi = dst_ind_hi;

	if (max_run > 0) {
		table->longest_run.to = run_to;
		table->longest_run.syms.start = run_lo;
		table->longest_run.syms.end   = run_hi;
	}

	table->mode.to = -1;
	table->mode.count = 0;

	// determine the mode
	qsort(&mode_dests[0], sizeof mode_dests / sizeof mode_dests[0], sizeof mode_dests[0], cmp_mode_dests);

	for (lo=0,i=1; i < FSM_SIGMA_COUNT+1; i++) {
		if (i == FSM_SIGMA_COUNT || mode_dests[lo] != mode_dests[i]) {
			int count = i-lo;
			if (count > table->mode.count) {
				table->mode.to    = mode_dests[lo];
				table->mode.count = count;
				lo = i;
			}
		}
	}
}

static int
xlate_table_ranges(struct dfavm_assembler_ir *a, struct dfa_table *table, struct dfavm_op_ir **opp)
{
	int i,lo;
	int count = 0;

	lo = 0;
	for (i=1; i < FSM_SIGMA_COUNT; i++) {
		int64_t dst;
		struct dfavm_op_ir *op;

		assert(lo < FSM_SIGMA_COUNT);

		if (table->tbl[lo] != table->tbl[i]) {
			dst = table->tbl[lo];

			/* emit instr */
			int arg = i-1;
			enum dfavm_op_cmp cmp = (i > lo+1) ? VM_CMP_LE : VM_CMP_EQ;

			op = (dst < 0)
				? opasm_new_stop(a, cmp, arg, VM_END_FAIL, table->ir_state)
				: opasm_new_branch(a, cmp, arg, dst, table->ir_state);

			if (op == NULL) {
				return -1;
			}

			*opp = op;
			opp = &(*opp)->next;
			count++;

			lo = i;
		}
	}

	if (lo < FSM_SIGMA_COUNT) {
		int64_t dst = table->tbl[lo];
		*opp = (dst < 0)
			? opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL, table->ir_state)
			: opasm_new_branch(a, VM_CMP_ALWAYS, 0, dst, table->ir_state);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;
		count++;
	}

	return count;
}

static int
xlate_table_cases(struct dfavm_assembler_ir *a, struct dfa_table *table, struct dfavm_op_ir **opp)
{
	int i, count = 0;
	int64_t mdst = table->mode.to;

	for (i=0; i < FSM_SIGMA_COUNT; i++) {
		int64_t dst;

		dst = table->tbl[i];

		if (dst == mdst) {
			continue;
		}

		*opp = (dst < 0)
			? opasm_new_stop(a, VM_CMP_EQ, i, VM_END_FAIL, table->ir_state)
			: opasm_new_branch(a, VM_CMP_EQ, i, dst, table->ir_state);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;
		count++;
	}

	*opp = (mdst < 0)
		? opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL, table->ir_state)
		: opasm_new_branch(a, VM_CMP_ALWAYS, 0, mdst, table->ir_state);
	if (*opp == NULL) {
		return -1;
	}
	opp = &(*opp)->next;
	count++;

	return count;
}

static int
initial_translate_table(struct dfavm_assembler_ir *a, struct dfa_table *table, struct dfavm_op_ir **opp)
{
	int count, best_count;
	struct dfavm_op_ir *op, *best_op;

	assert(a     != NULL);
	assert(table != NULL);
	assert(opp   != NULL);

	assert(*opp == NULL);

	analyze_table(table);

	// handle a couple of special cases...
	if (table->ndst == 1) {
		int sym;
		long dst;

		sym = table->dst_ind_lo;

		assert(sym >= 0);
		assert(sym == table->dst_ind_hi);

		dst = table->tbl[sym];

		assert(dst >= 0);
		assert((size_t)dst < a->nstates);

		*opp = opasm_new_stop(a, VM_CMP_NE, sym, VM_END_FAIL, table->ir_state);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;

		*opp = opasm_new_branch(a, VM_CMP_ALWAYS, 0, dst, table->ir_state);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;

		return 0;
	}

	best_op = NULL;
	best_count = xlate_table_ranges(a, table, &best_op);

	op = NULL;
	count = xlate_table_cases(a, table, &op);

	if (count < best_count) {
		opasm_free_list(a,best_op);
		best_op = op;
	} else {
		opasm_free_list(a,op);
	}

	*opp = best_op;

	return 0;
}

static void
ranges_to_table(long table[FSM_SIGMA_COUNT], const struct ir_range *r, const size_t n, const long to)
{
	size_t i;

	for (i=0; i < n; i++) {
		int c, end;

		end = r[i].end;
		for (c = r[i].start; c <= end; c++) {
			table[c] = to;
		}
	}
}

static void
error_to_table(struct dfa_table *table, const struct ir_error *err)
{
	ranges_to_table(table->tbl, err->ranges, err->n, -1);
}

static void
group_to_table(struct dfa_table *table, const struct ir_group *grp)
{
	ranges_to_table(table->tbl, grp->ranges, grp->n, grp->to);
}

static void
dfa_table_init(struct dfa_table *table, long default_dest, const struct ir_state *ir_state)
{
	static const struct dfa_table zero;
	int i;

	assert(table != NULL);

	*table = zero;

	table->ir_state = ir_state;

	for (i=0; i < FSM_SIGMA_COUNT; i++) {
		table->tbl[i] = default_dest;
	}
}

static int
initial_translate_partial(struct dfavm_assembler_ir *a, struct ir_state *st, struct dfavm_op_ir **opp)
{
	struct dfa_table table;
	size_t i, ngrps;

	assert(st->strategy == IR_PARTIAL);

	dfa_table_init(&table, -1, st);

	ngrps = st->u.partial.n;
	for (i=0; i < ngrps; i++) {
		group_to_table(&table, &st->u.partial.groups[i]);
	}

	return initial_translate_table(a, &table, opp);
}

static int
initial_translate_dominant(struct dfavm_assembler_ir *a, struct ir_state *st, struct dfavm_op_ir **opp)
{
	struct dfa_table table;
	size_t i, ngrps;

	assert(st->strategy == IR_DOMINANT);

	dfa_table_init(&table, st->u.dominant.mode, st);

	ngrps = st->u.dominant.n;
	for (i=0; i < ngrps; i++) {
		group_to_table(&table, &st->u.dominant.groups[i]);
	}

	return initial_translate_table(a, &table, opp);
}

static int
initial_translate_error(struct dfavm_assembler_ir *a, struct ir_state *st, struct dfavm_op_ir **opp)
{
	struct dfa_table table;
	size_t i, ngrps;

	assert(st->strategy == IR_ERROR);

	dfa_table_init(&table, st->u.error.mode, st);

	error_to_table(&table, &st->u.error.error);

	ngrps = st->u.error.n;
	for (i=0; i < ngrps; i++) {
		group_to_table(&table, &st->u.error.groups[i]);
	}

	return initial_translate_table(a, &table, opp);
}

static struct dfavm_op_ir *
initial_translate_state(struct dfavm_assembler_ir *a, const struct ir *ir, size_t ind)
{
	struct ir_state *st;
	struct dfavm_op_ir **opp;

	st = &ir->states[ind];
	opp = &a->ops[ind];

	if (st->isend && st->strategy == IR_SAME && st->u.same.to == ind) {
		*opp = opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_SUCC, st);
		return a->ops[ind];
	}

	*opp = opasm_new_fetch(a, ind, (st->isend) ? VM_END_SUCC : VM_END_FAIL, st);
	opp = &(*opp)->next;
	assert(*opp == NULL);

	switch (st->strategy) {
	case IR_NONE:
		*opp = opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL, st);
		opp = &(*opp)->next;
		break;

	case IR_SAME:
		*opp = opasm_new_branch(a, VM_CMP_ALWAYS, 0, st->u.same.to, st);
		opp = &(*opp)->next;
		break;

	case IR_COMPLETE:
		/* groups should be sorted */

	/* _currently_ expand these into a table and build the
	 * transitions off of the table.  We can do this more
	 * intelligently.
	 */
	case IR_PARTIAL:
		if (initial_translate_partial(a, st, opp) < 0) {
			return NULL;
		}
		break;

	case IR_DOMINANT:
		if (initial_translate_dominant(a, st, opp) < 0) {
			return NULL;
		}
		break;

	case IR_ERROR:
		if (initial_translate_error(a, st, opp) < 0) {
			return NULL;
		}
		break;

	case IR_TABLE:
		/* currently not used */
		fprintf(stderr, "implement IR_TABLE!\n");
		abort();

	default:
		fprintf(stderr, "unknown strategy!\n");
		abort();
	}

	return a->ops[ind];
}

static int
initial_translate(const struct ir *ir, struct dfavm_assembler_ir *a)
{
	size_t i,n;

	n = a->nstates;

	for (i=0; i < n; i++) {
		a->ops[i] = initial_translate_state(a, ir, i);
	}

	return 0;
}

static void
fixup_dests(struct dfavm_assembler_ir *a)
{
	size_t i,n;

	n = a->nstates;
	for (i=0; i < n; i++) {
		struct dfavm_op_ir *op;

		for (op = a->ops[i]; op != NULL; op = op->next) {
			if (op->instr != VM_OP_BRANCH) {
				continue;
			}

			op->u.br.dest_arg = a->ops[op->u.br.dest_state];
			op->u.br.dest_arg->num_incoming++;
		}
	}
}

struct dfavm_op_ir **find_opchain_end(struct dfavm_op_ir **opp)
{
	assert(opp != NULL);

	while (*opp != NULL) {
		opp = &(*opp)->next;
	}

	return opp;
}

static void
dump_states(FILE *f, struct dfavm_assembler_ir *a);

static void
eliminate_unnecessary_branches(struct dfavm_assembler_ir *a)
{
	int count;

	do {
		struct dfavm_op_ir **opp;

		count = 0;

		/* basic pass to eliminate unnecessary branches; branches to the
		 * next instruction can be elided
		 */
		for (opp = &a->linked; *opp != NULL;) {
			struct dfavm_op_ir *next, *dest;

			if ((*opp)->instr != VM_OP_BRANCH) {
				opp = &(*opp)->next;
				continue;
			}

			if ((*opp)->num_incoming > 0) {
				opp = &(*opp)->next;
				continue;
			}

			dest = (*opp)->u.br.dest_arg;
			next = (*opp)->next;

			assert(dest != NULL);

			if (dest == next) {
				// branch is to next instruction, eliminate
				//
				// condition doesn't matter since both cond and !cond
				// will end up at the same place
				*opp = next;
				next->num_incoming--;
				count++;
				continue;
			}

			// Rewrites:
			//   curr: BRANCH to next->next on condition C
			//   next: BRANCH ALWAYS to dest D
			// to:
			//   curr: BRANCH to dest D on condition not(C)
			//   next: <deleted>
			// 
			// Rewrites:
			//   curr: BRANCH to next->next on condition C
			//   next: STOP(S/F) ALWAYS
			// to:
			//   curr: STOP(S/F) on condition not(C)
			//   next: <deleted>
			//
			if (next != NULL && dest == next->next &&
					(next->instr == VM_OP_BRANCH || next->instr == VM_OP_STOP) &&
					(next->num_incoming == 0) &&
					(*opp)->cmp != VM_CMP_ALWAYS && next->cmp == VM_CMP_ALWAYS) {
				/* rewrite last two instructions to eliminate a
				 * branch
				 */
				struct dfavm_op_ir rewrite = *next;  // swapped
				int ok = 1;

				// invert the condition of current branch
				switch ((*opp)->cmp) {
				case VM_CMP_LT: rewrite.cmp = VM_CMP_GE; break;
				case VM_CMP_LE: rewrite.cmp = VM_CMP_GT; break;
				case VM_CMP_EQ: rewrite.cmp = VM_CMP_NE; break;
				case VM_CMP_GE: rewrite.cmp = VM_CMP_LT; break;
				case VM_CMP_GT: rewrite.cmp = VM_CMP_LE; break;
				case VM_CMP_NE: rewrite.cmp = VM_CMP_EQ; break;

				case VM_CMP_ALWAYS:
				default:
					// something is wrong
					ok = 0;
					break;
				}

				if (ok) {
					rewrite.cmp_arg = (*opp)->cmp_arg;

					**opp = rewrite;

					assert(dest->num_incoming > 0);
					dest->num_incoming--;

					count++;
					continue;
				}
			}

			opp = &(*opp)->next;
		}

	} while (count > 0);
}

static void
order_basic_blocks(struct dfavm_assembler_ir *a)
{
	size_t i,n;
	struct dfavm_op_ir **opp;
	struct dfavm_op_ir *st;

	/* replace this with something that actually
	 * orders basic blocks ...
	 */

	n = a->nstates;

	/* mark all states as !in_trace */
	for (i=0; i < n; i++) {
		a->ops[i]->in_trace = 0;
	}

	opp = &a->linked;
	*opp = NULL;

	st = a->ops[a->start];
	while (st != NULL) {
		struct dfavm_op_ir *branches[FSM_SIGMA_COUNT];  /* at most FSM_SIGMA_COUNT branches per state */
		struct dfavm_op_ir *instr;
		size_t j,count;

		/* add state to trace */
		*opp = st;
		opp = find_opchain_end(opp);

		assert(opp != NULL);
		assert(*opp == NULL);

		st->in_trace = 1;

		/* look for branches to states not in the trace.
		 * Start at the last branch and work toward the first.
		 */
		count = 0;
		for (instr=st; instr != NULL; instr=instr->next) {
			if (instr->instr == VM_OP_BRANCH) {
				branches[count++] = instr;
			}
			assert(count <= sizeof branches/sizeof branches[0]);
		}

		st = NULL;
		for (j=count; j > 0; j--) {
			struct dfavm_op_ir *dest = branches[j-1]->u.br.dest_arg;
			if (!dest->in_trace) {
				st = dest;
				break;
			}
		}

		if (st == NULL) {
			/* look for a new state ... */
			for (j=0; j < n; j++) {
				if (!a->ops[j]->in_trace) {
					st = a->ops[j];
					break;
				}
			}
		}
	}
}

static uint32_t
assign_opcode_indexes(struct dfavm_assembler_ir *a)
{
	uint32_t index;
	struct dfavm_op_ir *op;

	assert(a != NULL);
	assert(a->linked != NULL);

	index = 0;
	for (op=a->linked; op != NULL; op = op->next) {
		op->index = index++;
	}

	return index;
}

static void
dump_states(FILE *f, struct dfavm_assembler_ir *a)
{
	struct dfavm_op_ir *op;

	for (op = a->linked; op != NULL; op = op->next) {
		if (op->instr == VM_OP_FETCH) {
			unsigned state = op->u.fetch.state;
			fprintf(f, "\n%p ;;; state %u (index: %" PRIu32 ", asm_index: %" PRIu32 ") %s\n",
				(void *) op, state, op->index, op->asm_index,
				(state == a->start) ? "(start)" : "");
		}

		fprintf(f, "%p | %6" PRIu32 " | %6" PRIu32 " |  ", (void *)op, op->index, op->asm_index);
		print_op_ir(f, op);
	}
}

static void
print_all_states(struct dfavm_assembler_ir *a)
{
	dump_states(stderr, a);
}

int
dfavm_compile_ir(struct dfavm_assembler_ir *a, const struct ir *ir, struct fsm_vm_compile_opts opts)
{
	a->nstates = ir->n;
	a->start = ir->start;

	(void)dump_states; /* make clang happy */
	(void)print_all_states;

	if (ir->n == 0) {
		goto empty;
	}

	a->ops = calloc(ir->n, sizeof a->ops[0]);
	if (a->ops == NULL) {
		return 0;
	}

	if (initial_translate(ir, a) < 0) {
		return 0;
	}

	fixup_dests(a);

	if (opts.flags & FSM_VM_COMPILE_PRINT_IR_PREOPT) {
		FILE *f = (opts.log != NULL) ? opts.log : stdout;

		fprintf(f, "---[ before optimization ]---\n");
		dump_states(f, a);
		fprintf(f, "\n");
	}

	order_basic_blocks(a);

	/* basic optimizations */
	if (opts.flags & FSM_VM_COMPILE_OPTIM) {
		eliminate_unnecessary_branches(a);
	}

	/* optimization is finished.  now assign opcode indexes */
	assign_opcode_indexes(a);

	if (opts.flags & FSM_VM_COMPILE_PRINT_IR) {
		FILE *f = (opts.log != NULL) ? opts.log : stdout;

		fprintf(f, "---[ final IR ]---\n");
		dump_states(f, a);
		fprintf(f, "\n");
	}

	return 1;

empty:

	a->ops = calloc(1, sizeof a->ops[0]);
	if (a->ops == NULL) {
		return 0;
	}

	a->ops[0] = opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL, NULL);
	if (a->ops[0] == NULL) {
		return -1;
	}

	a->linked = a->ops[0];

	assign_opcode_indexes(a);

	return 1;
}

