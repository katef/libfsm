/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/vm.h>
#include "internal.h"
#include "print/ir.h"

#define DEBUG_ENCODING 0
#define DEBUG_VM_EXECUTION 0

// Instructions are 1-6 bytes each.  First byte is the opcode.  Second is the
// (optional) argument.  Additional bytes encode the branch destination.
//
// There are four instructions:
// BRANCH, MATCH, and STOP.  The opcodes for each are:
//
//          76543210
// BR       CCC10xDD
// STOP     CCC00xxE
// FETCH    00001xxE
//
// bits marked x are reserved for future use, and should be 0
//
// Possible future instruction:
// IBRANCH  CC11DDDD
//
// Comparison bits:
// 
//   CCC
//   000  always
//   001  less than
//   010  less than or equal to
//   011  greater than or equal to
//   100  greater than
//   101  equal to
//   110  not equal to
//
//   Comparisons other than 'always' have one comparison argument.  'Always'
//   comparisons have no comparison arguments.
//
// FETCH - fetch instructions
//
//   Fetch instructions fetch the next character from the stream and have a stop
//   bit that indicates success/failure if the stream has no more characters.
//
//   E = 0 indicates FETCHF, failure if EOS
//   E = 1 indicates FETCHS, success if EOS
//
//   FETCHS/F instructions should currently have comparison bits set to 00
//   (always).  A future version reserves the right to support conditional
//   FETCH.
//
//   (NB: the comparison bits are currently just ignored for FETCH instructions)
//
// STOP - stop instructions
//
// Stop instructions have an end bit: 
//     E = 0 indicates STOPF, stop w/ failure
//     E = 1 indicates STOPS, stop w/ success
//
//    STOPF generally indicates that there is no transition from the current
//         state matching the read character
//
//    STOPS generally indicates that the current state is an end state, is
//         complete, and all edges point back to the current state.
//
// BR - branch instructions
//
//   Branch instructions always have a signed relative destination argument.
//   The D bits specify its length:
//
//   DD
//   00    8-bit destination, 1-byte destination argument
//   01   16-bit destination, 2-byte destination argument
//   10   32-bit destination, 4-byte destination argument, LSB ignored
//
//   DD=11 is reserved for future use
//

#define MASK_OP   0x18
#define MASK_CMP  0xE0
#define MASK_END  0x01
#define MASK_DEST 0x03

enum dfavm_instr_bits {
	// Stop the VM, mark match or failure
	VM_OP_STOP   = 0,

	// Start of each state: fetch next character.  Indicates
	// match / fail if EOS.
	VM_OP_FETCH  = 1,

	// Branch to another state
	VM_OP_BRANCH = 2,
};

enum dfavm_cmp_bits {
	VM_CMP_ALWAYS = 0,
	VM_CMP_LT     = 1,
	VM_CMP_LE     = 2,
	VM_CMP_GE     = 3,
	VM_CMP_GT     = 4,
	VM_CMP_EQ     = 5,
	VM_CMP_NE     = 6,
};

enum dfavm_end_bits {
	VM_END_FAIL = 0,
	VM_END_SUCC = 1,
};

enum dfavm_dest_bits {
	VM_DEST_NONE  = 0,
	VM_DEST_SHORT = 1,  // 11-bit dest
	VM_DEST_NEAR  = 2,  // 18-bit dest
	VM_DEST_FAR   = 3,  // 32-bit dest
};

struct dfavm_op {
	struct dfavm_op *next;
	uint32_t count;
	uint32_t offset;

	enum dfavm_cmp_bits cmp;
	enum dfavm_instr_bits instr;

	union {
		struct {
			unsigned state;
			enum dfavm_end_bits end_bits;
		} fetch;

		struct {
			enum dfavm_end_bits end_bits;
		} stop;

		struct {
			struct dfavm_op *dest_arg;
			enum dfavm_dest_bits dest;
			uint32_t dest_state;
			int32_t  rel_dest;
		} br;
	} u;

	unsigned char cmp_arg;
	unsigned char num_encoded_bytes;
	int in_trace;
};

struct dfavm_op_pool {
	struct dfavm_op_pool *next;

	unsigned int top;
	struct dfavm_op ops[1024];
};

struct dfavm_assembler {
	struct dfavm_op_pool *pool;
	struct dfavm_op *freelist;

	struct dfavm_op **ops;
	struct dfavm_op *linked;

	size_t nstates;
	size_t start;

	uint32_t nbytes;
	uint32_t count;
};

struct fsm_dfavm {
	unsigned char *ops;
	uint32_t len;
};

static const char *
cmp_name(int cmp) {
	switch (cmp) {
	case VM_CMP_ALWAYS: return "";   break;
	case VM_CMP_LT:     return "LT"; break;
	case VM_CMP_LE:     return "LE"; break;
	case VM_CMP_EQ:     return "EQ"; break;
	case VM_CMP_GE:     return "GE"; break;
	case VM_CMP_GT:     return "GT"; break;
	case VM_CMP_NE:     return "NE"; break;
	default:            return "??"; break;
	}
}

static void
print_op(FILE *f, struct dfavm_op *op)
{
	const char *cmp;
	int nargs = 0;

	cmp = cmp_name(op->cmp);

	fprintf(f, "[%4lu] %1lu\t", (unsigned long)op->offset, (unsigned long)op->num_encoded_bytes);

	switch (op->instr) {
	case VM_OP_FETCH:
		fprintf(f, "FETCH%c%s",
			(op->u.fetch.end_bits == VM_END_FAIL) ? 'F' : 'S',
			cmp);
		break;

	case VM_OP_STOP:
		fprintf(f, "STOP%c%s",
			(op->u.stop.end_bits == VM_END_FAIL) ? 'F' : 'S',
			cmp);
		break;

	case VM_OP_BRANCH:
		{
			char dst;
			switch (op->u.br.dest) {
			case VM_DEST_NONE:  dst = 'Z'; break;
			case VM_DEST_SHORT: dst = 'S'; break;
			case VM_DEST_NEAR:  dst = 'N'; break;
			case VM_DEST_FAR:   dst = 'F'; break;
			default:            dst = '!'; break;
			}

			fprintf(f, "BR%c%s", dst, cmp);
		}
		break;

	default:
		fprintf(f, "UNK_%d_%s", (int)op->instr, cmp);
	}

	if (op->cmp != VM_CMP_ALWAYS) {
		if (isprint(op->cmp_arg)) {
			fprintf(f, " '%c'", op->cmp_arg);
		} else {
			fprintf(f, " 0x%02x",(unsigned)op->cmp_arg);
		}

		nargs++;
	}

	if (op->instr == VM_OP_BRANCH) {
		fprintf(f, "%s%ld [st=%lu]", ((nargs>0) ? ", " : " "),
			(long)op->u.br.rel_dest, (unsigned long)op->u.br.dest_state);
		nargs++;
	}

	fprintf(f, "\t; %6lu bytes", (unsigned long)op->num_encoded_bytes);
	switch (op->instr) {
	case VM_OP_FETCH:
		fprintf(f, "  [state %u]", op->u.fetch.state);
		break;

	case VM_OP_BRANCH:
		fprintf(f, "  [dest=%p]", (void *)op->u.br.dest_arg);
		break;

	default:
		break;
	}

	fprintf(f, "\n");
}

#define ARRAYLEN(a) (sizeof (a) / sizeof ((a)[0]))

static struct dfavm_op_pool *
new_op_pool(struct dfavm_op_pool *pcurr)
{
	static const struct dfavm_op_pool zero;

	struct dfavm_op_pool *p;

	p = malloc(sizeof *p);
	if (p != NULL) {
		*p = zero;
		p->next = pcurr;
	}

	return p;
}

static struct dfavm_op *
pool_newop(struct dfavm_op_pool **poolp)
{
	struct dfavm_op_pool *p;

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
opasm_free(struct dfavm_assembler *a, struct dfavm_op *op)
{
	static const struct dfavm_op zero;

	*op = zero;
	op->next = a->freelist;
	a->freelist = op;
}

static void
opasm_free_list(struct dfavm_assembler *a, struct dfavm_op *op)
{
	struct dfavm_op *next;
	while (op != NULL) {
		next = op->next;
		opasm_free(a,op);
		op = next;
	}
}

static struct dfavm_op *
opasm_new(struct dfavm_assembler *a, enum dfavm_instr_bits instr, enum dfavm_cmp_bits cmp, unsigned char arg)
{
	static const struct dfavm_op zero;

	struct dfavm_op *op;

	if (a->freelist != NULL) {
		op = a->freelist;
		a->freelist = op->next;
	} else {
		op = pool_newop(&a->pool);
	}

	if (op != NULL) {
		*op = zero;

		op->count = a->count++;

		op->cmp   = cmp;
		op->instr = instr;

		op->cmp_arg = arg;
	}

	return op;
}

static struct dfavm_op *
opasm_new_fetch(struct dfavm_assembler *a, unsigned state, enum dfavm_end_bits end)
{
	struct dfavm_op *op;

	op = opasm_new(a, VM_OP_FETCH, VM_CMP_ALWAYS, 0);
	if (op != NULL) {
		op->u.fetch.state    = state;
		op->u.fetch.end_bits = end;
	}

	return op;
}

static struct dfavm_op *
opasm_new_stop(struct dfavm_assembler *a, enum dfavm_cmp_bits cmp, unsigned char arg, enum dfavm_end_bits end)
{
	struct dfavm_op *op;

	op = opasm_new(a, VM_OP_STOP, cmp, arg);
	if (op != NULL) {
		op->u.stop.end_bits = end;
	}

	return op;
}

static struct dfavm_op *
opasm_new_branch(struct dfavm_assembler *a, enum dfavm_cmp_bits cmp, unsigned char arg, uint32_t dest_state)
{
	struct dfavm_op *op;

	assert(dest_state < a->nstates);

	op = opasm_new(a, VM_OP_BRANCH, cmp, arg);
	if (op != NULL) {
		op->u.br.dest  = VM_DEST_FAR;  // start with all branches as 'far'
		op->u.br.dest_state = dest_state;
	}

	return op;
}

void
dfavm_opasm_finalize(struct dfavm_assembler *a)
{
	static const struct dfavm_assembler zero;

	struct dfavm_op_pool *pool_curr, *pool_next;

	if (a == NULL) {
		return;
	}

	free(a->ops);

	for (pool_curr = a->pool; pool_curr != NULL; pool_curr = pool_next) {
		pool_next = pool_curr->next;
		free(pool_curr);
	}

	*a = zero;
}

struct dfa_table {
	long tbl[FSM_SIGMA_COUNT];

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
xlate_table_ranges(struct dfavm_assembler *a, struct dfa_table *table, struct dfavm_op **opp)
{
	int i,lo;
	int count = 0;

	lo = 0;
	for (i=1; i < FSM_SIGMA_COUNT; i++) {
		int64_t dst;
		struct dfavm_op *op;

		assert(lo < FSM_SIGMA_COUNT);

		if (table->tbl[lo] != table->tbl[i]) {
			dst = table->tbl[lo];

			/* emit instr */
			int arg = i-1;
			enum dfavm_cmp_bits cmp = (i > lo+1) ? VM_CMP_LE : VM_CMP_EQ;

			op = (dst < 0)
				? opasm_new_stop(a, cmp, arg, VM_END_FAIL)
				: opasm_new_branch(a, cmp, arg, dst);

			if (op == NULL) {
				return -1;
			}

			*opp = op;
			opp = &(*opp)->next;
			count++;

			lo = i;
		}
	}

	if (lo < FSM_SIGMA_COUNT-1) {
		int64_t dst = table->tbl[lo];
		*opp = (dst < 0)
			? opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL)
			: opasm_new_branch(a, VM_CMP_ALWAYS, 0, dst);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;
		count++;
	}

	return count;
}

static int
xlate_table_cases(struct dfavm_assembler *a, struct dfa_table *table, struct dfavm_op **opp)
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
			? opasm_new_stop(a, VM_CMP_EQ, i, VM_END_FAIL)
			: opasm_new_branch(a, VM_CMP_EQ, i, dst);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;
		count++;
	}

	*opp = (mdst < 0)
		? opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL)
		: opasm_new_branch(a, VM_CMP_ALWAYS, 0, mdst);
	if (*opp == NULL) {
		return -1;
	}
	opp = &(*opp)->next;
	count++;

	return count;
}

static int
initial_translate_table(struct dfavm_assembler *a, struct dfa_table *table, struct dfavm_op **opp)
{
	int count, best_count;
	struct dfavm_op *op, *best_op;

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

		*opp = opasm_new_stop(a, VM_CMP_NE, sym, VM_END_FAIL);
		if (*opp == NULL) {
			return -1;
		}
		opp = &(*opp)->next;

		*opp = opasm_new_branch(a, VM_CMP_ALWAYS, 0, dst);
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
dfa_table_init(struct dfa_table *table, long default_dest)
{
	static const struct dfa_table zero;
	int i;

	assert(table != NULL);

	*table = zero;

	for (i=0; i < FSM_SIGMA_COUNT; i++) {
		table->tbl[i] = default_dest;
	}
}

static int
initial_translate_partial(struct dfavm_assembler *a, struct ir_state *st, struct dfavm_op **opp)
{
	struct dfa_table table;
	size_t i, ngrps;

	assert(st->strategy == IR_PARTIAL);

	dfa_table_init(&table, -1);

	ngrps = st->u.partial.n;
	for (i=0; i < ngrps; i++) {
		group_to_table(&table, &st->u.partial.groups[i]);
	}

	return initial_translate_table(a, &table, opp);
}

static int
initial_translate_dominant(struct dfavm_assembler *a, struct ir_state *st, struct dfavm_op **opp)
{
	struct dfa_table table;
	size_t i, ngrps;

	assert(st->strategy == IR_DOMINANT);

	dfa_table_init(&table, st->u.dominant.mode);

	ngrps = st->u.dominant.n;
	for (i=0; i < ngrps; i++) {
		group_to_table(&table, &st->u.dominant.groups[i]);
	}

	return initial_translate_table(a, &table, opp);
}

static int
initial_translate_error(struct dfavm_assembler *a, struct ir_state *st, struct dfavm_op **opp)
{
	struct dfa_table table;
	size_t i, ngrps;

	assert(st->strategy == IR_ERROR);

	dfa_table_init(&table, st->u.error.mode);

	error_to_table(&table, &st->u.error.error);

	ngrps = st->u.error.n;
	for (i=0; i < ngrps; i++) {
		group_to_table(&table, &st->u.error.groups[i]);
	}

	return initial_translate_table(a, &table, opp);
}

static struct dfavm_op *
initial_translate_state(struct dfavm_assembler *a, struct ir *ir, size_t ind)
{
	struct ir_state *st;
	struct dfavm_op **opp;

	st = &ir->states[ind];
	opp = &a->ops[ind];

	if (st->isend && st->strategy == IR_SAME && st->u.same.to == ind) {
		*opp = opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_SUCC);
		return a->ops[ind];
	}

	*opp = opasm_new_fetch(a, ind, (st->isend) ? VM_END_SUCC : VM_END_FAIL);
	opp = &(*opp)->next;
	assert(*opp == NULL);

	switch (st->strategy) {
	case IR_NONE:
		*opp = opasm_new_stop(a, VM_CMP_ALWAYS, 0, VM_END_FAIL);
		opp = &(*opp)->next;
		break;

	case IR_SAME:
		*opp = opasm_new_branch(a, VM_CMP_ALWAYS, 0, st->u.same.to);
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
initial_translate(struct ir *ir, struct dfavm_assembler *a)
{
	size_t i,n;

	n = a->nstates;

	for (i=0; i < n; i++) {
		a->ops[i] = initial_translate_state(a, ir, i);
	}

	return 0;
}

static void
fixup_dests(struct dfavm_assembler *a)
{
	size_t i,n;

	n = a->nstates;
	for (i=0; i < n; i++) {
		struct dfavm_op *op;

		for (op = a->ops[i]; op != NULL; op = op->next) {
			if (op->instr != VM_OP_BRANCH) {
				continue;
			}

			op->u.br.dest_arg = a->ops[op->u.br.dest_state];
		}
	}
}

struct dfavm_op **find_opchain_end(struct dfavm_op **opp)
{
	assert(opp != NULL);

	while (*opp != NULL) {
		opp = &(*opp)->next;
	}

	return opp;
}

static void
eliminate_unnecessary_branches(struct dfavm_assembler *a)
{
	int count;

	do {
		struct dfavm_op **opp;

		count = 0;

		/* basic pass to eliminate unnecessary branches; branches to the
		 * next instruction can be elided
		 */
		for (opp = &a->linked; *opp != NULL;) {
			struct dfavm_op *next, *dest;

			if ((*opp)->instr != VM_OP_BRANCH) {
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
				count++;
				continue;
			}

			if (next != NULL && dest == next->next &&
					(next->instr == VM_OP_BRANCH || next->instr == VM_OP_STOP) &&
					(*opp)->cmp != VM_CMP_ALWAYS && next->cmp == VM_CMP_ALWAYS) {
				/* rewrite last two instructions to eliminate a
				 * branch
				 */
				struct dfavm_op rewrite1 = *next, rewrite2 = **opp;  // swapped
				int ok = 1;

				// invert the condition of current branch
				switch (rewrite2.cmp) {
				case VM_CMP_LT: rewrite1.cmp = VM_CMP_GE; break;
				case VM_CMP_LE: rewrite1.cmp = VM_CMP_GT; break;
				case VM_CMP_EQ: rewrite1.cmp = VM_CMP_NE; break;
				case VM_CMP_GE: rewrite1.cmp = VM_CMP_LT; break;
				case VM_CMP_GT: rewrite1.cmp = VM_CMP_LE; break;
				case VM_CMP_NE: rewrite1.cmp = VM_CMP_EQ; break;

				case VM_CMP_ALWAYS:
				default:
					// something is wrong
					ok = 0;
					break;
				}

				if (ok) {
					rewrite1.cmp_arg = rewrite2.cmp_arg;

					rewrite2.cmp = VM_CMP_ALWAYS;
					rewrite2.cmp_arg = 0;

					**opp = rewrite1;
					*next = rewrite2;
					count++;
					continue;
				}
			}

			opp = &(*opp)->next;
		}

	} while (count > 0);
}

static void
order_basic_blocks(struct dfavm_assembler *a)
{
	size_t i,n;
	struct dfavm_op **opp;
	struct dfavm_op *st;

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
		struct dfavm_op *branches[FSM_SIGMA_COUNT];  /* at most FSM_SIGMA_COUNT branches per state */
		struct dfavm_op *instr;
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
			struct dfavm_op *dest = branches[j-1]->u.br.dest_arg;
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

static const long min_dest_1b = INT8_MIN;
static const long max_dest_1b = INT8_MAX;

static const long min_dest_2b = INT16_MIN;
static const long max_dest_2b = INT16_MAX;

// static const long min_dest_4b = INT32_MIN;
// static const long max_dest_4b = INT32_MAX;

static int
op_encoding_size(struct dfavm_op *op, int max_enc)
{
	int nbytes = 1;

	assert(op != NULL);

	if (op->cmp != VM_CMP_ALWAYS) {
		nbytes++;
	}

	if (op->instr == VM_OP_BRANCH) {
		int32_t rel_dest = op->u.br.rel_dest;
		if (!max_enc && rel_dest >= min_dest_1b && rel_dest <= max_dest_1b) {
			nbytes += 1;
			op->u.br.dest = VM_DEST_SHORT;
		}
		else if (!max_enc && rel_dest >= min_dest_2b && rel_dest <= max_dest_2b) {
			nbytes += 2;
			op->u.br.dest = VM_DEST_NEAR;
		}
		else {
			// need 32 bits
			nbytes += 4;
			op->u.br.dest = VM_DEST_FAR;
		}
	}

	return nbytes;
}

static void
assign_rel_dests(struct dfavm_assembler *a)
{
	uint32_t off;
	struct dfavm_op *op;

	assert(a != NULL);
	assert(a->linked != NULL);

	/* start with maximum branch encoding */
	off = 0;
	for (op = a->linked; op != NULL; op = op->next) {
		int nenc;

		nenc = op_encoding_size(op, 1);

		assert(nenc > 0 && nenc <= 6);

		op->offset = off;
		op->num_encoded_bytes = nenc;
		off += nenc;
	}

	a->nbytes = off;

	/* iterate until we converge */
	for (;;) {
		int nchanged = 0;

		for (op = a->linked; op != NULL; op = op->next) {
			if (op->instr == VM_OP_BRANCH) {
				struct dfavm_op *dest;
				int64_t diff;
				int nenc;

				dest = op->u.br.dest_arg;

				assert(dest != NULL);

				diff = (int64_t)dest->offset - (int64_t)op->offset;

				assert(diff >= INT32_MIN && diff <= INT32_MAX);

				op->u.br.rel_dest = diff;
				
				nenc = op_encoding_size(op, 0);
				if (nenc != op->num_encoded_bytes) {
					op->num_encoded_bytes = nenc;
					nchanged++;
				}
			}
		}

		if (nchanged == 0) {
			break;
		}

		/* adjust offsets */
		off = 0;
		for (op = a->linked; op != NULL; op = op->next) {
			op->offset = off;
			off += op->num_encoded_bytes;
		}

		a->nbytes = off;
	}
}

static struct fsm_dfavm *
encode_opasm(struct dfavm_assembler *a)
{
	static const struct fsm_dfavm zero;

	struct fsm_dfavm *vm;
	size_t total_bytes;
	size_t off;
	struct dfavm_op *op;
	unsigned char *enc;

	vm = malloc(sizeof *vm);
	*vm = zero;

	total_bytes = a->nbytes;

	enc = malloc(total_bytes);

	vm->ops = enc;
	vm->len = total_bytes;

	for (off = 0, op = a->linked; op != NULL; op = op->next) {
		unsigned char bytes[6];
		unsigned char cmp_bits, instr_bits, rest_bits;
		int nb = 1;

		cmp_bits   = 0;
		instr_bits = 0;
		rest_bits  = 0;

		cmp_bits = op->cmp;
		if (cmp_bits > 7 || cmp_bits < 0) {
			goto error;
		}

		if (op->cmp != VM_CMP_ALWAYS) {
			bytes[1] = op->cmp_arg;
			nb++;
		}

		switch (op->instr) {
		case VM_OP_STOP:
			instr_bits = 0x0;
			rest_bits  = (op->u.stop.end_bits == VM_END_SUCC) ? 0x1 : 0x0;
			break;

		case VM_OP_FETCH:
			instr_bits = 0x1;
			rest_bits  = (op->u.fetch.end_bits == VM_END_SUCC) ? 0x1 : 0x0;
			break;

		case VM_OP_BRANCH:
			instr_bits = 0x2;
			switch (op->u.br.dest) {
			case VM_DEST_SHORT:
				{
					int8_t dst;

					assert(op->u.br.rel_dest >= INT8_MIN);
					assert(op->u.br.rel_dest <= INT8_MAX);

					rest_bits = 0;

				       	dst = op->u.br.rel_dest;
					memcpy(&bytes[nb], &dst, sizeof dst);
					nb += sizeof dst;
				}
				break;

			case VM_DEST_NEAR:
				{
					int16_t dst;

					assert(op->u.br.rel_dest >= INT16_MIN);
					assert(op->u.br.rel_dest <= INT16_MAX);

					rest_bits = 1;

				       	dst = op->u.br.rel_dest;
					memcpy(&bytes[nb], &dst, sizeof dst);
					nb += sizeof dst;
				}
				break;

			case VM_DEST_FAR:
				{
					int32_t dst;

					assert(op->u.br.rel_dest >= INT32_MIN);
					assert(op->u.br.rel_dest <= INT32_MAX);

					rest_bits = 2;

				       	dst = op->u.br.rel_dest;
					memcpy(&bytes[nb], &dst, sizeof dst);
					nb += sizeof dst;
				}
				break;

			case VM_DEST_NONE:
			default:
				goto error;
			}

			break;

		default:
			goto error;
		}

		bytes[0] = (cmp_bits << 5) | (instr_bits << 3) | (rest_bits);
#if DEBUG_ENCODING
		fprintf(stderr, "enc[%4zu] instr=0x%02x cmp=0x%02x rest=0x%02x b=0x%02x\n",
			off, instr_bits, cmp_bits, rest_bits, bytes[0]);
#endif /* DEBUG_ENCODING */

		memcpy(&enc[off], bytes, nb);
		off += nb;

		assert(off <= total_bytes);
	}

	assert(off == total_bytes);

	return vm;

error:
	/* XXX - cleanup */
	return NULL;
}

static void
dump_states(FILE *f, struct dfavm_assembler *a)
{
	struct dfavm_op *op;
	size_t count;

	count = 0;
	for (op = a->linked; op != NULL; op = op->next) {
		if (op->instr == VM_OP_FETCH) {
			unsigned state = op->u.fetch.state;
			fprintf(f, "%6s |    ; state %u %p %s\n", "", state, (void *)op, (state == a->start) ? "(start)" : "");
		}

		fprintf(f, "%6zu |    ", count++);
		print_op(f, op);
	}

	fprintf(f, "%6lu total bytes\n", (unsigned long)a->nbytes);
}

static struct fsm_dfavm *
dfavm_compile(struct ir *ir)
{
	static const struct dfavm_assembler zero;

	struct fsm_dfavm *vm;
	struct dfavm_assembler a;
	size_t nstates;

	nstates = ir->n;
	a = zero;

	a.nstates = nstates;
	a.start = ir->start;

	a.ops = calloc(nstates, sizeof a.ops[0]);
	if (a.ops == NULL) {
		goto error;
	}

	if (initial_translate(ir, &a) < 0) {
		goto error;
	}

	fixup_dests(&a);

	order_basic_blocks(&a);
	eliminate_unnecessary_branches(&a);

	assign_rel_dests(&a);

	dump_states(stdout, &a);

	vm = encode_opasm(&a);
	if (vm == NULL) {
		goto error;
	}

	dfavm_opasm_finalize(&a);

	return vm;

error:
	dfavm_opasm_finalize(&a);
	return NULL;
}

struct fsm_dfavm *
fsm_vm_compile(const struct fsm *fsm)
{
	struct ir *ir;
	struct fsm_dfavm *vm;

	ir = make_ir(fsm);
	if (ir == NULL) {
		return NULL;
	}

	vm = dfavm_compile(ir);

	if (vm == NULL) {
		int errsv = errno;
		free_ir(fsm,ir);
		errno = errsv;

		return NULL;
	}

	free_ir(fsm,ir);
	return vm;

}

void
fsm_vm_free(struct fsm_dfavm *vm)
{
	(void)vm;
}

enum dfavm_state {
	VM_FAIL     = -1,
	VM_MATCHING =  0,
	VM_SUCCESS  =  1,
};

struct vm_state {
	uint32_t pc;
	enum dfavm_state state;
	int fetch_state;
};

static uint32_t
running_print_op(const unsigned char *ops, uint32_t pc, const char *sp, const char *buf, size_t n, char ch, FILE *f) {
	int op, cmp, end, dest;
	unsigned char b = ops[pc];
	op = (b >> 3) & 0x03;
	cmp = (b>>5)  & 0x07;
	dest = b & 0x03;
	end  = b & 0x01;

	fprintf(f, "[%4lu sp=%zd n=%zu ch=%3u '%c' end=%d] ",
		(unsigned long)pc, sp-buf, n, (unsigned char)ch, isprint(ch) ? ch : ' ', end);

	switch (op) {
	case VM_OP_FETCH:
		fprintf(f, "FETCH%s\n", (end == VM_END_FAIL) ? "F" : "S");
		break;

	case VM_OP_STOP:
		fprintf(f, "STOP%s%s", (end == VM_END_FAIL) ? "F" : "S", cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			int arg = ops[++pc];
			if (isprint(arg)) {
				fprintf(f, " '%c'", arg);
			} else {
				fprintf(f, " %d", arg);
			}
		}
		fprintf(f, "\n");
		break;

	case VM_OP_BRANCH:
		fprintf(f, "BR%s%s", (dest == 0 ? "S" : (dest == 1 ? "N" : "F")), cmp_name(cmp));
		if (cmp != VM_CMP_ALWAYS) {
			int arg = ops[++pc];
			if (isprint(arg)) {
				fprintf(f, " '%c',", arg);
			} else {
				fprintf(f, " %d,", arg);
			}
		}


		{
			int32_t rel;
			unsigned char bytes[4] = { 0, 0, 0, 0 };

			if (dest == 0) {
				union {
					uint8_t u;
					int8_t  i;
				} packed;

				packed.u = ops[pc+1];
				bytes[0] = packed.u;
				rel = packed.i;
				pc += 1;
			} else if (dest == 1) {
				union {
					uint16_t u;
					int16_t  i;
				} packed;

				packed.u = (ops[pc+1] | (ops[pc+2] << 8));
				bytes[0] = ops[pc+1];
				bytes[1] = ops[pc+2];
				rel = packed.i;
				pc += 2;
			} else {
				union {
					uint32_t u;
					int32_t  i;
				} packed;
				packed.u = ops[pc+1] | (ops[pc+2] << 8) | (ops[pc+3] << 16) | (ops[pc+4] << 24);
				bytes[0] = ops[pc+1];
				bytes[1] = ops[pc+2];
				bytes[2] = ops[pc+3];
				bytes[3] = ops[pc+4];
				rel = packed.i;
				pc += 4;
			}

			fprintf(f, " %ld 0x%02x 0x%02x 0x%02x 0x%02x\n",
				(long)rel, bytes[0], bytes[1], bytes[2], bytes[3]);
		}
		break;

	default:
		fprintf(f, "UNK\n");
	}

	return pc;
}

static enum dfavm_state
vm_match(const struct fsm_dfavm *vm, struct vm_state *st, const char *buf, size_t n)
{
	const char *sp, *last;
	int ch;

	if (st->state != VM_MATCHING) {
		return st->state;
	}

	ch   = 0;
	sp   = buf;
	last = buf + n;

	for (;;) {
		int op;
		unsigned char b;

		/* decode instruction */
		b = vm->ops[st->pc];
		op = (b>>3)&0x03;

#if DEBUG_VM_EXECUTION
		running_print_op(vm->ops, st->pc, sp, buf, n, ch, stderr);
#endif /* DEBUG_VM_EXECUTION */

		if (op == VM_OP_FETCH) {
			int end;

			end = b & 0x01;
			if (sp >= last) {
				st->fetch_state = end;
				return VM_MATCHING;
			}

			ch = (unsigned char) *sp++;
			st->pc++;
		} else {
			int cmp, end, dest, dest_nbytes;
			int result;
			uint32_t off;

			/* either STOP or BRANCH */
			off = st->pc+1;
			cmp  = b >> 5;

			result = 0;
			if (cmp == VM_CMP_ALWAYS) {
				result = 1;
			} else {
				int arg;

				arg = vm->ops[off++];

				switch (cmp) {
				case VM_CMP_LT: result = ch <  arg; break;
				case VM_CMP_LE: result = ch <= arg; break;
				case VM_CMP_GE: result = ch >= arg; break;
				case VM_CMP_GT: result = ch >  arg; break;
				case VM_CMP_EQ: result = ch == arg; break;
				case VM_CMP_NE: result = ch != arg; break;
				}
			}

			dest = b & 0x3;
			dest_nbytes = 1 << dest;

			if (result) {
				int32_t rel;

				if (op == VM_OP_STOP) {
					end = b & 0x01;
					st->state = (end == VM_END_FAIL) ? VM_FAIL : VM_SUCCESS;
					return st->state;
				}

				if (dest == 0) {
					rel = (int8_t)(vm->ops[off++]);
				} else if (dest == 1) {
					union {
						uint16_t u;
						int16_t  i;
					} packed;

					packed.u = (vm->ops[off+0] | (vm->ops[off+1] << 8));
					rel = packed.i;
					off += 2;
				} else {
					union {
						uint32_t u;
						int32_t  i;
					} packed;
					packed.u = vm->ops[off+0] | (vm->ops[off+1] << 8) | (vm->ops[off+2] << 16) | (vm->ops[off+3] << 24);
					rel = packed.i;
				}

				st->pc += rel;
			} else if (op == VM_OP_BRANCH) {
				st->pc = off + dest_nbytes;
			} else {
				st->pc = off;
			}
		}
	}

	return VM_FAIL;
}

static enum dfavm_state 
vm_end(const struct fsm_dfavm *vm, struct vm_state *st)
{
	(void)vm;

#if DEBUG_VM_EXECUTION
	fprintf(stderr, "END: st->state = %d, st->fetch_state = %d\n",
		st->state, st->fetch_state);
#endif /* DEBUG_VM_EXECUTION */

	if (st->state != VM_MATCHING) {
		return st->state;
	}

	return (st->fetch_state == VM_END_SUCC) ? VM_SUCCESS : VM_FAIL;
}

static const struct vm_state state_init;

int
fsm_vm_match_file(const struct fsm_dfavm *vm, FILE *f)
{
	struct vm_state st = state_init;
	enum dfavm_state result;
	char buf[4096];

	result = VM_FAIL;
	for (;;) {
		size_t nb;

		nb = fread(buf, 1, sizeof buf, f);
		if (nb == 0) {
			break;
		}

		result = vm_match(vm, &st, buf, nb);
		if (result != VM_MATCHING) {
			return result == VM_SUCCESS;
		}
	}

	if (ferror(f)) {
		return 0;
	}

	return vm_end(vm, &st) == VM_SUCCESS;
}

int
fsm_vm_match_buffer(const struct fsm_dfavm *vm, const char *buf, size_t n)
{
	struct vm_state st = state_init;
	enum dfavm_state result;

	vm_match(vm, &st, buf, n);
	result = vm_end(vm, &st);

	return result == VM_SUCCESS;
}
