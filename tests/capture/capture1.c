/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>

#include "captest.h"
/* (a(b(c))) */

int main(void) {
	struct captest_single_fsm_test_info test_info = {
		"abc",
		{
			{ 0, 3 },
			{ 1, 3 },
			{ 2, 3 },
		}
	};
	return captest_run_single(&test_info);
}
