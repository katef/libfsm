#include "endids_utils.h"

#include <stdlib.h>
#include <errno.h>

int
match_string(const struct fsm *fsm, const char *s, fsm_state_t *end_ptr, fsm_end_id_t **endids_ptr, size_t *count_ptr)
{
	fsm_state_t end = 0;
	int ret;

	ret = fsm_exec(fsm, fsm_sgetc, &s, &end, NULL);
	if (ret == 1) {
		size_t count;

		if (end_ptr != NULL) {
			*end_ptr = end;
		}

		count = fsm_endid_count(fsm, end);
		if (count > 0) {
			enum fsm_getendids_res ret;
			size_t nwritten = 0;

			fsm_end_id_t *endids = calloc(count, sizeof *endids);
			if (endids == NULL) {
				return -1;
			}

			ret = fsm_endid_get(fsm, end, count, endids, &nwritten);
			if (ret != FSM_GETENDIDS_FOUND) {
				free(endids);
				errno = (ret == FSM_GETENDIDS_NOT_FOUND) ? EINVAL : ENOMEM;
				return -1;
			}
			
			if (endids_ptr != NULL) {
				*endids_ptr = endids;
				if (count_ptr != NULL) {
					*count_ptr = count;
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


