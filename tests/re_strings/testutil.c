#include "testutil.h"

#include <stdbool.h>
#include <assert.h>

#include "fsm/fsm.h"
#include "fsm/options.h"

#include "re/re.h"
#include "re/strings.h"

#define MAX_INPUTS 100
static fsm_end_id_t ids[MAX_INPUTS];

int
run_test(const char **strings)
{
	struct re_strings *s = re_strings_new();
	assert(s != NULL);

	fsm_end_id_t id = 0;
	const char **input = strings;
	while (*input != NULL) {
		if (!re_strings_add_str(s, *input, &id)) {
			assert(!"re_strings_add_str");
		}

		input++;
		id++;
		assert(id < MAX_INPUTS);
	}

	const int flags = 0;	/* not anchored */

	struct fsm *fsm = re_strings_build(s, NULL, flags);
	assert(fsm != NULL);

	/* Each literal string input should match, and the set of
	 * matching endids should include the expected one. */
	id = 0;
	input = strings;
	while (*input != NULL) {
		fsm_state_t end;
		const char **string = input;
		const int res = fsm_exec(fsm, fsm_sgetc, string, &end, NULL);
		assert(res > 0); /* match */

		size_t count = fsm_endid_count(fsm, end);
		assert(count <= MAX_INPUTS);

		int eres = fsm_endid_get(fsm, end, count, ids);
		assert(eres == 1);
		bool found = false;
		for (size_t i = 0; i < count; i++) {
			if (ids[i] == id) {
				found = true;
				break;
			}
		}
		assert(found);

		input++;
		id++;
	}

	re_strings_free(s);
	fsm_free(fsm);

	return EXIT_SUCCESS;
}
