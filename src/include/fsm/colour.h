/* $Id$ */

#ifndef FSM_COLOUR_H
#define FSM_COLOUR_H

#include <stdio.h>

struct fsm;
struct fsm_state;
struct fsm_edge;

/*
 * You may not modify the fsm from within the these callbacks.
 *
 * If no hooks are assigned, the default is as if they are set NULL.
 */
struct fsm_colour_hooks {

	/*
	 * This is called to compare colours for equivalence. This will be used by
	 * any internal operations which need to test for equality of colours.
	 *
	 * A "shallow" comparison by pointer value is performed if set NULL.
	 *
	 * The callback is expected to return true if the colours are considered
	 * equivalent, and false otherwise.
	 */
	int (*compare)(const struct fsm *fsm, void *a, void *b);

	/*
	 * Format a colour to a stream. This is called during code generation.
	 * Return values are as per printf.
	 *
	 * If NULL
	 */
	int (*print)(const struct fsm *fsm, FILE *f, void *colour);

};


/*
 * Assign a callbacks used for colour operations. If none have been set, the
 * default behaviour is as if each function is set NULL. This default can be
 * restored by assigning the pointer to the entire hooks struct to be NULL.
 *
 * An internal copy of the hooks struct is taken, and hence its storage may be
 * destroyed after this call.
 */
void
fsm_setcolourhooks(struct fsm *fsm, const struct fsm_colour_hooks *h);


#endif

