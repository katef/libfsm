#ifndef ENDIDS_UTILS_H
#define ENDIDS_UTILS_H

#include <stdlib.h>
#include <fsm/fsm.h>

int
match_string(const struct fsm *fsm, const char *s, fsm_state_t *end_ptr, fsm_end_id_t **endids_ptr, size_t *num_endids_ptr);

int
cmp_endids(const void *pa, const void *pb);

#endif /* ENDIDS_UTILS_H */

