#include <adt/ipriq.h>

#include <assert.h>
#include <limits.h>

#define DEF_CELLS 2
#define CHECK 0
#define LOG 0

#if LOG
#include <stdio.h>
#endif

struct ipriq {
	const struct fsm_alloc *alloc;
	ipriq_cmp_fun *cmp;
	void *opaque;

	size_t ceil;
	size_t count;
	size_t *cells;
};

struct ipriq *
ipriq_new(const struct fsm_alloc *alloc,
    ipriq_cmp_fun *cmp, void *opaque)
{
	struct ipriq *res;
	size_t *cells;

	(void)alloc;
	(void)cmp;
	(void)opaque;

	if (cmp == NULL) {
		return NULL;	/* required */
	}

	res = f_malloc(alloc, sizeof(*res));
	if (res == NULL) {
		return NULL;
	}

	cells = f_malloc(alloc, DEF_CELLS * sizeof(cells[0]));
	if (cells == NULL) {
		f_free(alloc, res);
		return NULL;
	}

	res->alloc = alloc;
	res->cmp = cmp;
	res->opaque = opaque;
	res->ceil = DEF_CELLS;
	res->count = 0;
	res->cells = cells;
	return res;
}

void
ipriq_free(struct ipriq *pq)
{
	if (pq == NULL) {
		return;
	}

	f_free(pq->alloc, pq->cells);
	f_free(pq->alloc, pq);
}

int
ipriq_empty(const struct ipriq *pq)
{
	assert(pq != NULL);
	return pq->count == 0;
}

int
ipriq_peek(const struct ipriq *pq, size_t *res)
{
	assert(res != NULL);

	if (ipriq_empty(pq)) {
		return 0;
	}

	*res = pq->cells[0];
	return 1;
}

static void
swap(struct ipriq *pq, size_t x, size_t y)
{
    const size_t tmp = pq->cells[x];
    pq->cells[x] = pq->cells[y];
    pq->cells[y] = tmp;
}

static void
check_heapness(const struct ipriq *pq)
{
#if CHECK
	size_t i;
	for (i = 0; i < pq->count; i++) {
		if (i > 0) {
			size_t parent = (i - 1)/2;
			const size_t pv = pq->cells[parent];
			const size_t cv = pq->cells[i];
			assert(pq->cmp(pv, cv, pq->opaque) < IPRIQ_CMP_GT);
		}
	}
#else
	(void)pq;
#endif
}

#define NO_CELL ((size_t)-1)

int
ipriq_pop(struct ipriq *pq, size_t *res)
{
	assert(res != NULL);

	if (ipriq_empty(pq)) {
		return 0;
	}

	*res = pq->cells[0];

	if (pq->count > 1) {
		size_t pos = 0;
		pq->cells[0] = pq->cells[pq->count - 1];
		for (;;) {
			/* left pos, right pos */
			const size_t lp = 2*(pos + 1) - 1;
			if (lp >= pq->count) { break; }
			const size_t rp = 2*(pos + 1);

			/* current, left, right values */
			const size_t cv = pq->cells[pos];
			const size_t lv = pq->cells[lp];
			const size_t rv = (rp >= pq->count ? NO_CELL : pq->cells[rp]);
#if LOG
			printf("    -- pos %zu (%zu), lp %zu (%zu), rp %zu (%zu)\n",
			    pos, cv, lp, lv, rp, rv);
#endif
			if (pq->cmp(lv, cv, pq->opaque) == IPRIQ_CMP_LT) {
				if (rv != NO_CELL
				    && pq->cmp(rv, lv, pq->opaque) == IPRIQ_CMP_LT) {
					swap(pq, pos, rp);
					pos = rp;
				} else {
					swap(pq, pos, lp);
					pos = lp;
				}
			} else if (rv != NO_CELL
			    && pq->cmp(rv, cv, pq->opaque) == IPRIQ_CMP_LT) {
				swap(pq, pos, rp);
				pos = rp;
			} else {
				break;
			}
		}
	}

	pq->count--;
#if LOG
	printf("POP: %zu <-", *res);
	for (size_t i = 0; i < pq->count; i++) {
		printf(" %zu", pq->cells[i]);
	}
	printf("\n");
#endif

	check_heapness(pq);

	return 1;
}

int
ipriq_add(struct ipriq *pq, size_t x)
{
	assert(pq != NULL);
	(void)x;

	if (pq->count == pq->ceil) {
		size_t nceil = 2*pq->ceil;
		assert(nceil > pq->ceil);

		size_t *ncells = realloc(pq->cells,
		    nceil * sizeof(*ncells));
		if (ncells == NULL) { return 0; }
		pq->cells = ncells;
		pq->ceil = nceil;
	}

	pq->cells[pq->count] = x;

	size_t pos = pq->count;

	while (pos > 0) {
		const size_t parent = (pos - 1)/2;
#if LOG
		printf("%s: pos %zu, parent %zu\n", __func__, pos, parent);
#endif
		const size_t pv = pq->cells[parent];
		const size_t cv = pq->cells[pos];

		if (pq->cmp(cv, pv, pq->opaque) == IPRIQ_CMP_LT) {
			swap(pq, parent, pos);
			pos = parent;
		} else {
			break;
		}
	}

	pq->count++;

#if LOG
	printf("ADD:");
	for (size_t i = 0; i < pq->count; i++) {
		printf(" %zu", pq->cells[i]);
	}
	printf("\n");
#endif

	check_heapness(pq);
	return 1;
}
