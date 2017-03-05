/* $Id$ */

#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/out.h>

int main(void) {
	struct fsm *fsm;
	struct fsm_state *s[10];
	struct fsm_state *new;
	int i;

	fsm = fsm_new(NULL);

	for (i = 0; i < sizeof s / sizeof *s; i++) {
		s[i] = fsm_addstate(fsm);
	}

	fsm_setstart(fsm, s[0]);

	fsm_addedge_literal(fsm, s[0], s[1], 'a');
	fsm_addedge_literal(fsm, s[0], s[6], 'b');

	fsm_addedge_literal(fsm, s[1], s[2], 'a');
	fsm_addedge_literal(fsm, s[1], s[3], 'b');

	fsm_addedge_literal(fsm, s[2], s[2], 'a');
	fsm_addedge_literal(fsm, s[2], s[3], 'b');

	fsm_addedge_literal(fsm, s[3], s[4], 'a');

	fsm_addedge_literal(fsm, s[4], s[2], 'a');
	fsm_addedge_literal(fsm, s[4], s[3], 'b');
	fsm_addedge_literal(fsm, s[4], s[5], 'c');

	fsm_addedge_literal(fsm, s[6], s[3], 'a');

	fsm_addedge_literal(fsm, s[6], s[7], 'a');

	fsm_addedge_literal(fsm, s[7], s[8], 'a');

	fsm_addedge_literal(fsm, s[8], s[6], 'a');
	fsm_addedge_literal(fsm, s[8], s[5], 'b');

	new = fsm_state_duplicatesubgraph(fsm, s[1]);

	fsm_addedge_literal(fsm, s[0], new, 'b');

	fsm_print(fsm, stdout, FSM_OUT_FSM);

	fsm_free(fsm);

	return 0;
}

