/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define LOG_BITSET 0
#define LOG_BSEARCH 0

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge, and the edges array */

#include <print/esc.h>

#include <adt/alloc.h>
#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#define DEF_EDGE_GROUP_CEIL 1

/* Array of <to, symbols> tuples, sorted by to.
 *
 * Design assumption: It is significantly more likely in practice to
 * have to be more edges with different labels going to the same state
 * than the same symbol going to different states. This does not
 * include epsilon edges, which can be stored in a state_set. */
struct edge_set {
	size_t ceil;		/* nonzero */
	size_t count;		/* <= ceil */
	struct edge_group {
		fsm_state_t to;	/* distinct */
		uint64_t symbols[256/64];
	} *groups;		/* sorted by .to */
};

#define SYMBOLS_SET(S, ID) (S[ID/64] |= (1ULL << (ID & 63)))
#define SYMBOLS_GET(S, ID) (S[ID/64] & (1ULL << (ID & 63)))
#define SYMBOLS_CLEAR(S, ID) (S[ID/64] &=~ (1ULL << (ID & 63)))

struct edge_set *
edge_set_new(void)
{
#if LOG_BITSET
	fprintf(stderr, " -- edge_set_new\n");
#endif
	return NULL;		/* -> empty set */
}

void
edge_set_free(const struct fsm_alloc *a, struct edge_set *set)
{
#if LOG_BITSET
	fprintf(stderr, " -- edge_set_free %p\n", (void *)set);
#endif
	if (set == NULL) {
		return;
	}

	f_free(a, set->groups);
	f_free(a, set);
}

static int
grow_groups(struct edge_set *set, const struct fsm_alloc *alloc)
{
	/* TODO: This could also squash out any groups where
	 * none of the symbols are set anymore. */
	const size_t nceil = 2 *set->ceil;
	struct edge_group *ng;
	assert(nceil > 0);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set grow_groups: %lu -> %lu\n",
	    set->ceil, nceil);
#endif

	ng = f_realloc(alloc, set->groups,
	    nceil * sizeof(set->groups[0]));
	if (ng == NULL) {
		return 0;
	}

	set->ceil = nceil;
	set->groups = ng;

	return 1;
}

static void
dump_edge_set(const struct edge_set *set)
{
	const struct edge_group *eg;
	size_t i;
#if LOG_BITSET < 2
	return;
#endif

	if (edge_set_empty(set)) {
		fprintf(stderr, "dump_edge_set: <empty>\n");
		return;
	}

	fprintf(stderr, "dump_edge_set: %p\n", (void *)set);

	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];
		fprintf(stderr, " -- %zu: [0x%" PRIx64 ", 0x%" PRIx64 ", 0x%" PRIx64 ", 0x%" PRIx64 "] -> %u\n",
		    i,
		    eg->symbols[0], eg->symbols[1],
		    eg->symbols[2], eg->symbols[3], eg->to);
	}
}

static struct edge_set *
init_empty(const struct fsm_alloc *alloc)
{
	struct edge_set *set = f_calloc(alloc, 1, sizeof(*set));
	if (set == NULL) {
		return NULL;
	}

	set->groups = f_malloc(alloc,
	    DEF_EDGE_GROUP_CEIL * sizeof(set->groups[0]));
	if (set->groups == NULL) {
		f_free(alloc, set);
		return NULL;
	}

	set->ceil = DEF_EDGE_GROUP_CEIL;
	set->count = 0;
	return set;
}

int
edge_set_add(struct edge_set **pset, const struct fsm_alloc *alloc,
	unsigned char symbol, fsm_state_t state)
{
	uint64_t symbols[256/64] = { 0 };
	SYMBOLS_SET(symbols, symbol);
	return edge_set_add_bulk(pset, alloc, symbols, state);
}

int
edge_set_advise_growth(struct edge_set **pset, const struct fsm_alloc *alloc,
    size_t count)
{
	struct edge_set *set = *pset;
	if (set == NULL) {
		set = init_empty(alloc);
		if (set == NULL) {
			return 0;
		}
		*pset = set;
	}

	const size_t oceil = set->ceil;

	size_t nceil = 1;
	while (nceil < oceil + count) {
		nceil *= 2;
	}
	assert(nceil > 0);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set advise_growth: %lu -> %lu\n",
	    set->ceil, nceil);
#endif

	struct edge_group *ng = f_realloc(alloc, set->groups,
	    nceil * sizeof(set->groups[0]));
	if (ng == NULL) {
		return 0;
	}

	set->ceil = nceil;
	set->groups = ng;

	return 1;
}

enum fsp_res {
	FSP_FOUND_INSERT_POSITION,
	FSP_FOUND_VALUE_PRESENT,
};

/* Use binary search to find the first position N where set->groups[N].to >= state,
 * which includes the position immediately following the last entry. Return an enum
 * which indicates whether state is already present. */
static enum fsp_res
find_state_position(const struct edge_set *set, fsm_state_t state, size_t *dst)
{
	size_t lo = 0, hi = set->count;
	if (LOG_BSEARCH) {
		fprintf(stderr, "%s: looking for %d in %p (count %zu)\n",
		    __func__, state, (void *)set, set->count);
	}

#if EXPENSIVE_CHECKS
	/* invariant: input is unique and sorted */
	for (size_t i = 1; i < set->count; i++) {
		assert(set->groups[i - 1].to < set->groups[i].to);
	}
#endif

	if (set->count == 0) {
		if (LOG_BSEARCH) {
			fprintf(stderr, "%s: empty, returning 0\n", __func__);
		}
		*dst = 0;
		return FSP_FOUND_INSERT_POSITION;
	} else {
		if (LOG_BSEARCH) {
			fprintf(stderr, "%s: fast path: looking for %d, set->groups[last].to %d\n",
			    __func__, state, set->groups[hi - 1].to);
		}

		/* Check the last entry so we can append in constant time. */
		const fsm_state_t last = set->groups[hi - 1].to;
		if (state > last) {
			*dst = hi;
			return FSP_FOUND_INSERT_POSITION;
		} else if (state == last) {
			*dst = hi - 1;
			return FSP_FOUND_VALUE_PRESENT;
		}
	}

	size_t mid;
	while (lo < hi) {		/* lo <= mid < hi */
		mid = lo + (hi - lo)/2; /* avoid overflow */
		const struct edge_group *eg = &set->groups[mid];
		const fsm_state_t cur = eg->to;
		if (LOG_BSEARCH) {
			fprintf(stderr, "%s: lo %zu, hi %zu, mid %zu, cur %d, looking for %d\n",
			    __func__, lo, hi, mid, cur, state);
		}

		if (state == cur) {
			*dst = mid;
			return FSP_FOUND_VALUE_PRESENT;
		} else if (state > cur) {
			lo = mid + 1;
			if (LOG_BSEARCH) {
				fprintf(stderr, "%s: new lo %zd\n", __func__, lo);
			}

			/* Update mid if we're about to halt, because we're looking
			 * for the first position >= state, not the last position <=. */
			if (lo == hi) {
				mid = lo;
				if (LOG_BSEARCH) {
					fprintf(stderr, "%s: special case, updating mid to %zd\n", __func__, mid);
				}
			}
		} else if (state < cur) {
			hi = mid;
			if (LOG_BSEARCH) {
				fprintf(stderr, "%s: new hi %zd\n", __func__, hi);
			}
		}
	}

	if (LOG_BSEARCH) {
		fprintf(stderr, "%s: halting at %zd (looking for %d, cur %d)\n",
		    __func__, mid, state, set->groups[mid].to);
	}

	/* dst is now the first position > state (== case is handled above),
	 * which may be one past the end of the array. */
	assert(mid == set->count || set->groups[mid].to > state);
	*dst = mid;
	return FSP_FOUND_INSERT_POSITION;
}

int
edge_set_add_bulk(struct edge_set **pset, const struct fsm_alloc *alloc,
	uint64_t symbols[256/64], fsm_state_t state)
{
	struct edge_set *set;
	struct edge_group *eg;
	size_t i;

	assert(pset != NULL);

	set = *pset;

	if (set == NULL) {	/* empty */
		set = init_empty(alloc);
		if (set == NULL) {
			return 0;
		}

		eg = &set->groups[0];
		eg->to = state;
		memcpy(eg->symbols, symbols, sizeof(eg->symbols));
		set->count++;

		*pset = set;

#if LOG_BITSET
		fprintf(stderr, " -- edge_set_add: symbols [0x%lx, 0x%lx, 0x%lx, 0x%lx] -> state %d on empty -> %p\n",
		    symbols[0], symbols[1], symbols[2], symbols[3],
		    state, (void *)set);
#endif
		dump_edge_set(set);

		return 1;
	}

	assert(set->ceil > 0);
	assert(set->count <= set->ceil);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_add: symbols [0x%lx, 0x%lx, 0x%lx, 0x%lx] -> state %d on %p\n",
	    symbols[0], symbols[1], symbols[2], symbols[3],
	    state, (void *)set);
#endif

	switch (find_state_position(set, state, &i)) {
	case FSP_FOUND_VALUE_PRESENT:
		assert(i < set->count);
		eg = &set->groups[i];
		for (i = 0; i < 256/64; i++) {
			eg->symbols[i] |= symbols[i];
		}
		dump_edge_set(set);
		return 1;

		break;
	case FSP_FOUND_INSERT_POSITION:
		break;		/* continue below */
	}

	/* insert/append at i */
	if (set->count == set->ceil) {
		if (!grow_groups(set, alloc)) {
			return 0;
		}
	}

	eg = &set->groups[i];

	if (i < set->count && eg->to != state) {  /* shift down by one */
		const size_t to_mv = set->count - i;

#if LOG_BITSET
		fprintf(stderr, "   --- shifting, count %ld, i %ld, to_mv %ld\n",
		    set->count, i, to_mv);
#endif

		if (to_mv > 0) {
			memmove(&set->groups[i + 1],
			    &set->groups[i],
			    to_mv * sizeof(set->groups[i]));
		}
		eg = &set->groups[i];
	}

	eg->to = state;
	memset(eg->symbols, 0x00, sizeof(eg->symbols));
	memcpy(eg->symbols, symbols, sizeof(eg->symbols));
	set->count++;
	dump_edge_set(set);

	return 1;
}

int
edge_set_add_state_set(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, const struct state_set *state_set)
{
	struct state_iter si;
	fsm_state_t state;

	state_set_reset(state_set, &si);

	while (state_set_next(&si, &state)) {
		if (!edge_set_add(setp, alloc, symbol, state)) {
			return 0;
		}
	}

	return 1;
}

int
edge_set_find(const struct edge_set *set, unsigned char symbol,
	struct fsm_edge *e)
{
	const struct edge_group *eg;
	size_t i;
	if (set == NULL) {
		return 0;
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_find: symbol 0x%x on %p\n",
	    symbol, (void *)set);
#endif

	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];
		if (SYMBOLS_GET(eg->symbols, symbol)) {
			e->state = eg->to;
			e->symbol = symbol;
			return 1;
		}
	}

	return 0;		/* not found */
}

/* This interface exists specifically so that fsm_minimise can use
 * bit parallelelism to speed up some of its partitioning, it should
 * not be used by anything else.
 *
 * In particular, this assumes determinism -- only the FIRST edge_group
 * with the label will match. */
int
edge_set_check_edges_with_EC_mapping(const struct edge_set *set,
    unsigned char label, size_t ec_map_count, fsm_state_t *state_ecs,
    fsm_state_t *to_state, uint64_t labels[256/64])
{
	size_t i;

	if (set == NULL) {
		return 0;
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_check_edges: symbol 0x%x on %p\n",
	    label, (void *)set);
#endif

	memset(labels, 0x00, (256/64)*sizeof(labels[0]));

	/* Find the state the label leads to, if any. */
	const size_t offset = label/64;
	const uint64_t bit = (uint64_t)1 << (label & 63);

	int found = 0;
	fsm_state_t first_ec;

	for (i = 0; i < set->count; i++) {
		const struct edge_group *eg = &set->groups[i];
		if (eg->symbols[offset] & bit) {
			if (!found) {
				assert(eg->to < ec_map_count);
				first_ec = state_ecs[eg->to];
				*to_state = eg->to;
				found = 1;
				break;
			}
		}
	}

	if (!found) {
		return 0;
	}

	/* Second pass: Note labels that lead to other states which
	 * belong to the same EC.
	 *
	 * This has to be done in a second pass because some of the
	 * relevant edge groups may appear before label is found. */
	for (i = 0; i < set->count; i++) {
		const struct edge_group *eg = &set->groups[i];
		assert(eg->to < ec_map_count);

		/* Note other labels leading to states that
		 * map to the same EC. */
		if (state_ecs[eg->to] == first_ec) {
			for (size_t w_i = 0; w_i < 4; w_i++) {
				labels[w_i] |= eg->symbols[w_i];
			}
		}
	}

	return 1;
}

int
edge_set_contains(const struct edge_set *set, unsigned char symbol)
{
	const struct edge_group *eg;
	size_t i;
	if (edge_set_empty(set)) {
		return 0;
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_contains: symbol 0x%x on %p\n",
	    symbol, (void *)set);
#endif

	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];
		if (SYMBOLS_GET(eg->symbols, symbol)) {
			return 1;
		}
	}

	return 0;
}

int
edge_set_hasnondeterminism(const struct edge_set *set, struct bm *bm)
{
	size_t i, w_i;
	assert(bm != NULL);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_hasnondeterminism: on %p\n",
	    (void *)set);
#endif

	dump_edge_set(set);

	if (edge_set_empty(set)) {
		return 0;
	}

	for (i = 0; i < set->count; i++) {
		const struct edge_group *eg = &set->groups[i];
		for (w_i = 0; w_i < 4; w_i++) {
			const uint64_t cur = eg->symbols[w_i];
			size_t b_i;
			if (cur == 0) {
				continue;
			}

#if LOG_BITSET > 1
			fprintf(stderr, " -- eshnd: [0x%lx, 0x%lx, 0x%lx, 0x%lx] + group %ld cur %ld: 0x%lx\n",
			    eg->symbols[0], eg->symbols[1],
			    eg->symbols[2], eg->symbols[3],
			    i, w_i, cur);
#endif

			for (b_i = 0; b_i < 64; b_i++) {
				if (cur & (1ULL << b_i)) {
					const size_t bit = 64*w_i + b_i;
					if (bm_get(bm, bit)) {
#if LOG_BITSET > 1
						fprintf(stderr, "-- eshnd: hit on bit %lu\n", bit);
#endif
						return 1;
					}
					bm_set(bm, bit);
				}
			}
		}
	}

	return 0;
}

int
edge_set_transition(const struct edge_set *set, unsigned char symbol,
	fsm_state_t *state)
{
	/*
	 * This function is meaningful for DFA only; we require a DFA
	 * by contract in order to identify a single destination state
	 * for a given symbol.
	 */
	struct fsm_edge e;
	if (!edge_set_find(set, symbol, &e)) {
		return 0;
	}
	*state = e.state;
	return 1;
}

size_t
edge_set_count(const struct edge_set *set)
{
	size_t res = 0;
	size_t i;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_count: on %p\n",
	    (void *)set);
#endif

	/* This does not call edge_set_empty directly
	 * here, because that does the same scan as below,
	 * it just exits at the first label found. */
	if (set == NULL) {
		return 0;
	}

	for (i = 0; i < set->count; i++) {
		size_t w_i;
		const struct edge_group *eg = &set->groups[i];
		for (w_i = 0; w_i < 4; w_i++) {
			/* TODO: res += popcount64(w) */
			const uint64_t w = eg->symbols[w_i];
			uint64_t bit;
			if (w == 0) {
				continue;
			}
			for (bit = (uint64_t)1; bit; bit <<= 1) {
				if (w & bit) {
					res++;
				}
			}
		}
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_count: %zu\n", res);
#endif
	return res;
}

static int
find_first_group_label(const struct edge_group *g, unsigned char *label)
{
	size_t i, bit;
	for (i = 0; i < 4; i++) {
		if (g->symbols[i] == 0) {
			continue;
		}
		for (bit = 0; bit < 64; bit++) {
			if (g->symbols[i] & ((uint64_t)1 << bit)) {
				*label = i*64 + bit;
				return 1;
			}
		}
	}
	return 0;
}

static int
edge_set_copy_into(struct edge_set **pdst, const struct fsm_alloc *alloc,
	const struct edge_set *src)
{
	/* Because the source and destination edge_set groups are
	 * sorted by .to, we can pairwise bulk merge them. If the
	 * to label appears in src, we can just bitwise-or the
	 * labels over in parallel; if not, we need to add it
	 * first, but it will be added at the current offset. */
	size_t sg_i, dg_i, w_i;	/* src/dst group index, word index */
	struct edge_set *dst = *pdst;

	dg_i = 0;
	sg_i = 0;
	while (sg_i < src->count) {
		const struct edge_group *src_g = &src->groups[sg_i];
		struct edge_group *dst_g = (dg_i < dst->count
		    ? &dst->groups[dg_i]
		    : NULL);
		if (dst_g == NULL || dst_g->to > src_g->to) {
			unsigned char label;

			/* If the src group is empty, skip it (that can
			 * happen as labels are added and removed,
			 * we don't currently prune empty ones),
			 * otherwise get the first label present for
			 * the edge_set_add below. */
			if (!find_first_group_label(src_g, &label)) {
				sg_i++;
				continue;
			}

			/* Insert the first label, so a group with
			 * that .to will exist at the current offset. */
			if (!edge_set_add(&dst, alloc,
				label, src_g->to)) {
				return 0;
			}

			dst_g = &dst->groups[dg_i];
		}

		assert(dst_g != NULL); /* exists now */
		if (dst_g->to < src_g->to) {
			dg_i++;
			continue;
		}

		assert(dst_g->to == src_g->to);
		for (w_i = 0; w_i < 4; w_i++) { /* bit-parallel union */
			dst_g->symbols[w_i] |= src_g->symbols[w_i];
		}
		sg_i++;
		dg_i++;
	}
	return 1;
}

int
edge_set_copy(struct edge_set **dst, const struct fsm_alloc *alloc,
	const struct edge_set *src)
{
	struct edge_set *set;
	assert(dst != NULL);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_copy: on %p -> %p\n",
	    (void *)src, (void *)*dst);
#endif

	if (*dst != NULL) {
		/* The other `edge_set_copy` copies in the
		 * edges from src if *dst already exists. */
		return edge_set_copy_into(dst, alloc, src);
	}

	if (edge_set_empty(src)) {
		*dst = NULL;
		return 1;
	}

	set = f_calloc(alloc, 1, sizeof(*set));
	if (set == NULL) {
		return 0;
	}

	set->groups = f_malloc(alloc,
	    src->ceil * sizeof(set->groups[0]));
	if (set->groups == NULL) {
		f_free(alloc, set);
		return 0;
	}

	set->ceil = src->ceil;
	memcpy(set->groups, src->groups,
	    src->count * sizeof(src->groups[0]));
	set->count = src->count;

#if LOG_BITSET
	{
		size_t i;
		for (i = 0; i < set->count; i++) {
			fprintf(stderr, "edge_set[%zd]: to %d, [0x%lx, 0x%lx, 0x%lx, 0x%lx]\n",
			    i, set->groups[i].to,
			    set->groups[i].symbols[0],
			    set->groups[i].symbols[1],
			    set->groups[i].symbols[2],
			    set->groups[i].symbols[3]);
		}
	}
#endif

	*dst = set;
	return 1;
}

void
edge_set_remove(struct edge_set **pset, unsigned char symbol)
{
	size_t i;
	struct edge_set *set;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_remove: symbol 0x%x on %p\n",
	    symbol, (void *)set);
#endif

	if (edge_set_empty(set)) {
		return;
	}

	/* This does not track whether edge(s) were actually removed.
	 * It also does not remove edge groups where none of the
	 * symbols are set anymore. */
	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		SYMBOLS_CLEAR(eg->symbols, symbol);
	}
}

void
edge_set_remove_state(struct edge_set **pset, fsm_state_t state)
{
	size_t i;
	struct edge_set *set;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_remove_state: state %d on %p\n",
	    state, (void *)set);
#endif

	if (edge_set_empty(set)) {
		return;
	}

	i = 0;
	while (i < set->count) {
		const struct edge_group *eg = &set->groups[i];
		if (eg->to == state) {
			const size_t to_mv = set->count - i;
			memmove(&set->groups[i],
			    &set->groups[i + 1],
			    to_mv * sizeof(*eg));
			set->count--;
		} else {
			i++;
		}
	}
}

struct to_info {
	fsm_state_t old_to;
	fsm_state_t new_to;
	fsm_state_t assignment;
};

static int
collate_info_by_new_to(const void *pa, const void *pb)
{
	const struct to_info *a = (const struct to_info *)pa;
	const struct to_info *b = (const struct to_info *)pb;
	if (a->new_to < b->new_to) {
		return -1;
	} else if (a->new_to > b->new_to) {
		return 1;
	} else {
		return 0;
	}
}

static int
collate_info_by_old_to(const void *pa, const void *pb)
{
	const struct to_info *a = (const struct to_info *)pa;
	const struct to_info *b = (const struct to_info *)pb;
	if (a->old_to < b->old_to) {
		return -1;
	} else if (a->old_to > b->old_to) {
		return 1;
	} else {
		assert(!"violated uniqueness invariant");
		return 0;
	}
}

#define LOG_COMPACT 0

int
edge_set_compact(struct edge_set **pset, const struct fsm_alloc *alloc,
    fsm_state_remap_fun *remap, const void *opaque)
{
	struct edge_set *set;
	size_t i;
	struct to_info *info;
	struct edge_group *ngroups;
	size_t ncount = 0;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET || LOG_COMPACT
	fprintf(stderr, " -- edge_set_compact: set %p\n",
	    (void *)set);
#endif

	if (edge_set_empty(set)) {
		return 1;
	}

	assert(set->count > 0);

	info = f_malloc(alloc, set->count * sizeof(info[0]));
	if (info == NULL) {
		return 0;
	}

	ngroups = f_calloc(alloc, set->ceil, sizeof(ngroups[0]));
	if (ngroups == NULL) {
		f_free(alloc, info);
		return 0;
	}

	/* first pass, construct mapping */
	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		const fsm_state_t new_to = remap(eg->to, opaque);
#if LOG_BITSET > 1 || LOG_COMPACT
		fprintf(stderr, "compact: %ld, old_to %d -> new_to %d\n",
		    i, eg->to, new_to);
#endif
		info[i].old_to = eg->to;
		info[i].new_to = new_to;
		info[i].assignment = (fsm_state_t)-1; /* not yet assigned */
	}

	/* sort info by new_state */
	qsort(info, set->count, sizeof(info[0]),
	    collate_info_by_new_to);

#if LOG_BITSET > 1 || LOG_COMPACT
	fprintf(stderr, "== after sort by new_state\n");
	for (i = 0; i < set->count; i++) {
		fprintf(stderr, " -- %lu: old_to: %d, new_to: %d, assignment: %d\n",
		    i,
		    info[i].old_to,
		    info[i].new_to,
		    info[i].assignment);
	}
#endif

	info[0].assignment = 0;
	ncount++;

	for (i = 1; i < set->count; i++) {
		const fsm_state_t prev_new_to = info[i - 1].new_to;
		const fsm_state_t prev_assignment = info[i - 1].assignment;

		assert(info[i].new_to >= prev_new_to);

		if (info[i].new_to == FSM_STATE_REMAP_NO_STATE) {
			break;
		}

		if (info[i].new_to == prev_new_to) {
			info[i].assignment = prev_assignment;
		} else {
			info[i].assignment = prev_assignment + 1;
			ncount++;
		}
	}

	/* sort again, by old_state */
	qsort(info, set->count, sizeof(info[0]),
	    collate_info_by_old_to);

#if LOG_BITSET > 1 || LOG_COMPACT
	fprintf(stderr, "== after sort by old_state\n");
	for (i = 0; i < set->count; i++) {
		fprintf(stderr, " -- %lu: old_to: %d, new_to: %d, assignment: %d\n",
		    i,
		    info[i].old_to,
		    info[i].new_to,
		    info[i].assignment);
	}
#endif

	/* second pass, copy/condense */
	for (i = 0; i < set->count; i++) {
		struct edge_group *g;
		size_t w_i;
		if (info[i].new_to == FSM_STATE_REMAP_NO_STATE) {
			continue;
		}
		g = &ngroups[info[i].assignment];
		g->to = info[i].new_to;

		for (w_i = 0; w_i < 256/64; w_i++) {
			g->symbols[w_i] |= set->groups[i].symbols[w_i];
		}
	}

	f_free(alloc, info);
	f_free(alloc, set->groups);
	set->groups = ngroups;
	set->count = ncount;

#if LOG_BITSET > 1 || LOG_COMPACT
	for (i = 0; i < set->count; i++) {
		fprintf(stderr, "ngroups[%zu]: to %d\n",
		    i, set->groups[i].to);
	}
#endif
	return 1;
}

void
edge_set_reset(const struct edge_set *set, struct edge_iter *it)
{
	assert(it != NULL);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_reset: set %p\n",
	    (void *)set);
#endif

	it->i = 0;
	it->j = 0;
	it->set = set;
}

int
edge_set_next(struct edge_iter *it, struct fsm_edge *e)
{
	const struct edge_set *set;
	assert(it != NULL);
	set = it->set;

	set = it->set;

#if LOG_BITSET > 1
	fprintf(stderr, " -- edge_set_next: set %p, i %ld, j 0x%x\n",
	    (void *)set, it->i, (unsigned)it->j);
#endif

	if (set == NULL) {
		return 0;
	}

	while (it->i < set->count) {
		const struct edge_group *eg = &set->groups[it->i];
		while (it->j < 256) {
			if ((it->j & 63) == 0 && 0 == eg->symbols[it->j/64]) {
				it->j += 64;
			} else {
				if (SYMBOLS_GET(eg->symbols, it->j)) {
					e->symbol = it->j;
					e->state = eg->to;
					it->j++;
					return 1;
				}
				it->j++;
			}
		}

		it->i++;
		it->j = 0;
	}

	return 0;
}

void
edge_set_rebase(struct edge_set **pset, fsm_state_t base)
{
	size_t i;
	struct edge_set *set;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_rebase: set %p, base %d\n",
	    (void *)set, base);
#endif

	if (edge_set_empty(set)) {
		return;
	}

	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		eg->to += base;
	}
}

int
edge_set_replace_state(struct edge_set **pset, const struct fsm_alloc *alloc,
    fsm_state_t old, fsm_state_t new)
{
	size_t i;
	struct edge_set *set;
	struct edge_group cp;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_replace_state: set %p, state %d -> %d\n",
	    (void *)set, old, new);
#endif

	if (edge_set_empty(set)) {
		return 1;
	}

	/* Invariants: if a group with .to == old appears in the group,
	 * it should only appear once. Replacing .to may lead to
	 * duplicates, so duplicates may need to be merged after. */

	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		if (eg->to == old) {
			const size_t to_mv = set->count - i;
			unsigned char label;
			if (!find_first_group_label(eg, &label)) {
				return 1; /* ignore empty group */
			}

#if LOG_BITSET
			fprintf(stderr, " -- edge_set_replace_state: removing group at %ld with .to=%d\n", i, old);
#endif
			/* Remove group */
			memcpy(&cp, eg, sizeof(cp));
			memmove(&set->groups[i],
			    &set->groups[i + 1],
			    to_mv * sizeof(set->groups[i]));
			set->count--;

#if LOG_BITSET
			dump_edge_set(set);
			fprintf(stderr, " -- edge_set_replace_state: reinserting group with .to=%d and label 0x%x\n", new, (unsigned)label);
#endif

			/* Realistically, this shouldn't fail, because
			 * edge_set_add only fails on allocation failure
			 * when it needs to grow the backing array, but
			 * we're removing a group and then adding the
			 * group again so add's bookkeeping puts the
			 * group in the appropriate place. */
			if (!edge_set_add(&set, alloc,
				label, new)) {
				return 0;
			}
			dump_edge_set(set);

			for (i = 0; i < set->count; i++) {
				eg = &set->groups[i];
				if (eg->to == new) {
					size_t w_i;
#if LOG_BITSET
					fprintf(stderr, " -- edge_set_replace_state: found new group at %ld, setting other labels from copy\n", i);
#endif
					for (w_i = 0; w_i < 4; w_i++) {
						eg->symbols[w_i] |= cp.symbols[w_i];
					}
					dump_edge_set(set);
					return 1;
				}
			}
			assert(!"internal error: just added, but not found");
		}
	}
	return 1;
}

int
edge_set_empty(const struct edge_set *s)
{
	size_t i;
	if (s == NULL || s->count == 0) {
		return 1;
	}

	for (i = 0; i < s->count; i++) {
		unsigned char label;
		if (find_first_group_label(&s->groups[i], &label)) {
			return 0;
		}
	}

	return 1;
}

void
edge_set_ordered_iter_reset_to(const struct edge_set *set,
    struct edge_ordered_iter *eoi, unsigned char symbol)
{
	eoi->symbol = symbol;		/* stride by character */
	eoi->pos = 0;
	eoi->set = set;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_ordered_iter_reset_to: set %p, symbol 0x%x\n",
	    (void *)set, symbol);
#endif

}

/* Reset an ordered iterator, equivalent to
 * edge_set_ordered_iter_reset_to(set, eoi, '\0'). */
void
edge_set_ordered_iter_reset(const struct edge_set *set,
    struct edge_ordered_iter *eoi)
{
	edge_set_ordered_iter_reset_to(set, eoi, 0x00);
}

/* Get the next edge from an ordered iterator and return 1,
 * or return 0 when no more are available. */
int
edge_set_ordered_iter_next(struct edge_ordered_iter *eoi, struct fsm_edge *e)
{
	const struct edge_set *set;
	assert(eoi != NULL);

	set = eoi->set;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_ordered_iter_next: set %p, pos %ld, symbol 0x%x\n",
	    (void *)set, eoi->pos, eoi->symbol);
#endif

	if (set == NULL) {
		return 0;
	}

	for (;;) {
		while (eoi->pos < set->count) {
			struct edge_group *eg = &set->groups[eoi->pos++];
			if (SYMBOLS_GET(eg->symbols, eoi->symbol)) {
				e->symbol = eoi->symbol;
				e->state = eg->to;
				return 1;
			}
		}

		if (eoi->symbol == 255) { /* done */
			eoi->set = NULL;
			return 0;
		} else {
			eoi->symbol++;
			eoi->pos = 0;
		}
	}

	return 0;
}

void
edge_set_group_iter_reset(const struct edge_set *set,
    enum edge_group_iter_type iter_type,
    struct edge_group_iter *egi)
{
	memset(egi, 0x00, sizeof(*egi));
	egi->set = set;
	egi->flag = iter_type;

#if LOG_BITSET > 1
	fprintf(stderr, " -- edge_set_group_iter_reset: set %p, type %d\n",
	    (void *)set, iter_type);
#endif

	if (iter_type == EDGE_GROUP_ITER_UNIQUE && set != NULL) {
		struct edge_group *g;
		size_t g_i, i;
		uint64_t seen[256/64] = { 0 };
		for (g_i = 0; g_i < set->count; g_i++) {
			g = &set->groups[g_i];
			for (i = 0; i < 256; i++) {
				if ((i & 63) == 0 && g->symbols[i/64] == 0) {
					i += 63; /* skip empty word */
					continue;
				}
				if (SYMBOLS_GET(g->symbols, i)) {
					if (SYMBOLS_GET(seen, i)) {
						SYMBOLS_SET(egi->internal, i);
					} else {
						SYMBOLS_SET(seen, i);
					}
				}
			}
		}
	}
}

int
edge_set_group_iter_next(struct edge_group_iter *egi,
    struct edge_group_iter_info *eg)
{
	struct edge_group *g;
	int any = 0;
	size_t i;
advance:
	if (egi->set == NULL || egi->i == egi->set->count) {
#if LOG_BITSET > 1
		fprintf(stderr, " -- edge_set_group_iter_next: set %p, count %lu, done\n",
		    (void *)egi->set, egi->i);
#endif
		return 0;
	}

	g = &egi->set->groups[egi->i];

	eg->to = g->to;

#if LOG_BITSET > 1
	fprintf(stderr, " -- edge_set_group_iter_next: flag %d, i %zu, to %d\n",
	    egi->flag, egi->i, g->to);
#endif

	if (egi->flag == EDGE_GROUP_ITER_ALL) {
		egi->i++;
		for (i = 0; i < 4; i++) {
			eg->symbols[i] = g->symbols[i];
			if (eg->symbols[i] != 0) {
				any = 1;
			}
		}
		if (!any) {
			goto advance;
		}
		return 1;
	} else if (egi->flag == EDGE_GROUP_ITER_UNIQUE) { /* uniques first */
		for (i = 0; i < 4; i++) {
			eg->symbols[i] = g->symbols[i] &~ egi->internal[i];
			if (eg->symbols[i] != 0) {
				any = 1;
			}
		}

		/* next time, yield non-uniques */
		egi->flag = EDGE_GROUP_ITER_UNIQUE + 1;

		/* if there are any uniques, yield them, otherwise
		 * continue to the non-unique branch below. */
		if (any) {
			eg->unique = 1;
			return 1;
		}
	}

        if (egi->flag == EDGE_GROUP_ITER_UNIQUE + 1) {
		for (i = 0; i < 4; i++) {
			eg->symbols[i] = g->symbols[i] & egi->internal[i];
			if (eg->symbols[i]) {
				any = 1;
			}
		}
		eg->unique = 0;

		egi->flag = EDGE_GROUP_ITER_UNIQUE;
		egi->i++;
		if (!any) {
			goto advance;
		}

		return 1;
	} else {
		assert("match fail");
		return 0;
	}
}
