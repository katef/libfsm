#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <assert.h>

#include <re/re.h>
#include <fsm/fsm.h>
#include <fsm/bool.h>

static const char *strings[] = {
	[0] = "apple",
	[1] = "banana",
	[2] = "carrot",
	[3] = "durian",
	[4] = "eggplant",
};
#define STRING_COUNT sizeof(strings)/sizeof(strings[0])

int main(void)
{
	struct fsm *fsms[STRING_COUNT] = {0};

	for (size_t i = 0; i < STRING_COUNT; i++) {
		fsms[i] = re_comp(RE_PCRE, fsm_sgetc, &strings[i], NULL, 0, NULL);
		assert(fsms[i] != NULL);
	}

	struct fsm *combined_fsm = fsm_union_array(STRING_COUNT, fsms, NULL);
	assert(combined_fsm != NULL);

	size_t state_limit_base = fsm_countstates(combined_fsm);
	size_t max_state_limit = state_limit_base + 100;

	bool hit_state_limit = false;

	for (size_t state_limit = state_limit_base; state_limit < max_state_limit; state_limit += 10) {
		struct fsm *cp = fsm_clone(combined_fsm);

		const struct fsm_determinise_config det_config = {
			.state_limit = state_limit,
		};

		/* Previously this would leak memory when hitting the STATE_LIMIT_REACHED
		 * early exit, because the edge sets for the DFA being constructed were
		 * not freed properly.
		 *
		 * The first time this should fail immediately because the state limit IS the starting size,
		 * but later on it should halt in the middle of construction. */
		switch (fsm_determinise_with_config(cp, &det_config)) {
		case FSM_DETERMINISE_WITH_CONFIG_OK:
			fsm_free(cp);
			break;
		case FSM_DETERMINISE_WITH_CONFIG_STATE_LIMIT_REACHED:
			hit_state_limit = true;
			fsm_free(cp);
			break;
		case FSM_DETERMINISE_WITH_CONFIG_ERRNO:
			assert(!"internal error");
			return EXIT_FAILURE;
		}
	}

	assert(hit_state_limit);

	fsm_free(combined_fsm);
	return EXIT_SUCCESS;
}
