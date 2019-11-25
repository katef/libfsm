/*
 * Copyright 2019 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

struct fsm_alloc;

/* Basic fixed-capacity FIFO queue. */
struct queue;

/*
 * Allocate a new queue, returns NULL on error.
 * Note that the queue does not resize, all allocation is
 * done once, upfront.
 */
struct queue *
queue_new(const struct fsm_alloc *a, size_t max_capacity);

/*
 * Push a pointer into the queue. Returns 1 on success,
 * or 0 if full. Note that P can be NULL.
 */
int
queue_push(struct queue *q, fsm_state_t state);

/*
 * Pop the next pointer from the queue. Returns 1 if
 * *p was written into, or 0 if the queue is empty.
 */
int
queue_pop(struct queue *q, fsm_state_t *state);

/*
 * Free the queue. If the queue is non-empty, then any
 * pointers contained within will leak.
 */
void
queue_free(struct queue *q);

#endif
