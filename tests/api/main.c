#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

int fsm(struct fsm *fsm, fsm_state_t *x, fsm_state_t *y);

static int
exec_str(const struct fsm *f, const char *s)
{
	fsm_state_t state;
	return fsm_exec(f, fsm_sgetc, &s, &state, NULL);
}

int
main(void)
{
	struct fsm *f;
	fsm_state_t x, y;

	f = fsm_new(NULL);
	assert(f != NULL);

	assert(fsm_addstate(f, &x));
	assert(fsm_addstate(f, &y));
	fsm_setstart(f, x);
	fsm_setend(f, y, 1);

	assert(fsm(f, &x, &y) == 1);

	assert(exec_str(f, "a0") == 1);
	assert(exec_str(f, "abc0") == 1);
	assert(exec_str(f, "") == 0);
	assert(exec_str(f, "0") == 0);
	assert(exec_str(f, "a") == 0);

	fsm_free(f);
	return 0;
}
