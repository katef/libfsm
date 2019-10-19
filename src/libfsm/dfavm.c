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

#include <fsm/fsm.h>
#include "internal.h"
#include "print/ir.h"

// Instructions are 1-6 bytes each.  First byte is the opcode.  Second is the
// (optional) argument.  Additional bytes encode the branch destination.
//
// There are four instructions:
// BRANCH, MATCH, and STOP.  The opcodes for each are:
//
//          76543210
// BR       CCC1DDDD
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
//   001  less than or equal to
//   010  greater than or equal to
//   011  equal to
//   100  not equal to
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
//   DDDD
//   0AAA   11-bit destination, 1-byte destination argument
//   10AA   18-bit destination, 2-byte destination argument
//   110x   32-bit destination, 4-byte destination argument, LSB ignored
//
//   for 110x, x should be 0.  x=1 is reserved for future use
//   111x is also reserved for future use for x=0 and x=1.
//

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
	VM_CMP_LE     = 1,
	VM_CMP_GE     = 2,
	VM_CMP_EQ     = 3,
	VM_CMP_NE     = 4,
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
};

enum dfavm_state {
	VM_STOPPED  = 0,
	VM_MATCHING = 1,
};

struct dfavm_op_pool {
	struct dfavm_op_pool *next;

	unsigned int top;
	struct dfavm_op ops[1024];
};

struct dfavm_assembler {
	struct dfavm_op_pool *pool;

	struct dfavm_op **ops;
	struct dfavm_op *linked;

	size_t nstates;
	size_t start;

	uint32_t nbytes;
	uint32_t count;
};

struct fsm_dfavm {
	unsigned char *sp;
	unsigned char *end;

	unsigned char *ops;
	uint32_t len;

	uint32_t pc;
	enum dfavm_state state;
};

static void
print_op(FILE *f, struct dfavm_op *op)
{
	const char *cmp;
	int nargs = 0;

	switch (op->cmp) {
	case VM_CMP_ALWAYS: cmp = "";   break;
	case VM_CMP_LE:     cmp = "LE"; break;
	case VM_CMP_EQ:     cmp = "EQ"; break;
	case VM_CMP_GE:     cmp = "GE"; break;
	case VM_CMP_NE:     cmp = "NE"; break;
	default:            cmp = "??"; break;
	}

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

static struct dfavm_op *
opasm_new(struct dfavm_assembler *a, enum dfavm_instr_bits instr, enum dfavm_cmp_bits cmp, unsigned char arg)
{
	struct dfavm_op *op;

	op = pool_newop(&a->pool);
	if (op != NULL) {
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
	(void)a;
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
	} mode;
};

static void
analyze_table(struct dfa_table *table)
{
	int i, nerr, ndst, max_run;
	int run_lo, run_hi;
	int dst_ind_lo, dst_ind_hi;
	long lo, dlo, run_to;

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
		}

		if (i == FSM_SIGMA_COUNT || dst != dlo) {
			int len = i - lo;

			if (len > max_run) {
				max_run = len;
				run_lo  = lo;
				run_hi  = i-1;
				run_to  = dlo;
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
		table->mode.to = run_to;
		table->mode.syms.start = run_lo;
		table->mode.syms.end   = run_hi;
	}
}

static int
initial_translate_table(struct dfavm_assembler *a, struct dfa_table *table, struct dfavm_op **opp)
{
	int i,lo;

	assert(table != NULL);

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
	}

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
reorder_basic_blocks(struct dfavm_assembler *a)
{
	size_t i,n;
	struct dfavm_op **opp;

	/* replace this with something that actually
	 * orders basic blocks ...
	 */

	opp = &a->linked;
	n = a->nstates;

	/* begin with starting state */
	*opp = a->ops[a->start];
	opp = find_opchain_end(opp);

	assert(opp != NULL);
	assert(*opp == NULL);

	for (i=0; i < n; i++) {
		if (i == a->start) {
			continue;
		}

		*opp = a->ops[i];
		opp = find_opchain_end(opp);

		assert(opp != NULL);
		assert(*opp == NULL);
	}

	/* basic pass to eliminate unnecessary branches; branches to the
	 * next instruction can be elided
	 */
	for (opp = &a->linked; *opp != NULL;) {
		if ((*opp)->instr != VM_OP_BRANCH) {
			opp = &(*opp)->next;
			continue;
		}

		if ((*opp)->u.br.dest_arg != (*opp)->next) {
			opp = &(*opp)->next;
			continue;
		}

		// branch is to next instruction, eliminate
		//
		// condition doesn't matter since both cond and !cond
		// will end up at the same place
		*opp = (*opp)->next;
	}
}

static const long min_dest_1b = -(1L << 10);
static const long max_dest_1b =  (1L << 10)-1;

static const long min_dest_2b = -(1L << 17);
static const long max_dest_2b =  (1L << 17)-1;

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
	}
}

static struct fsm_dfavm *
encode_opasm(struct dfavm_assembler *a)
{
	(void)a;
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

	reorder_basic_blocks(&a);

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

void fsm_vm_free(struct fsm_dfavm *vm)
{
	(void)vm;
}

/* -- Notes --

   1. Need to reorder states like basic blocks.
   2. Start state should be the first state.

 */








