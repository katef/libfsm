#include "endids_utils.h"

#include <stdlib.h>
#include <errno.h>

int
match_string(const struct fsm *fsm, const char *s, fsm_state_t *end_ptr, fsm_end_id_t **ids_ptr, size_t *count_ptr)
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
			int ret;

			fsm_end_id_t *ids = calloc(count, sizeof *ids);
			if (ids == NULL) {
				return -1;
			}

			ret = fsm_endid_get(fsm, end, count, ids);
			if (ret == 0) {
				free(ids);
				errno = EINVAL;
				return -1;
			}
			
			if (ids_ptr != NULL) {
				*ids_ptr = ids;
				if (count_ptr != NULL) {
					*count_ptr = count;
				}
			} else {
				free(ids);
			}
		}
	}

	return ret;
}

