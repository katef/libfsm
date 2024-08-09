/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <fsm/fsm.h>

#include "libfsm/internal.h"

#include "libfsm/print/ir.h"
#include "libfsm/vm/retlist.h"

static bool
append_ret(struct ret_list *list,
	const fsm_end_id_t *ids, size_t count)
{
	const size_t low    = 16; /* must be power of 2 */
	const size_t factor =  2; /* must be even */

	assert(list != NULL);

	// TODO: alloc callbacks
	if (list->count == 0) {
		list->a = malloc(low * sizeof *list->a);
		if (list->a == NULL) {
			return false;
		}
	} else if (list->count >= low && (list->count & (list->count - 1)) == 0) {
		void *tmp;
		size_t new = list->count * factor;
		if (new < list->count) {
			errno = E2BIG;
			perror("realloc");
			exit(EXIT_FAILURE);
		}

		tmp = realloc(list->a, new * sizeof *list->a);
		if (tmp == NULL) {
			return false;
		}

		list->a = tmp;
	}

	list->a[list->count].ids = ids;
	list->a[list->count].count = count;

	list->count++;

	return true;
}

static int
cmp_ret(const void *pa, const void *pb)
{
	const struct ret *a = pa;
	const struct ret *b = pb;

	if (a->count < b->count) { return -1; }
	if (a->count > b->count) { return +1; }

	if (a->count == 0) {
		return 0;
	}

	assert(a->ids != NULL);
	assert(b->ids != NULL);

	return memcmp(a->ids, b->ids, a->count * sizeof *a->ids);
}

struct ret *
find_ret(const struct ret_list *list,
	const fsm_end_id_t *ids, size_t count)
{
	struct ret key;

	key.count = count;
	key.ids   = ids;

	return bsearch(&key, list->a, list->count, sizeof *list->a, cmp_ret);
}

bool
build_retlist(struct ret_list *list, const struct ir *ir)
{
	size_t i;

	assert(list != NULL);
	assert(ir != NULL);

	list->count = 0;

	for (i = 0; i < ir->n; i++) {
		if (!ir->states[i].isend) {
			continue;
		}

		if (!append_ret(list, ir->states[i].endids.ids, ir->states[i].endids.count)) {
			return false;
		}
	}

	if (list->count > 0) {
		size_t j = 0;

		/* sort for both dedup and bsearch */
		qsort(list->a, list->count, sizeof *list->a, cmp_ret);

		/* deduplicate based on endids only.
		 * j is the start of a run; i increments until we find
		 * the start of the next run */
		for (size_t i = 1; i < list->count; i++) {
			assert(i > j);
			if (cmp_ret(&list->a[j], &list->a[i]) == 0) {
				continue;
			}

			j++;
			list->a[j] = list->a[i];
		}

		list->count = j + 1;

		assert(list->count > 0);
	}

	return true;
}

void
free_retlist(struct ret_list *list)
{
	if (list->count > 0) {
		free(list->a);
	}
}

