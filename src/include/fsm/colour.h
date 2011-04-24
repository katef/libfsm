/* $Id$ */

#ifndef FSM_COLOUR_H
#define FSM_COLOUR_H

#include <stdio.h>

struct fsm;
struct fsm_state;

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
 * Set user-specified colour data per-state. Multiple colours may be assigned
 * to the same state. Duplicate values (as determined by the comparison callback
 * specified to fsm_setcompare) are disregarded and silently succeed.
 *
 * A colour is really just an opaque pointer which is passed around by libfsm
 * and never dereferenced, so you can use it to point to application-specific
 * data.
 *
 * There are two types of void * used for user data floating around; these are
 * called colours (as in graph colouring) because they're also used to uniquely
 * identify unambigious subgraphs during graph splitting. The other kind (passed
 * through to callbacks e.g. by fsm_exec and the generated code) have no semantic
 * meaning, and so are just called "opaques", to keep the distinction clear.
 *
 * Returns false on error; see errno.
 */
int
fsm_addcolour(struct fsm *fsm, struct fsm_state *state, void *colour);

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

