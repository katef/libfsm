#include "testutil.h"

#include <re/re.h>
#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/walk.h>
#include <fsm/print.h>

int main()
{
	enum re_flags flags = 0;
	struct re_err err;
	const char *regex = "^abcde$";
	
	struct fsm *fsm = re_comp(RE_PCRE, fsm_sgetc, &regex, NULL, flags, &err);
	assert(fsm != NULL);

	if (!fsm_determinise(fsm)) {
		assert(!"determinise");
		return EXIT_FAILURE;
	}
	if (!fsm_minimise(fsm)) {
		assert(!"minimise");
		return EXIT_FAILURE;
	}

	uint64_t charmap[4];

	/* keep decreasing the step limit until it's hit, and check that
	 * the bitmap is cleared. */
	bool hit_step_limit = false;
	size_t step_limit = 25;
	while (!hit_step_limit) {
		assert(step_limit > 0);

		const enum fsm_detect_required_characters_res res = fsm_detect_required_characters(fsm, step_limit, charmap, NULL);
		if (res == FSM_DETECT_REQUIRED_CHARACTERS_STEP_LIMIT_REACHED) {
			hit_step_limit = true;

			/* this should not contain any partially complete information */
			for (size_t i = 0; i < 4; i++) {
				if (charmap[i] != 0) {
					fprintf(stderr, "-- Test failure: partial information set when step limit reached\n");
					return EXIT_FAILURE;
				}
			}
		}
		
		step_limit--;
	}
	assert(hit_step_limit);
	printf("-- successfully hit step limit at %zd\n", step_limit);

	fsm_free(fsm);
	return EXIT_SUCCESS;
}
