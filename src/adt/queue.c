/*
 * Copyright 2019 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <string.h>
#include <assert.h>

#include <adt/alloc.h>
#include <adt/queue.h>

struct queue {
	const struct fsm_alloc *alloc;
	/* Read and write offsets. When rd == wr, the queue is empty. rd
	 * will always be <= wr, and wr <= capacity. */
	size_t rd;
	size_t wr;
	size_t capacity;
	void *q[1 /* capacity */];
};

struct queue *
queue_new(const struct fsm_alloc *a, size_t max_capacity)
{
	struct queue *q;
	size_t alloc_size;
	if (max_capacity == 0) { return NULL; }
	alloc_size = sizeof(*q) + (max_capacity - 1) * sizeof(q->q[0]);

	q = f_calloc(a, 1, alloc_size);
	if (q == NULL) { return NULL; }

	q->alloc = a;
	q->capacity = max_capacity;

	return q;
}

int
queue_push(struct queue *q, void *p)
{
	assert(q->rd <= q->wr);
	assert(q->wr <= q->capacity);

	if (q->wr == q->capacity) {
		/* If there's no room but some have been read, shift all
		 * the entries down. In the worst case, this will
		 * memmove everything after reading 1. */
		if (q->rd > 0 && q->rd < q->wr) {
			memmove(&q->q[0], &q->q[q->rd],
			    q->rd * sizeof(q->q[0]));
			q->wr -= q->rd;
			q->rd = 0;
		} else {
			return 0;
		}
	}

	q->q[q->wr] = p;
	q->wr++;
	return 1;
}

int
queue_pop(struct queue *q, void **p)
{
	assert(q != NULL);
	assert(p != NULL);
	assert(q->rd <= q->wr);
	assert(q->wr <= q->capacity);

	if (q->rd == q->wr) { return 0; }

	*p = q->q[q->rd];
	q->rd++;

	if (q->rd == q->wr) {
		q->rd = 0;
		q->wr = 0;
	}

	return 1;
}

void
queue_free(struct queue *q)
{
	f_free(q->alloc, q);
}
