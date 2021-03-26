/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/stateset.h>
#include <adt/hashset.h>

/*
 * TODO: now fsm_state_t is a numeric index, this could be a dynamically
 * allocated bitmap, instead of a set.inc's array of items.
 */

#define SET_INITIAL 8

/*
 * Most state sets only contain a single item (think of a DFA from a typical
 * regexp with long chains of letters). In an attempt at optimisation, we avoid
 * allocating for a set until it contains more than a single item. Instead of
 * allocating storage for struct state_set, the single item is instead stored
 * directly in the struct state_set * pointer value returned.
 *
 * To distinguish these "singleton" pointers from regular struct pointers,
 * bit 0 is set. This uses the assumption that struct pointers are word aligned
 * for this particular target machine, and so this code is not portable.
 *
 * The other assumption here is that NULL is all-bits-zero, distinguishing it
 * from a singleton value where the fsm_state_t item is 0.
 */
#define SINGLETON_MAX           ((~ (uintptr_t) 0U) >> 1)
#define SINGLETON_ENCODE(state) ((void *) ((((uintptr_t) (state)) << 1) | 0x1))
#define SINGLETON_DECODE(ptr)   ((fsm_state_t) (((uintptr_t) (ptr)) >> 1))
#define IS_SINGLETON(ptr)       (((uintptr_t) (ptr)) & 0x1)

struct state_set {
	const struct fsm_alloc *alloc;
	fsm_state_t *a;
	size_t i;
	size_t n;
};

int
state_set_has(const struct fsm *fsm, const struct state_set *set,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	struct state_iter it;
	fsm_state_t s;

	assert(fsm != NULL);
	assert(predicate != NULL);

	for (state_set_reset((void *) set, &it); state_set_next(&it, &s); ) {
		if (predicate(fsm, s)) {
			return 1;
		}
	}

	return 0;
}

static int
state_set_cmpval(fsm_state_t a, fsm_state_t b)
{
	if (a > b) { return +1; }
	if (a < b) { return -1; }

	return 0;
}

static int
state_set_cmpptr(const void *a, const void *b)
{
	return state_set_cmpval(* (fsm_state_t *) a, * (fsm_state_t *) b);
}

int
state_set_cmp(const struct state_set *a, const struct state_set *b)
{
	size_t count_a, count_b;

	assert(a != NULL);
	assert(b != NULL);

	count_a = state_set_count(a);
	count_b = state_set_count(b);

	if (count_a != count_b) {
		return count_a - count_b;
	}

	if (IS_SINGLETON(a) && IS_SINGLETON(b)) {
		fsm_state_t sa, sb;

		sa = SINGLETON_DECODE(a);
		sb = SINGLETON_DECODE(b);

		return (sa > sb) - (sa < sb);
	}

	if (IS_SINGLETON(a)) {
		fsm_state_t sa, sb;

		assert(b->a != NULL);
		assert(count_b == 1);

		sa = SINGLETON_DECODE(a);
		sb = state_set_only(b);

		return (sa > sb) - (sa < sb);
	}

	if (IS_SINGLETON(b)) {
		fsm_state_t sa, sb;

		assert(a->a != NULL);
		assert(count_a == 1);

		sa = state_set_only(a);
		sb = SINGLETON_DECODE(b);

		return (sa > sb) - (sa < sb);
	}

	assert(a->a != NULL);
	assert(b->a != NULL);
	assert(a->i == b->i);

	return memcmp(a->a, b->a, a->i * sizeof *a->a);
}

/*
 * Return where an item would be, if it were inserted
 */
static size_t
state_set_search(const struct state_set *set, fsm_state_t state)
{
	size_t start, end;
	size_t mid;

	assert(set != NULL);
	assert(!IS_SINGLETON(set));
	assert(set->a != NULL);

	start = mid = 0;
	end = set->i;

	while (start < end) {
		int r;
		mid = start + (end - start) / 2;
		r = state_set_cmpval(state, set->a[mid]);
		if (r < 0) {
			end = mid;
		} else if (r > 0) {
			start = mid + 1;
		} else {
			return mid;
		}
	}

	return mid;
}

static struct state_set *
state_set_create(const struct fsm_alloc *a)
{
	struct state_set *set;

	set = f_malloc(a, sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->a = f_malloc(a, SET_INITIAL * sizeof *set->a);
	if (set->a == NULL) {
		return NULL;
	}

	set->alloc = a;
	set->i = 0;
	set->n = SET_INITIAL;

	return set;
}

void
state_set_free(struct state_set *set)
{
	if (set == NULL) {
		return;
	}

	if (IS_SINGLETON(set)) {
		return;
	}

	assert(set->a != NULL);

	f_free(set->alloc, set->a);
	f_free(set->alloc, set);
}

int
state_set_add(struct state_set **setp, const struct fsm_alloc *alloc,
	fsm_state_t state)
{
	struct state_set *set;
	size_t i;

	assert(setp != NULL);

	if (*setp == NULL) {
		*setp = SINGLETON_ENCODE(state);
		return 1;
	}

	if (IS_SINGLETON(*setp)) {
		fsm_state_t a[2];

		a[0] = SINGLETON_DECODE(*setp);
		a[1] = state;

		*setp = NULL;

		return state_set_add_bulk(setp, alloc, a, sizeof a / sizeof *a);
	}

	set = *setp;

	i = 0;

	/*
	 * If the item already exists in the set, return success.
	 */
	if (!state_set_empty(set)) {
		i = state_set_search(set, state);
		if (set->a[i] == state) {
			return 1;
		}
	}

	if (set->i) {
		/* We're at capacity. Get more */
		if (set->i == set->n) {
			fsm_state_t *new;

			new = f_realloc(set->alloc, set->a, (sizeof *set->a) * (set->n * 2));
			if (new == NULL) {
				return 0;
			}

			set->a = new;
			set->n *= 2;
		}

		if (state_set_cmpval(state, set->a[i]) > 0) {
			i++;
		}

		if (i <= set->i) {
			memmove(&set->a[i + 1], &set->a[i], (set->i - i) * (sizeof *set->a));
		}

		set->a[i] = state;
		set->i++;
	} else {
		set->a[0] = state;
		set->i = 1;
	}

	assert(state_set_contains(set, state));

	return 1;
}

int
state_set_add_bulk(struct state_set **setp, const struct fsm_alloc *alloc,
	const fsm_state_t *a, size_t n)
{
	struct state_set *set;
	size_t newlen;

	assert(setp != NULL);
	assert(a != NULL);

	if (n == 0) {
		return 1;
	}

	if (n == 1) {
		return state_set_add(setp, alloc, a[0]);
	}

	if (IS_SINGLETON(*setp)) {
		fsm_state_t prev;

		prev = SINGLETON_DECODE(*setp);

		*setp = state_set_create(alloc);
		if (*setp == NULL) {
			return 0;
		}

		assert(!IS_SINGLETON(*setp));

		if (!state_set_add(setp, alloc, prev)) {
			return 0;
		}

		/* fallthrough */
	}

	if (*setp == NULL) {
		*setp = state_set_create(alloc);
		if (*setp == NULL) {
			return 0;
		}
	}

	set = *setp;

	newlen = set->i + n;
	if (newlen > set->n) {
		/* need to expand */
		fsm_state_t *new;
		size_t newcap;

		newcap = (newlen < 2*set->n) ? 2*set->n : newlen;
		new = f_realloc(set->alloc, set->a, newcap * sizeof set->a[0]);
		if (new == NULL) {
			return 0;
		}

		set->a = new;
		set->n = newcap;
	}

	memcpy(&set->a[set->i], &a[0], n * sizeof a[0]);

	qsort(&set->a[0], set->i+n, sizeof set->a[0], state_set_cmpptr);

	/* remove any duplicates */
	{
		size_t i, curr = 1, max = set->i + n;
		for (i = 1; i < max; i++) {
			int cmp = state_set_cmpval(set->a[i-1], set->a[i]);

			assert(cmp <= 0);
			assert(curr <= i);

			if (cmp != 0) {
				set->a[curr++] = set->a[i];
			}
		}

		set->i = curr;
	}

	return 1;
}

int
state_set_copy(struct state_set **dst, const struct fsm_alloc *alloc,
	const struct state_set *src)
{
	assert(dst != NULL);

	if (src == NULL) {
		return 1;
	}

	if (IS_SINGLETON(src)) {
		if (!state_set_add(dst, alloc, SINGLETON_DECODE(src))) {
			return 0;
		}

		return 1;
	}

	if (!state_set_add_bulk(dst, alloc, src->a, src->i)) {
		return 0;
	}

	return 1;
}

#define LOG_COMPACT 0
#if LOG_COMPACT
#include <stdio.h>
#endif

void
state_set_compact(struct state_set **setp,
    fsm_state_remap_fun *remap, void *opaque)
{
	struct state_set *set;
	size_t i, removed, dst;

	if (IS_SINGLETON(*setp)) {
		const fsm_state_t s = SINGLETON_DECODE(*setp);
		const fsm_state_t new_id = remap(s, opaque);
		if (new_id == FSM_STATE_REMAP_NO_STATE) {
			*setp = NULL;
		} else {
			assert(new_id <= s);
			*setp = SINGLETON_ENCODE(new_id);
		}
		return;
	}

	set = *setp;

	if (state_set_empty(set)) {
		return;
	}

	i = 0;
	removed = 0;
	dst = 0;

	for (i = 0; i < set->i; i++) {
		const fsm_state_t s = set->a[i];
		const fsm_state_t new_id = remap(s, opaque);

		if (new_id == FSM_STATE_REMAP_NO_STATE) { /* drop */
			removed++;
		} else {	/* keep */
			set->a[dst] = new_id;
			dst++;
		}
#if LOG_COMPACT
		fprintf(stderr, "state_set_compact: set->a[%zu]: %d -> %d, removed %zu\n", i, s, new_id, removed);
#endif
	}
	set->i -= removed;
	assert(set->i == dst);

#if LOG_COMPACT
	for (i = 0; i < set->i; i++) {
		fprintf(stderr, "state_set_compact: now set->a[%zu/%zu]: %d\n", i, set->i, set->a[i]);
	}
#endif
}

void
state_set_remove(struct state_set **setp, fsm_state_t state)
{
	struct state_set *set;
	size_t i;

	if (IS_SINGLETON(*setp)) {
		if (SINGLETON_DECODE(*setp) == state) {
			*setp = NULL;
		}
		return;
	}

	set = *setp;

	if (state_set_empty(set)) {
		return;
	}

	i = state_set_search(set, state);
	if (set->a[i] == state) {
		if (i < set->i) {
			memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
		}

		set->i--;
	}

	assert(!state_set_contains(set, state));
}

int
state_set_empty(const struct state_set *set)
{
	if (set == NULL) {
		return 1;
	}

	if (IS_SINGLETON(set)) {
		return 0;
	}

	return set->i == 0;
}

fsm_state_t
state_set_only(const struct state_set *set)
{
	assert(set != NULL);

	if (IS_SINGLETON(set)) {
		return SINGLETON_DECODE(set);
	}

	assert(set->n >= 1);
	assert(set->i == 1);

	return set->a[0];
}

int
state_set_contains(const struct state_set *set, fsm_state_t state)
{
	size_t i;

	if (state_set_empty(set)) {
		return 0;
	}

	if (IS_SINGLETON(set)) {
		return SINGLETON_DECODE(set) == state;
	}

	i = state_set_search(set, state);
	if (set->a[i] == state) {
		return 1;
	}

	return 0;
}

size_t
state_set_count(const struct state_set *set)
{
	if (set == NULL) {
		return 0;
	}

	if (IS_SINGLETON(set)) {
		return 1;
	}

	assert(set->a != NULL);

	return set->i;
}

void
state_set_reset(const struct state_set *set, struct state_iter *it)
{
	it->i = 0;
	it->set = set;
}

int
state_set_next(struct state_iter *it, fsm_state_t *state)
{
	assert(it != NULL);
	assert(state != NULL);

	if (it->set == NULL) {
		return 0;
	}

	if (IS_SINGLETON(it->set)) {
		if (it->i >= 1) {
			return 0;
		}

		*state = SINGLETON_DECODE(it->set);

		it->i++;

		return 1;
	}

	if (it->i >= it->set->i) {
		return 0;
	}

	*state = it->set->a[it->i];

	it->i++;

	return 1;
}

const fsm_state_t *
state_set_array(const struct state_set *set)
{
	assert(set != NULL);
	assert(!IS_SINGLETON(set));

	if (set == NULL) {
		return NULL;
	}

	return set->a;
}

void
state_set_rebase(struct state_set **setp, fsm_state_t base)
{
	struct state_set *set;
	size_t i;

	assert(setp != NULL);

	if (*setp == NULL) {
		return;
	}

	if (IS_SINGLETON(*setp)) {
		fsm_state_t state;

		state = SINGLETON_DECODE(*setp);
		state += base;

		*setp = SINGLETON_ENCODE(state);
		return;
	}

	set = *setp;

	for (i = 0; i < set->i; i++) {
		set->a[i] += base;
	}
}

void
state_set_replace(struct state_set **setp, fsm_state_t old, fsm_state_t new)
{
	struct state_set *set;
	size_t i;

	assert(setp != NULL);

	if (IS_SINGLETON(*setp)) {
		if (SINGLETON_DECODE(*setp) != old) {
			return;
		}

		*setp = SINGLETON_ENCODE(new);
		return;
	}

	set = *setp;

	if (set == NULL) {
		return;
	}

	for (i = 0; i < set->i; i++) {
		if (set->a[i] == old) {
			set->a[i] = new;
		}
	}
}

unsigned long
state_set_hash(const struct state_set *set)
{
	if (set == NULL) {
		return 0;	/* empty */
	}

	if (IS_SINGLETON(set)) {
		fsm_state_t state;
		state = SINGLETON_DECODE(set);
		return hashrec(&state, sizeof state);
	}

	return hashrec(set->a, set->i * sizeof *set->a);
}
