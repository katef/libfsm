/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "re_capvm_compile.h"
#include "../libfsm/capture_vm.h"
#include "../libfsm/capture_vm_program.h"
#include "../libfsm/capture_vm_log.h"

/* for EXPENSIVE_CHECKS */
#include "adt/common.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <ctype.h>

#include <adt/alloc.h>
#include <adt/hash.h>
#include <adt/u64bitset.h>

#include <re/re.h>

#include "ast.h"

#define DEF_OPCODE_CEIL 8
#define DEF_CHARCLASS_BUCKETS 8
#define DEF_CHARCLASS_CEIL 4
#define DEF_REPEATED_ALT_BACKPATCH_CEIL 1
#define NO_BUCKET_ID ((uint32_t)-1)
#define NO_CAPTURE_ID ((uint32_t)-1)

#define LOG_REPETITION_CASES 0

/* Placeholder markers for pending offset values (which would
 * otherwise temporarily be uninitialized memory), chosen so
 * they stand out visually in a debugger. */
enum pending_offset {
	PENDING_OFFSET_REPEAT_OPTIONAL_NEW = 11111111,
	PENDING_OFFSET_REPEAT_OPTIONAL_CONT = 22222222,
	PENDING_OFFSET_ALT_BACKPATCH_JMP = 33333333,
	PENDING_OFFSET_ALT_BACKPATCH_NEW = 44444444,
	PENDING_OFFSET_ALT_BACKPATCH_AFTER_REPEAT_PLUS = 55555555,
};

struct capvm_compile_env {
	const struct fsm_alloc *alloc;
	enum re_flags re_flags;
	struct capvm_program *program;

	uint32_t max_capture_seen;

	/* Hash table for interning character classes.
	 * Doubles and rehashes when half full. */
	struct charclass_htab {
		uint32_t bucket_count;
		uint32_t buckets_used;
		uint32_t ids_used;
		struct charclass_htab_bucket {
			uint32_t id; /* or NO_BUCKET_ID for unused */
			struct capvm_char_class bitset;
		} *buckets;
	} charclass_htab;

#define DEF_REPEATED_GROUPS_CEIL 8
	/* Linked list of nodes used at compile time to compile regexes
	 * such as '^(a((b*)*)*)$' as if they were '^(a(?:b*)(()))$'.
	 * Since the inner body of the repeated subexpression with the
	 * capture groups can be empty, it will always repeat after
	 * its body matches any input. We move the group captures to
	 * the end of the repeated subexpression to explicitly represent
	 * them always capturing afterward, because otherwise the
	 * infinite loop protection skips them. */
	struct repeated_group_info {
		/* Ancestor node that should emit the SAVE opcodes; can
		 * be either a REPEAT or ALT. */
		const struct ast_expr *outermost_ancestor;
		size_t ceil;
		size_t count;
		const struct ast_expr **groups;
		/* linked list */
		struct repeated_group_info *prev;
	} *repeated_groups;
};

static bool
ensure_program_capacity(const struct fsm_alloc *alloc,
    struct capvm_program *p, uint32_t count)
{
#define STRESS_GROWING (EXPENSIVE_CHECKS && 1)

	const uint32_t capacity = p->used + count;

	if (capacity > p->ceil) {
#if STRESS_GROWING
		const uint32_t nceil = (p->ceil + 1 < capacity
		    ? capacity : p->ceil + 1);
#else
		const uint32_t nceil = (p->ceil == 0
		    ? DEF_OPCODE_CEIL
		    : 2*p->ceil);
		/* This should always be enough for any capacity
		 * requested during compilation. */
		assert(nceil >= p->used + count);
#endif
		LOG(3, "%s: growing %u -> %u (count %u)\n",
		    __func__, p->ceil, nceil, count);
		struct capvm_opcode *nops = f_realloc(alloc,
		    p->ops, nceil * sizeof(p->ops[0]));
		if (nops == NULL) {
			return false;
		}

#if EXPENSIVE_CHECKS
		for (size_t i = p->ceil; i < nceil; i++) {
			/* out of range, will trigger asserts */
			nops[i].t = 'X';
		}
#endif

		p->ceil = nceil;
		p->ops = nops;
	}
	return true;
}

static void
check_program_for_invalid_labels(const struct capvm_program *p)
{
	for (uint32_t op_i = 0; op_i < p->used; op_i++) {
		const struct capvm_opcode *op = &p->ops[op_i];
		switch (op->t) {
		case CAPVM_OP_JMP:
			assert(op->u.jmp != op_i);
			break;
		case CAPVM_OP_JMP_ONCE:
			assert(op->u.jmp_once != op_i);
			break;
		case CAPVM_OP_SPLIT:
			assert(op->u.split.greedy < p->used);
			assert(op->u.split.greedy != op_i);
			assert(op->u.split.nongreedy < p->used);
			assert(op->u.split.nongreedy != op_i);
			break;

		case CAPVM_OP_CHAR:
		case CAPVM_OP_CHARCLASS:
		case CAPVM_OP_MATCH:
		case CAPVM_OP_SAVE:
		case CAPVM_OP_ANCHOR:
			break;
		default:
			assert(!"out of range");
			break;
		}
	}
}

static uint32_t
get_program_offset(const struct capvm_program *p)
{
	assert(p->used < p->ceil);

#if EXPENSIVE_CHECKS
	struct capvm_opcode *op = &p->ops[p->used];
	op->t = 'X';		/* out of range */
#endif

	return p->used;
}

static uint32_t
reserve_program_opcode(struct capvm_program *p)
{
	assert(p->used < p->ceil);
	const uint32_t res = p->used;
	p->used++;

#if EXPENSIVE_CHECKS
	struct capvm_opcode *op = &p->ops[res];
	op->t = 'X';		/* out of range */
#endif

	return res;
}

static bool
grow_program_char_classes(const struct fsm_alloc *alloc,
    struct capvm_program *p)
{
	const uint32_t nceil = (p->char_classes.ceil == 0
	    ? DEF_CHARCLASS_CEIL
	    : 2*p->char_classes.ceil);
	struct capvm_char_class *nsets = f_realloc(alloc,
	    p->char_classes.sets, nceil * sizeof(nsets[0]));
	if (nsets == NULL) {
		return false;
	}

	p->char_classes.sets = nsets;
	p->char_classes.ceil = nceil;
	return true;
}

static bool
intern_char_class(struct capvm_compile_env *env,
    struct capvm_program *p, uint64_t chars[4],
    uint32_t *id)
{
	LOG(5, "%s: used %u/%u\n", __func__,
	    env->charclass_htab.buckets_used, env->charclass_htab.bucket_count);
	if (env->charclass_htab.buckets_used >= env->charclass_htab.bucket_count/2) {
		const uint32_t ocount = env->charclass_htab.bucket_count;
		const uint32_t ncount = (ocount == 0
		    ? DEF_CHARCLASS_BUCKETS
		    : 2*env->charclass_htab.bucket_count);
		LOG(3, "%s: growing from %u -> %u\n", __func__, ocount, ncount);
		struct charclass_htab_bucket *nbuckets =
		    f_malloc(env->alloc, ncount * sizeof(nbuckets[0]));
		if (nbuckets == NULL) {
			return false;
		}
		for (uint32_t n_i = 0; n_i < ncount; n_i++) {
			nbuckets[n_i].id = NO_BUCKET_ID;
		}

		const uint32_t nmask = ncount - 1;
		assert((ncount & nmask) == 0);

		struct charclass_htab_bucket *obuckets = env->charclass_htab.buckets;
		for (uint32_t o_i = 0; o_i < ocount; o_i++) {
			if (obuckets[o_i].id == NO_BUCKET_ID) {
				continue;
			}
			const uint64_t h = hash_fnv1a_64((const uint8_t *)obuckets[o_i].bitset.octets,
			    sizeof(obuckets[o_i].bitset));

			for (uint32_t n_i = 0; n_i < ncount; n_i++) {
				const uint64_t b = (h + n_i) & nmask;
				if (nbuckets[b].id == NO_BUCKET_ID) {
					memcpy(&nbuckets[b].bitset,
					    &obuckets[o_i].bitset,
					    sizeof(obuckets[o_i].bitset));
					nbuckets[b].id = obuckets[o_i].id;
					break;
				}
			}
		}
		f_free(env->alloc, obuckets);
		env->charclass_htab.bucket_count = ncount;
		env->charclass_htab.buckets = nbuckets;
	}

	assert(env->charclass_htab.buckets_used < env->charclass_htab.bucket_count/2);

	const uint32_t count = env->charclass_htab.bucket_count;
	const uint32_t mask = count - 1;
	struct charclass_htab_bucket *buckets = env->charclass_htab.buckets;

	const uint64_t h = hash_fnv1a_64((const uint8_t *)chars,
	    sizeof(buckets[0].bitset));
	for (uint32_t i = 0; i < count; i++) {
		const uint64_t b = (h + i) & mask;
		LOG(5, "%s: buckets[%lu].id == %d\n",
		    __func__, b, buckets[b].id);
		if (buckets[b].id == NO_BUCKET_ID) {
			memcpy(&buckets[b].bitset, chars, sizeof(buckets[b].bitset));
			if (p->char_classes.count == p->char_classes.ceil) {
				if (!grow_program_char_classes(env->alloc, p)) {
					return false;
				}
			}

			memcpy(&p->char_classes.sets[p->char_classes.count],
				chars, sizeof(buckets[b].bitset));
			p->char_classes.count++;
			buckets[b].id = env->charclass_htab.ids_used;
			env->charclass_htab.ids_used++;
			env->charclass_htab.buckets_used++;
			*id = buckets[b].id;

			return true;
		} else if (0 == memcmp(chars, &buckets[b].bitset, sizeof(buckets[b].bitset))) {
			*id = buckets[b].id;
			return true; /* already present, reuse */
		} else {
			/* collision */
		}
	}

	assert(!"unreachable");
	return false;
}

static void
dump_endpoint(const struct ast_endpoint *e)
{
	switch (e->type) {
	case AST_ENDPOINT_LITERAL:
		fprintf(stderr, "endpoint[LITERAL]: 0x%02x '%c'\n",
		    e->u.literal.c,
		    isprint(e->u.literal.c) ? e->u.literal.c : '.');
		break;
	case AST_ENDPOINT_CODEPOINT:
		fprintf(stderr, "endpoint[CODEPOINT]: 0x%x\n",
		    e->u.codepoint.u);
		break;
	case AST_ENDPOINT_NAMED:
		assert(!"todo?");
		break;
	}
}

static void
dump_pos(const struct ast_pos *p)
{
	fprintf(stderr, "pos: byte %u, line %u, col %u\n",
	    p->byte, p->line, p->col);
}

static bool
active_node(const struct ast_expr *n)
{
	assert(n != NULL);

	switch (n->type) {
	case AST_EXPR_TOMBSTONE:
		return false;
	default:
		return !(n->flags & AST_FLAG_UNSATISFIABLE);
	}
}

static bool
subtree_represents_character_class(const struct ast_expr *expr, uint64_t cc[4])
{
	for (size_t i = 0; i < 4; i++) {
		cc[i] = 0;
	}

	switch (expr->type) {
	case AST_EXPR_EMPTY:
		/* empty set */
		return false;

	case AST_EXPR_LITERAL:
		u64bitset_set(cc, (uint8_t)expr->u.literal.c);
		return true;

	case AST_EXPR_RANGE:
	{
		const struct ast_endpoint *f = &expr->u.range.from;
		const struct ast_endpoint *t = &expr->u.range.to;
		if (f->type != AST_ENDPOINT_LITERAL
		    || t->type != AST_ENDPOINT_LITERAL) {
			return false;
		}
		for (uint64_t c = (uint8_t)f->u.literal.c; c <= (uint8_t)t->u.literal.c; c++) {
			u64bitset_set(cc, (uint8_t)c);
		}
		return true;
	}

	case AST_EXPR_ALT:
	{
		/* union character classes from children */
		assert(expr->u.alt.count > 0);
		for (size_t c_i = 0; c_i < expr->u.alt.count; c_i++) {
			uint64_t child_cc[4];
			const struct ast_expr *child = expr->u.alt.n[c_i];
			if (subtree_represents_character_class(child, child_cc)) {
				for (size_t cc_i = 0; cc_i < 4; cc_i++) {
					cc[cc_i] |= child_cc[cc_i];
				}
			} else {
				return false;
			}
		}
		return true;
	}

	case AST_EXPR_SUBTRACT:
	{
		/* Only support AST_EXPR_SUBTRACT nodes where .a is a
		 * RANGE:0x00-0xff and .b is either a LITERAL, RANGE, EMPTY,
		 * or an ALT that itself represents a character class, */

		const struct ast_expr *sub_a = expr->u.subtract.a;
		if (sub_a->type != AST_EXPR_RANGE) {
			return false;
		}

		const struct ast_endpoint *f = &sub_a->u.range.from;
		const struct ast_endpoint *t = &sub_a->u.range.to;
		if (f->type != AST_ENDPOINT_LITERAL || t->type != AST_ENDPOINT_LITERAL) {
			return false;
		}

		for (uint64_t i = 0; i < 256; i++) {
			if (i >= (uint8_t)f->u.literal.c && i <= (uint8_t)f->u.literal.c) {
				u64bitset_set(cc, i);
			}
		}

		for (size_t i = 0; i < 4; i++) {
			cc[i] = ~(uint64_t)0;
		}

		uint64_t neg_cc[4];
		if (expr->u.subtract.b->type == AST_EXPR_EMPTY) {
			for (size_t cc_i = 0; cc_i < 4; cc_i++) {
				neg_cc[cc_i] = (uint64_t)0;
			}
		} else if (subtree_represents_character_class(expr->u.subtract.b, neg_cc)) {
			for (size_t cc_i = 0; cc_i < 4; cc_i++) {
				cc[cc_i] &=~ neg_cc[cc_i];
			}
		} else {
			return false;
		}
		return true;
	}

 	default:
		return false;
	}
}

static void
make_charclass_case_insensitive(uint64_t *cc)
{
	for (size_t i = 0; i < 256; i++) {
		if (isalpha(i) && u64bitset_get(cc, i)) {
			const char c = (char)i;
			const uint64_t cl = (uint64_t)tolower(c);
			const uint64_t cu = (uint64_t)toupper(c);
			u64bitset_set(cc, cl);
			u64bitset_set(cc, cu);
		}
	}
}

static bool
can_safely_skip_JMP_ONCE(const struct ast_expr *expr)
{
	/* There are potentially cases where it's safe to skip the
	 * JMP_ONCE special case, which would save memory by not
	 * expanding the path an extra bit per iteration, but the
	 * criteria are subtle enough that it can probably wait. */
	(void)expr;
	return false;
}

static bool
push_repeated_group_info(struct capvm_compile_env *env, const struct ast_expr *expr)
{
	LOG(3 - LOG_REPETITION_CASES,
	    "%s: setting env->repeated_groups.outermost_ancestor <- %p\n",
	    __func__, (void *)expr);

	assert(expr != NULL);
	assert(expr->type == AST_EXPR_REPEAT || expr->type == AST_EXPR_ALT);

	struct repeated_group_info *rgi = f_calloc(env->alloc, 1, sizeof(*rgi));
	if (rgi == NULL) {
		return false;
	}
	rgi->outermost_ancestor = expr;
	rgi->prev = env->repeated_groups;
	env->repeated_groups = rgi;
	LOG(3 - LOG_REPETITION_CASES,
	    "%s: push rgi, allocated %p, prev %p\n",
	    __func__, (void *)rgi, (void *)rgi->prev);
	return true;
}

static void
pop_repeated_group_info(struct capvm_compile_env *env, const struct ast_expr *expr)
{
	assert(expr != NULL);
	assert(expr->type == AST_EXPR_REPEAT || expr->type == AST_EXPR_ALT);
	struct repeated_group_info *rgi = env->repeated_groups;
	LOG(3 - LOG_REPETITION_CASES,
	    "%s: pop rgi, expecting %p, got %p\n",
	    __func__, (void *)expr, (void *)rgi->outermost_ancestor);
	assert(rgi->outermost_ancestor == expr);
	struct repeated_group_info *prev = rgi->prev;
	LOG(3 - LOG_REPETITION_CASES,
	    "%s: pop rgi, freeing %p, prev %p\n",
	    __func__, (void *)rgi, (void *)prev);

	env->repeated_groups = prev;
	if (rgi->groups != NULL) {
		f_free(env->alloc, rgi->groups);
	}
	f_free(env->alloc, rgi);
}

static bool
emit_repeated_groups(struct capvm_compile_env *env, struct capvm_program *p);

static bool
capvm_compile_iter_save_groups_in_skipped_subtree(struct capvm_compile_env *env,
	struct capvm_program *p, const struct ast_expr *expr);

static bool
compile_kleene_star(struct capvm_compile_env *env,
    struct capvm_program *p, const struct ast_expr *expr);

static bool
capvm_compile_iter(struct capvm_compile_env *env,
	struct capvm_program *p, const struct ast_expr *expr)
{
	LOG(4, "%s: expr %p, type %s, %u/%u used, re_flags 0x%02x\n",
	    __func__, (void *)expr, ast_node_type_name(expr->type),
	    p->used, p->ceil, expr->re_flags);

	switch (expr->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_TOMBSTONE:
		break;
	case AST_EXPR_CONCAT:
		for (size_t i = 0; i < expr->u.concat.count; i++) {
			/* append instructions from each consecutive node */
			const struct ast_expr *n = expr->u.concat.n[i];
			if (!capvm_compile_iter(env, p, n)) { return false; }
		}
		break;
	case AST_EXPR_ALT:
	{
		if (!ensure_program_capacity(env->alloc, p, 1)) {
			return false;
		}
		assert(expr->u.alt.count > 0);

		if (expr->u.alt.contains_empty_groups) {
			if (!push_repeated_group_info(env, expr)) {
				return false;
			}
		}

		/* If this ALT node represents a character class (such as a
		 * rewritten . character's ALT[0x00 - 0x09, 0x0b - 0xff] or
		 * a rewritten [abc-ef]'s ... , then produce the corresponding
		 * character class literal. The direct representation of the
		 * subtree would take several instructions and introduce
		 * unnecessary splits, increasing memory usage at runtime. */
		uint64_t cc[4];
		if (subtree_represents_character_class(expr, cc)) {
			const uint32_t pos = reserve_program_opcode(p);
			struct capvm_opcode *op_cc = &p->ops[pos];
			op_cc->t = CAPVM_OP_CHARCLASS;

			if (expr->re_flags & RE_ICASE) {
				make_charclass_case_insensitive(cc);
			}
			if (!intern_char_class(env, p, cc, &op_cc->u.charclass_id)) {
				return false;
			}

			if (expr->u.alt.contains_empty_groups) {
				pop_repeated_group_info(env, expr);
			}
			break;
		}

		uint32_t active_count = 0;
		uint32_t last_active;
		struct alt_flow_info {
			bool is_active;
			uint32_t backpatch;
		};
		struct alt_flow_info *flow_info = f_calloc(env->alloc,
		    expr->u.alt.count, sizeof(flow_info[0]));
		assert(flow_info != NULL);

		for (uint64_t i = 0; i < expr->u.alt.count; i++) {
			const struct ast_expr *n = expr->u.alt.n[i];
			if (active_node(n)) {
				last_active = i;
				active_count++;
				flow_info[i].is_active = true;
			}
		}

		/* If there are no children active this should terminate
		 * with an empty program. */
		LOG(3, "%s: active_count == %d\n", __func__, active_count);
		if (active_count == 0) {
			LOG(3, "%s: active_count == 0, skipping\n", __func__);

			for (uint64_t i = 0; i < expr->u.alt.count; i++) {
				const struct ast_expr *n = expr->u.alt.n[i];
				capvm_compile_iter_save_groups_in_skipped_subtree(env, p, n);
				if (n->flags & AST_FLAG_NULLABLE) {
					break;
				}
			}

			f_free(env->alloc, flow_info);
			if (expr->u.alt.contains_empty_groups) {
				pop_repeated_group_info(env, expr);
			}

			/* FIXME: May need distinct error case to not
			 * leak. There is currently no test reaching
			 * this and the fuzzer has not produced an input
			 * that reaches it -- unsatisfiability has probably
			 * already pruned subtrees that would get here. */
			return true;
		} else if (active_count == 1) {
			/* even if one of the later subtrees is active, an earlier
			 * subtree can still shadow it. */
			bool shadowed = false;

			for (uint64_t i = 0; i < expr->u.alt.count; i++) {
				if (i != last_active) { /* evaluate for empty groups */
					const struct ast_expr *n = expr->u.alt.n[i];
					capvm_compile_iter_save_groups_in_skipped_subtree(env, p, n);
					if (n->flags & AST_FLAG_NULLABLE) {
						shadowed = true;
						break;
					}
				}
			}

			if (!shadowed) {
				LOG(5, "narrowing to last_active %u\n", last_active);
				assert(last_active < expr->u.alt.count);
				const struct ast_expr *n = expr->u.alt.n[last_active];
				if (!capvm_compile_iter(env, p, n)) {
					return false;
				}
				f_free(env->alloc, flow_info);
				if (expr->u.alt.contains_empty_groups) {
					pop_repeated_group_info(env, expr);
				}
				break;
			} else {
				f_free(env->alloc, flow_info);
				if (expr->u.alt.contains_empty_groups) {
					pop_repeated_group_info(env, expr);
				}
				return true; /* may need distinct error case to not leak */
			}
		}

		LOG(3, "%s: compiling AST_EXPR_ALT with %u active nodes, last_active %u\n",
		    __func__, active_count, last_active);

		/* note: binarized split: for a|b|c, treat this like "a else (b else c)",
		 * leading to generated code like:
		 *
		 * // note: trying each case in order, earlier cases are more greedy
		 * - split_cont j1
		 * - split_new j2
		 * j1:
		 * - <case a>
		 * - jmp pos_after_all   // or split pos_after_all, PLUS_BACKPATCH, see below
		 * j2:
		 * - split_cont j3
		 * - split_new j4
		 * j3:
		 * - <case b>
		 * - jmp pos_after_all
		 * j4:
		 * //// DO NOT EMIT split instructions here, treat like a final else
		 * - <case c>
		 * // fall through to pos_after_all
		 * pos_after_all:
		 *
		 *
		 * When an ALT case:
		 * - is nullable (can match the empty string)
		 * - is the first nullable case (shadowing cases after)
		 * - is in a subtree of a REPEAT{1,inf} (+) node whose entire subtree is nullable
		 * then that case's `jmp pos_after_all` should be replaced with
		 * `split pos_after_all pos_after_repeat_backjmp`, which will need a special
		 * label for batch-patching by the REPEAT later. This is necessary for cases
		 * like '^(?:($|x))+$', where the `jmp pos_after_all` would lead to code after
		 * the ALT that has already been executed at the current input position.
		 * */
		for (uint32_t c_i = 0; c_i < expr->u.alt.count; c_i++) {
			const bool is_final_else_case = c_i == last_active;
			LOG(3, "%s: %p c_i %u/%zu, is_final_else_case %d\n",
			    __func__, (void *)expr, c_i, expr->u.alt.count, is_final_else_case);
			if (!flow_info[c_i].is_active) { continue; }

			if (is_final_else_case) {
				/* Just add the case for the child node and
				 * then fall through to pos_after_all. */
				const struct ast_expr *n = expr->u.alt.n[c_i];
				LOG(3, "%s: %p recursing...\n", __func__, (void *)expr);
				if (!capvm_compile_iter(env, p, n)) {
					return false;
				}
				LOG(3, "%s: %p recursing...done (final-else-case)\n", __func__, (void *)expr);

				struct repeated_group_info *rgi = env->repeated_groups;
				LOG(3 - LOG_REPETITION_CASES,
				    "%s: ALT %p: contains_empty_groups: %d, outermost_ancestor: %p == %p ? %d\n",
				    __func__, (void *)expr,
				    expr->u.alt.contains_empty_groups,
				    (void *)(rgi ? rgi->outermost_ancestor : NULL),
				    (void *)expr,
				    (rgi ? rgi->outermost_ancestor == expr : 0));
				if (expr->u.alt.contains_empty_groups) {
					assert(rgi != NULL);
					LOG(3 - LOG_REPETITION_CASES,
					    "%s: outermost_ancestor match, count %zu\n", __func__, rgi->count);
					if (!emit_repeated_groups(env, p)) {
						return false;
					}
				}
			} else {
				if (!ensure_program_capacity(env->alloc, p, 2)) {
					return false;
				}
				const uint32_t pos_split_before_case = reserve_program_opcode(p);
				struct capvm_opcode *op_split_before = &p->ops[pos_split_before_case];
				op_split_before->t = CAPVM_OP_SPLIT;

				/* greedier branch: trying the next case, in order */
				op_split_before->u.split.greedy = get_program_offset(p);

				/* less greedy branch: moving on to the next case.
				 * will backpatch .new to after this case's JMP later */
				op_split_before->u.split.nongreedy = PENDING_OFFSET_ALT_BACKPATCH_NEW;

				const struct ast_expr *n = expr->u.alt.n[c_i];
				LOG(3, "%s: %p recursing...\n", __func__, (void *)expr);
				if (!capvm_compile_iter(env, p, n)) {
					return false;
				}
				LOG(3, "%s: %p recursing...done (non-final)\n", __func__, (void *)expr);

				struct repeated_group_info *rgi = env->repeated_groups;
				LOG(3 - LOG_REPETITION_CASES,
				    "%s: ALT %p: contains_empty_groups: %d, outermost_ancestor: %p == %p ? %d\n",
				    __func__, (void *)expr, expr->u.alt.contains_empty_groups,
				    (void *)(rgi ? rgi->outermost_ancestor : NULL),
				    (void *)expr,
				    (rgi ? rgi->outermost_ancestor == expr : 0));
				if (expr->u.alt.contains_empty_groups) {
					assert(rgi != NULL);
					LOG(3 - LOG_REPETITION_CASES,
					    "%s: outermost_ancestor match, count %zu\n", __func__, rgi->count);
					if (!emit_repeated_groups(env, p)) {
						return false;
					}
				}

				/* JMP or SPLIT, plus space after */
				if (!ensure_program_capacity(env->alloc, p, 2)) {
					return false;
				}

				/* Based on analysis, either emit a JMP or SPLIT. */
				if (n->u.alt.nullable_alt_inside_plus_repeat) {
					const uint32_t pos_split_after = reserve_program_opcode(p);
					flow_info[c_i].backpatch = pos_split_after;
					struct capvm_opcode *op_split_after = &p->ops[pos_split_after];
					op_split_after->t = CAPVM_OP_SPLIT;
					op_split_after->u.split.greedy = PENDING_OFFSET_ALT_BACKPATCH_JMP;
					op_split_after->u.split.nongreedy = PENDING_OFFSET_ALT_BACKPATCH_AFTER_REPEAT_PLUS;
				} else {
					const uint32_t pos_jmp_after = reserve_program_opcode(p);
					flow_info[c_i].backpatch = pos_jmp_after;
					struct capvm_opcode *op_jmp = &p->ops[pos_jmp_after];
					op_jmp->t = CAPVM_OP_JMP;
					op_jmp->u.jmp = PENDING_OFFSET_ALT_BACKPATCH_JMP;
				}

				/* refresh pointer after possible realloc */
				op_split_before = &p->ops[pos_split_before_case];

				/* and the original split jumps to after
				 * this case's JMP */
				op_split_before->u.split.nongreedy = get_program_offset(p);
			}
		}

		/* Ensure there's space for the next instruction, and then
		 * set every case's JMP suffix to it. */
		if (!ensure_program_capacity(env->alloc, p, 1)) {
			return false;
		}
		const uint32_t pos_after_all = get_program_offset(p);

		for (size_t i = 0; i < expr->u.alt.count - 1; i++) {
			const bool is_final_else_case = i == last_active;
			assert(flow_info[i].backpatch < p->used);
			if (is_final_else_case || !flow_info[i].is_active) {
				continue;
			}

			struct capvm_opcode *op_patch = &p->ops[flow_info[i].backpatch];
			if (op_patch->t == CAPVM_OP_JMP) {
				assert(op_patch->u.jmp == PENDING_OFFSET_ALT_BACKPATCH_JMP);
				op_patch->u.jmp = pos_after_all;
			} else if (op_patch->t == CAPVM_OP_SPLIT) {
				assert(op_patch->u.split.greedy == PENDING_OFFSET_ALT_BACKPATCH_JMP);
				op_patch->u.split.greedy = pos_after_all;
				/* This will be patched by an ancestor repeat node after returning. */
				assert(op_patch->u.split.greedy == PENDING_OFFSET_ALT_BACKPATCH_AFTER_REPEAT_PLUS);
			} else {
				assert(!"type mismatch");
			}
		}

		f_free(env->alloc, flow_info);
		if (expr->u.alt.contains_empty_groups) {
			pop_repeated_group_info(env, expr);
		}
		break;
	}
	case AST_EXPR_LITERAL:
	{
		if (!ensure_program_capacity(env->alloc, p, 1)) {
			return false;
		}
		const uint32_t pos = reserve_program_opcode(p);
		struct capvm_opcode *op = &p->ops[pos];

		if (expr->re_flags & RE_ICASE) {
			uint64_t cc[4] = { 0 };
			u64bitset_set(cc, (uint8_t)expr->u.literal.c);

			op->t = CAPVM_OP_CHARCLASS;
			make_charclass_case_insensitive(cc);
			if (!intern_char_class(env, p, cc, &op->u.charclass_id)) {
				return false;
			}
		} else {
			op->t = CAPVM_OP_CHAR;
			op->u.chr = (uint8_t)expr->u.literal.c;
		}
		break;
	}
	case AST_EXPR_CODEPOINT:
		assert(!"not implemented, unreachable");
		break;
	case AST_EXPR_REPEAT:
	{
		const unsigned min = expr->u.repeat.min;
		const unsigned max = expr->u.repeat.max;
		const struct ast_expr *e = expr->u.repeat.e;

		/* collect groups to emit */
		if (expr->u.repeat.contains_empty_groups) {
			if (!push_repeated_group_info(env, expr)) {
				return false;
			}
		}

		if (min == 1 && max == 1) { /* {1,1} */
			/* if repeating exactly once, just defer to subtree,
			 * but still do the repeated_group_info cleanup below */
			if (!capvm_compile_iter(env, p, e)) {
				return false;
			}
		} else if (min == 0 && max == 1) { /* ? */
			/*     split l1, l2
			 * l1: <subtree>
			 * l2: <after> */
			if (!ensure_program_capacity(env->alloc, p, 2)) {
				return false;
			}

			const uint32_t pos_split = reserve_program_opcode(p);
			const uint32_t pos_l1 = get_program_offset(p);

			struct capvm_opcode *op_split = &p->ops[pos_split];
			op_split->t = CAPVM_OP_SPLIT;
			op_split->u.split.greedy = pos_l1;
			op_split->u.split.nongreedy = PENDING_OFFSET_REPEAT_OPTIONAL_NEW;

			if (!capvm_compile_iter(env, p, e)) { return false; }

			if (!ensure_program_capacity(env->alloc, p, 1)) {
				return false;
			}
			op_split = &p->ops[pos_split]; /* refresh pointer */

			const uint32_t after_expr = get_program_offset(p);
			op_split->u.split.nongreedy = after_expr;
		} else if (min == 0 && max == AST_COUNT_UNBOUNDED) { /* * */
			if (!compile_kleene_star(env, p, expr)) {
				return false;
			}
		} else if (min == 1 && max == AST_COUNT_UNBOUNDED) { /* + */
			/* l1: <subtree>
			 *     split l1, l2
			 * l2: <after> */
			if (!ensure_program_capacity(env->alloc, p, 1)) {
				return false;
			}
			const uint32_t pos_l1 = get_program_offset(p);

			if (!capvm_compile_iter(env, p, e)) { return false; }

			if (!ensure_program_capacity(env->alloc, p, 1)) {
				return false;
			}

			/* Only emit the backwards jump for repetition branching
			 * if the subtree added any instructions. */
			if (get_program_offset(p) != pos_l1) {
				if (!ensure_program_capacity(env->alloc, p, 3)) {
					return false;
				}
				const uint32_t pos_split = reserve_program_opcode(p);
				const uint32_t pos_l2 = get_program_offset(p);

				struct capvm_opcode *op_split = &p->ops[pos_split];
				op_split->t = CAPVM_OP_SPLIT;
				op_split->u.split.greedy = pos_l1;
				op_split->u.split.nongreedy = pos_l2;
			}
		} else if (min == 0 && max == 0) { /* {0,0} */
			/* ignored, except any groups contained within that could match
			 * empty input still get emitted (unless unsatisfiable). */
			if (e->flags & AST_FLAG_UNSATISFIABLE) {
				LOG(3, "%s: repeat{0,0} && UNSATISFIABILE -> skipping\n", __func__);
				break;
			}

			/* Unreachable group captures still need to be counted, otherwise
			 * subsequent ones would get shifted down. */
			if (!capvm_compile_iter_save_groups_in_skipped_subtree(env, p, e)) { return false; }
			break;
		} else { 	/* other bounded count */
			/* repeat the minimum number of times */
			for (size_t i = 0; i < min; i++) {
				if (!capvm_compile_iter(env, p, e)) { return false; }
			}

			if (max == AST_COUNT_UNBOUNDED) {
				/* A repeat of {x,inf} should be treated like
				 * (?:subtree){x} (?:subtree)* , where any numbered
				 * capture groups inside have the same group ID in
				 * both copies of the subtree. */
				if (!compile_kleene_star(env, p, expr)) {
					return false;
				}
			} else {
				/* then repeat up to the max as <expr>?
				 *
				 *     split_cont l1
				 *     split_new l2
				 * l1: <subtree>
				 * l2: <after> */
				for (size_t i = min; i < max; i++) {
					if (!ensure_program_capacity(env->alloc, p, 3)) {
						return false;
					}

					const uint32_t pos_split = reserve_program_opcode(p);
					const uint32_t pos_l1 = get_program_offset(p);

					struct capvm_opcode *op_split = &p->ops[pos_split];
					op_split->t = CAPVM_OP_SPLIT;
					op_split->u.split.greedy = pos_l1;
					op_split->u.split.nongreedy = PENDING_OFFSET_REPEAT_OPTIONAL_NEW;

					if (!capvm_compile_iter(env, p, e)) { return false; }

					if (!ensure_program_capacity(env->alloc, p, 1)) {
						return false;
					}
					op_split = &p->ops[pos_split]; /* refresh pointer */

					const uint32_t after_expr = get_program_offset(p);
					op_split->u.split.nongreedy = after_expr;
				}
			}
		}

		struct repeated_group_info *rgi = env->repeated_groups;
		LOG(3 - LOG_REPETITION_CASES,
		    "%s: REPEAT %p: contains_empty_groups: %d, outermost_ancestor: %p == %p ? %d\n",
		    __func__, (void *)expr, expr->u.repeat.contains_empty_groups,
		    (void *)(rgi ? rgi->outermost_ancestor : NULL),
		    (void *)expr,
		    (rgi ? rgi->outermost_ancestor == expr : 0));
		if (expr->u.repeat.contains_empty_groups
		    && rgi != NULL
		    && rgi->outermost_ancestor == expr) {
			LOG(3 - LOG_REPETITION_CASES,
			    "%s: outermost_ancestor match, count %zu\n", __func__, rgi->count);
			if (!emit_repeated_groups(env, p)) {
				return false;
			}
			pop_repeated_group_info(env, expr);
		}

		break;
	}
	case AST_EXPR_GROUP:
	{
		const uint32_t id = expr->u.group.id;
		const int is_repeated = expr->u.group.repeated;

		/* If the group is nullable and repeated, then move its save
		 * instructions to the end, since the final iteration matching
		 * nothing will always clobber any earlier saves. This is a
		 * workaround for cases that would otherwise incorrectly be
		 * halted by infinite loop prevention at runtime. */
		if (is_repeated && ((expr->flags & AST_FLAG_NULLABLE)
			|| !(expr->flags & AST_FLAG_CAN_CONSUME))) {

			struct repeated_group_info *rgi = env->repeated_groups;

			LOG(3 - LOG_REPETITION_CASES,
			    "%s: checking repeated group %u (capvm_compile_iter recurse), parent %p\n",
			    __func__, id, (void *)(rgi ? rgi->outermost_ancestor : NULL));
			if (!capvm_compile_iter(env, p, expr->u.group.e)) { return false; }
			LOG(3 - LOG_REPETITION_CASES,
			    "%s: checking repeated group %u (capvm_compile_iter done), parent %p\n",
			    __func__, id, (void *)(rgi ? rgi->outermost_ancestor : NULL));

			/* don't emit these here, parent repeat node will add them after. */
			if (rgi && rgi->outermost_ancestor != NULL) {
				if (rgi->count == rgi->ceil) {
					const size_t nceil = (rgi->ceil == 0
					    ? DEF_REPEATED_GROUPS_CEIL
					    : 2*rgi->ceil);
					const struct ast_expr **ngroups = f_realloc(env->alloc,
					    rgi->groups,
					    nceil * sizeof(ngroups[0]));
					if (ngroups == NULL) {
						return false;
					}
					rgi->groups = ngroups;
					rgi->ceil = nceil;
				}

				LOG(3 - LOG_REPETITION_CASES,
				    "%s: adding group %u (%p) to outermost_ancestor %p\n",
				    __func__, id, (void *)expr,
				    (void *)rgi->outermost_ancestor);
				rgi->groups[rgi->count] = expr;
				rgi->count++;
			}
		} else {
			if (!ensure_program_capacity(env->alloc, p, 1)) {
				return false;
			}
			const uint32_t pos_start = reserve_program_opcode(p);
			struct capvm_opcode *op = &p->ops[pos_start];
			op->t = CAPVM_OP_SAVE; /* save capture start */
			op->u.save = 2*id;

			if (!capvm_compile_iter(env, p, expr->u.group.e)) { return false; }

			if (!ensure_program_capacity(env->alloc, p, 1)) {
				return false;
			}
			const uint32_t pos_end = reserve_program_opcode(p);
			op = &p->ops[pos_end];
			op->t = CAPVM_OP_SAVE; /* save capture end */
			op->u.save = 2*id + 1;
		}

		if (id > env->max_capture_seen || env->max_capture_seen == NO_CAPTURE_ID) {
			env->max_capture_seen = id;
		}

		break;
	}

	case AST_EXPR_ANCHOR:
	{
		if (!ensure_program_capacity(env->alloc, p, 1)) {
			return false;
		}
		const uint32_t pos = reserve_program_opcode(p);
		struct capvm_opcode *op = &p->ops[pos];
		op->t = CAPVM_OP_ANCHOR;
		op->u.anchor = (expr->u.anchor.type == AST_ANCHOR_START
		    ? CAPVM_ANCHOR_START : CAPVM_ANCHOR_END);
		break;
	}
	case AST_EXPR_SUBTRACT:
	{
		uint64_t cc[4];
		for (size_t i = 0; i < 4; i++) {
			cc[i] = ~(uint64_t)0;
		}
		if (subtree_represents_character_class(expr, cc)) {
			if (!ensure_program_capacity(env->alloc, p, 1)) {
				return false;
			}
			const uint32_t pos = reserve_program_opcode(p);
			struct capvm_opcode *op_cc = &p->ops[pos];
			op_cc->t = CAPVM_OP_CHARCLASS;

			if (expr->re_flags & RE_ICASE) {
				make_charclass_case_insensitive(cc);
			}

			if (!intern_char_class(env, p, cc, &op_cc->u.charclass_id)) {
				return false;
			}
		} else {
			/* FIXME: should return UNSUPPORTED */
			assert(!"unreachable");
			return false;
		}
		break;
	}
	case AST_EXPR_RANGE:
	{
		uint64_t cc[4] = { 0 };
		if (!subtree_represents_character_class(expr, cc)) {
			dump_endpoint(&expr->u.range.from);
			dump_pos(&expr->u.range.start);
			dump_endpoint(&expr->u.range.to);
			dump_pos(&expr->u.range.end);
			assert(!"unreachable");
			return false;
		}

		if (!ensure_program_capacity(env->alloc, p, 1)) {
			return false;
		}
		const uint32_t pos = reserve_program_opcode(p);
		struct capvm_opcode *op = &p->ops[pos];

		op->t = CAPVM_OP_CHARCLASS;
		if (expr->re_flags & RE_ICASE) {
			make_charclass_case_insensitive(cc);
		}

		if (!intern_char_class(env, p, cc, &op->u.charclass_id)) {
			return false;
		}
		break;
	}
	default:
		assert(!"matchfail");
	}

	return true;
}

static bool
compile_kleene_star(struct capvm_compile_env *env,
    struct capvm_program *p, const struct ast_expr *expr)
{
	/* Note: min count may be > 0 because this is also
	 * used for unbounded repetition with a lower count,
	 * as in `a{3,}`, but in that case the {min}
	 * repetitions have already been handled by the caller. */
	assert(expr && expr->type == AST_EXPR_REPEAT &&
	    expr->u.repeat.max == AST_COUNT_UNBOUNDED);

	/* l1: split l2, l3
	 * l2: <subtree>
	 *     jmp_once l1    OR    jmp l1
	 * l3: <after> */
	if (!ensure_program_capacity(env->alloc, p, 2)) {
		return false;
	}

	const uint32_t pos_l1 = reserve_program_opcode(p);
	const uint32_t pos_l2 = get_program_offset(p);

	struct capvm_opcode *op_split = &p->ops[pos_l1];
	op_split->t = CAPVM_OP_SPLIT;
	op_split->u.split.greedy = PENDING_OFFSET_REPEAT_OPTIONAL_CONT;
	op_split->u.split.nongreedy = PENDING_OFFSET_REPEAT_OPTIONAL_NEW;

	if (!capvm_compile_iter(env, p, expr->u.repeat.e)) { return false; }

	if (!ensure_program_capacity(env->alloc, p, 2)) {
		return false;
	}

	/* It's more expensive to always emit JMP_ONCE because it
	 * extends the path each iteration, so we could detect when
	 * it would be safe to use a JMP instead. */
	if (can_safely_skip_JMP_ONCE(expr)) {
		const uint32_t pos_jmp = reserve_program_opcode(p);
		struct capvm_opcode *op_jmp = &p->ops[pos_jmp];
		op_jmp->t = CAPVM_OP_JMP;
		op_jmp->u.jmp = pos_l1;
	} else {
		const uint32_t pos_jmp_once = reserve_program_opcode(p);
		struct capvm_opcode *op_jmp_once = &p->ops[pos_jmp_once];
		op_jmp_once->t = CAPVM_OP_JMP_ONCE;
		op_jmp_once->u.jmp_once = pos_l1;
	}

	const uint32_t pos_l3 = get_program_offset(p);
	op_split = &p->ops[pos_l1]; /* refresh pointer */
	op_split->u.split.greedy = pos_l2;
	op_split->u.split.nongreedy = pos_l3;
	return true;
}

static bool
emit_repeated_groups(struct capvm_compile_env *env, struct capvm_program *p)
{
	struct repeated_group_info *rgi = env->repeated_groups;
	for (size_t i = 0; i < rgi->count; i++) {
		const struct ast_expr *group = rgi->groups[i];
		assert(group->u.group.repeated);
		const unsigned id = group->u.group.id;
		LOG(3 - LOG_REPETITION_CASES,
		    "%s: checking %zu/%zu: group_id %u\n",
		    __func__, i, rgi->count, id);

		if (group->flags & (AST_FLAG_ANCHORED_START | AST_FLAG_ANCHORED_END)) {
			/* if the otherwise empty group contains any anchors,
			 * then emit a subtree like (^)? so that its capture
			 * is only set when the anchors would match. */
			if (!ensure_program_capacity(env->alloc, p, 6)) {
				return false;
			}

			/*     split l1, l2
			 * l1: <optional start anchor>
			 *     <optional end anchor>
			 * l2: save (start)
			 *     save (end)
			 *     <after> */
			const uint32_t pos_split = reserve_program_opcode(p);
			const uint32_t pos_l1 = get_program_offset(p);

			struct capvm_opcode *op_split = &p->ops[pos_split];
			op_split->t = CAPVM_OP_SPLIT;
			op_split->u.split.greedy = pos_l1;
			op_split->u.split.nongreedy = PENDING_OFFSET_REPEAT_OPTIONAL_NEW;

			if (group->flags & AST_FLAG_ANCHORED_START) {
				const uint32_t pos_start = reserve_program_opcode(p);
				struct capvm_opcode *op = &p->ops[pos_start];
				op->t = CAPVM_OP_ANCHOR;
				op->u.anchor = CAPVM_ANCHOR_START;
			}

			if (group->flags & AST_FLAG_ANCHORED_END) {
				const uint32_t pos_end = reserve_program_opcode(p);
				struct capvm_opcode *op = &p->ops[pos_end];
				op->t = CAPVM_OP_ANCHOR;
				op->u.anchor = CAPVM_ANCHOR_END;
			}

			const uint32_t pos_start = reserve_program_opcode(p);
			struct capvm_opcode *op = &p->ops[pos_start];
			op->t = CAPVM_OP_SAVE; /* save capture start */
			op->u.save = 2*id;

			const uint32_t pos_end = reserve_program_opcode(p);
			op = &p->ops[pos_end];
			op->t = CAPVM_OP_SAVE; /* save capture end */
			op->u.save = 2*group->u.group.id;
			op->u.save = 2*id + 1;

			const uint32_t after_expr = get_program_offset(p);
			op_split = &p->ops[pos_split]; /* refresh pointer */
			op_split->u.split.nongreedy = after_expr;
		} else {
			/* simple case, emit SAVE pair */
			if (!ensure_program_capacity(env->alloc, p, 2)) {
				return false;
			}
			const uint32_t pos_start = reserve_program_opcode(p);
			struct capvm_opcode *op = &p->ops[pos_start];
			op->t = CAPVM_OP_SAVE; /* save capture start */
			op->u.save = 2*id;

			const uint32_t pos_end = reserve_program_opcode(p);
			op = &p->ops[pos_end];
			op->t = CAPVM_OP_SAVE; /* save capture end */
			op->u.save = 2*group->u.group.id;
			op->u.save = 2*id + 1;
		}
	}

	/* clear, because an ALT's subtrees can have distinct repeated groups */
	rgi->count = 0;

	return true;
}

static bool
capvm_compile_iter_save_groups_in_skipped_subtree(struct capvm_compile_env *env,
	struct capvm_program *p, const struct ast_expr *expr)
{
	/* Follow the subtree as far as any expressions that could
	 * contain GROUPs. Emit any empty groups. This is necessary for
	 * regexes like /()*^/ and /(x|(x|))^/ whose subtrees are
	 * otherwise pruned but would still match the empty string
	 * before ^. */
	switch (expr->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_ANCHOR:
	case AST_EXPR_SUBTRACT:
	case AST_EXPR_RANGE:
	case AST_EXPR_TOMBSTONE:
		/* none of these can contain groups */
		break;

	case AST_EXPR_CONCAT:
		if (expr->flags & AST_FLAG_UNSATISFIABLE) {
			return true; /* skip */
		}
		for (size_t i = 0; i < expr->u.concat.count; i++) {
			if (!capvm_compile_iter_save_groups_in_skipped_subtree(env, p, expr->u.concat.n[i])) {
				return false;
			}
		}
		break;
	case AST_EXPR_ALT:
		for (size_t i = 0; i < expr->u.alt.count; i++) {
			if (!capvm_compile_iter_save_groups_in_skipped_subtree(env, p, expr->u.alt.n[i])) {
				return false;
			}
		}
		break;

	case AST_EXPR_REPEAT:
		return capvm_compile_iter_save_groups_in_skipped_subtree(env, p, expr->u.repeat.e);

	case AST_EXPR_GROUP:
	{
		const uint32_t id = expr->u.group.id;
		LOG(5, "%s: recording otherwise skipped group %u\n", __func__, id);

		if (!ensure_program_capacity(env->alloc, p, 2)) {
			return false;
		}

		if (id > env->max_capture_seen || env->max_capture_seen == NO_CAPTURE_ID) {
			env->max_capture_seen = id;
		}

		const uint32_t pos_start = reserve_program_opcode(p);
		struct capvm_opcode *op = &p->ops[pos_start];
		op->t = CAPVM_OP_SAVE; /* save capture start */
		op->u.save = 2*id;

		const uint32_t pos_end = reserve_program_opcode(p);
		op = &p->ops[pos_end];
		op->t = CAPVM_OP_SAVE; /* save capture end */
		op->u.save = 2*id + 1;

		if (!capvm_compile_iter_save_groups_in_skipped_subtree(env, p, expr->u.group.e)) {
			return false;
		}

		break;
	}
	default:
		assert(!"match fail");

	}
	return true;
}

static enum re_capvm_compile_ast_res
capvm_compile(struct capvm_compile_env *env,
	const struct ast *ast)
{
	struct capvm_program *p = f_calloc(env->alloc, 1, sizeof(*p));
	if (p == NULL) {
		return RE_CAPVM_COMPILE_AST_ERROR_ALLOC;
	}

	LOG(3, "%s: has_unanchored: start? %d, end? %d\n", __func__,
	    ast->has_unanchored_start,
	    ast->has_unanchored_end);

	/* If the regex has an unanchored start, it gets a `.*` prefix,
	 * but with the labels swapped so that the unanchored start
	 * loop is NOT greedy. */
	if (ast->has_unanchored_start) {
		if (!ensure_program_capacity(env->alloc, p, 4)) {
			return RE_CAPVM_COMPILE_AST_ERROR_ALLOC;
		}

		/* l1: split l3, l2
		 * l2: .
		 *     jmp l1
		 * l3: <after> */
		const uint32_t l1 = get_program_offset(p);
		const uint32_t split_pos = reserve_program_opcode(p);
		struct capvm_opcode *op_split = &p->ops[split_pos];

		const uint32_t l2 = get_program_offset(p);
		const uint32_t op_cc_pos = reserve_program_opcode(p);
		struct capvm_opcode *op_cc = &p->ops[op_cc_pos];

		const uint32_t op_jmp_pos = reserve_program_opcode(p);
		struct capvm_opcode *op_jmp = &p->ops[op_jmp_pos];

		const uint32_t l3 = get_program_offset(p);

		op_split->t = CAPVM_OP_SPLIT;
		op_split->u.split.greedy = l3;
		op_split->u.split.nongreedy = l2;

		op_cc->t = CAPVM_OP_CHARCLASS;
		uint64_t any[4];
		for (size_t i = 0; i < 4; i++) {
			any[i] = ~(uint64_t)0;
		}
		if (!intern_char_class(env, p, any, &op_cc->u.charclass_id)) {
			goto cleanup;
		}

		op_jmp->t = CAPVM_OP_JMP;
		op_jmp->u.jmp = l1;
	}

	/* Compile the regex AST, assuming match group 0 is
	 * explicitly represented. */
	if (!capvm_compile_iter(env, p, ast->expr)) {
		goto cleanup;
	}

	/* Add the unanchored end loop, outside of match group 0 */
	if (ast->has_unanchored_end) {
		if (!ensure_program_capacity(env->alloc, p, 4)) {
			return RE_CAPVM_COMPILE_AST_ERROR_ALLOC;
		}

		/* l1: split l3, l2
		 * l2: .
		 *     jmp l1
		 * l3: <after, will be MATCH> */
		const uint32_t l1 = reserve_program_opcode(p);
		const uint32_t l2 = reserve_program_opcode(p);
		const uint32_t l_jmp = reserve_program_opcode(p);
		const uint32_t l3 = get_program_offset(p);

		struct capvm_opcode *op_split = &p->ops[l1];

		struct capvm_opcode *op_any = &p->ops[l2];
		struct capvm_opcode *op_jmp = &p->ops[l_jmp];

		op_split->t = CAPVM_OP_SPLIT;
		op_split->u.split.greedy = l3;
		op_split->u.split.nongreedy = l2;

		op_any->t = CAPVM_OP_CHARCLASS;
		uint64_t any[4];
		for (size_t i = 0; i < 4; i++) {
			any[i] = ~(uint64_t)0;
		}
		if (!intern_char_class(env, p, any, &op_any->u.charclass_id)) {
			goto cleanup;
		}

		op_jmp->t = CAPVM_OP_JMP;
		op_jmp->u.jmp = l1;
	}

	/* add MATCH opcode at end */
	if (!ensure_program_capacity(env->alloc, p, 1)) {
		return RE_CAPVM_COMPILE_AST_ERROR_ALLOC;
	}
	const uint32_t pos_m = reserve_program_opcode(p);
	struct capvm_opcode *op_m = &p->ops[pos_m];
	op_m->t = CAPVM_OP_MATCH;

	/* TODO: populate info about max threads, etc. in p,
	 * because it should be possible to calculate runtime
	 * memory limits at compile time. */
	env->program = p;
	p->capture_count = (env->max_capture_seen == NO_CAPTURE_ID
	    ? 0 : env->max_capture_seen + 1);

	if (LOG_CAPVM > 2) {
		LOG(0, "====\n");
		fsm_capvm_program_dump(stderr, p);
		LOG(0, "====\n");
	}

	/* TODO: it may be worth exposing these static checks as
	 * something the caller can run at load-time */
	check_program_for_invalid_labels(p);

	return RE_CAPVM_COMPILE_AST_OK;

cleanup:
	fsm_capvm_program_free(env->alloc, p);
	return RE_CAPVM_COMPILE_AST_ERROR_ALLOC;
}

#define DUMP_AST 0
#define DUMP_RESULT 0		/* should be 0 in production */

#if DUMP_AST || DUMP_RESULT
#include <fsm/options.h>
#include "print.h"
static struct fsm_options opt = { .group_edges = 1 };

static unsigned
get_max_capture_id(const struct capvm_program *program)
{
	assert(program != NULL);
	return (program->capture_count == 0
	    ? 0
	    : program->capture_base + program->capture_count - 1);
}

#endif

enum re_capvm_compile_ast_res
re_capvm_compile_ast(const struct fsm_alloc *alloc,
	const struct ast *ast,
	enum re_flags re_flags,
	struct capvm_program **program)
{
#if DUMP_AST
	if (LOG_CAPVM > 2) {
		ast_print_dot(stderr, &opt, re_flags, ast);
		ast_print_tree(stderr, &opt, re_flags, ast);
	}
#endif

	struct capvm_compile_env env = {
		.alloc = alloc,
		.re_flags = re_flags,
		.max_capture_seen = NO_CAPTURE_ID,
	};

	enum re_capvm_compile_ast_res res;
	res = capvm_compile(&env, ast);


	struct repeated_group_info *rgi = env.repeated_groups;
	while (rgi != NULL) {
		struct repeated_group_info *prev = rgi->prev;
		LOG(3 - LOG_REPETITION_CASES,
		    "%s: rgi cleanup, freeing %p, prev %p\n",
		    __func__, (void *)rgi, (void *)prev);

		if (rgi->groups != NULL) {
			f_free(alloc, rgi->groups);
		}
		f_free(alloc, rgi);
		rgi = prev;
	}

	if (res == RE_CAPVM_COMPILE_AST_OK) {
#if DUMP_RESULT > 0
		if (DUMP_RESULT > 1 || getenv("DUMP")) {
			ast_print_tree(stderr, &opt, re_flags, ast);
			fsm_capvm_program_dump(stderr, env.program);
			fprintf(stderr, "%s: max_capture_id %u\n", __func__,
			    get_max_capture_id(env.program));

		}
#endif

		*program = env.program;
	}

	free(env.charclass_htab.buckets);

	return res;
}
