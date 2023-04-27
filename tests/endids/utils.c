#include "endids_utils.h"

#include <stdlib.h>
#include <errno.h>

int
match_string(const struct fsm *fsm, const char *s, fsm_state_t *end_ptr, fsm_end_id_t **endids_ptr, size_t *num_endids_ptr)
{
	fsm_state_t end = 0;
	int ret;

	ret = fsm_exec(fsm, fsm_sgetc, &s, &end, NULL);
	if (ret == 1) {
		size_t num_endids;

		if (end_ptr != NULL) {
			*end_ptr = end;
		}

		num_endids = fsm_getendidcount(fsm, end);
		if (num_endids > 0) {
			enum fsm_getendids_res ret;
			size_t nwritten = 0;

			fsm_end_id_t *endids = calloc(num_endids, sizeof *endids);
			if (endids == NULL) {
				return -1;
			}

			ret = fsm_getendids(fsm, end, num_endids, endids, &nwritten);
			if (ret != FSM_GETENDIDS_FOUND) {
				free(endids);
				errno = (ret == FSM_GETENDIDS_NOT_FOUND) ? EINVAL : ENOMEM;
				return -1;
			}
			
			if (endids_ptr != NULL) {
				*endids_ptr = endids;
				if (num_endids_ptr != NULL) {
					*num_endids_ptr = num_endids;
				}
			} else {
				free(endids);
			}
		}
	}

	return ret;
}

int
cmp_endids(const void *pa, const void *pb) {
	const fsm_end_id_t *a = pa;
	const fsm_end_id_t *b = pb;

	if (*a < *b) { return -1; }
	if (*a > *b) { return  1; }
	return 0;
}


