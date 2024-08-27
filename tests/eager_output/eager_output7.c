#include "utils.h"

int main(void)
{
	/* Run this test with env FORCE_ENDIDS=N ... to see how much more
	 * expensive it is to combine the first N patterns using endids,
	 * rather than eager_outputs. It becomes VERY slow for >= 9 or so.
	 * (Note that the checks probably will not pass for N < 4, because
	 * it will start skipping appear in the early test inputs.) */
	bool force_endids = false;
	size_t force_endid_count = 0;
	{
		const char *str = getenv("FORCE_ENDIDS");
		if (str != NULL) {
			force_endid_count = atoi(str);
			if (force_endid_count == 0) {
				force_endid_count = 26;
			}
			force_endids = true;
		}
	}

	struct eager_output_test test = {
		.patterns =  {
			[0] = "apple",
			[1] = "banana",
			[2] = "carrot",
			[3] = "durian",
			[4] = "eggplant",
			[5] = "fig",
			[6] = "grapefruit",
			[7] = "hazelnut",
			[8] = "iceberg lettuce",
			[9] = "jicama",
			[10] = "kiwano",
			[11] = "lemon",
			[12] = "mango",
			[13] = "nectarine",
			[14] = "orange",
			[15] = "plum",
			[16] = "quince",
			[17] = "radish",
			[18] = "strawberry",
			[19] = "turnip",
			[20] = "ube",
			[21] = "vanilla",
			[22] = "watermelon",
			[23] = "xigua watermelon",
			[24] = "yam",
			[25] = "zucchini",
		},
		.inputs = {
			/* Note: expected IDs are shifted by 1, it's 0-terminated. */
			{ .input = "apple", .expected_ids = { 1 } },
			{ .input = "banana", .expected_ids = { 2 } },
			{ .input = "carrot", .expected_ids = { 3 } },
			{ .input = "apple banana", .expected_ids = { 1, 2 } },
			{ .input = "carrot durian apple", .expected_ids = { 1, 3, 4 } },
			{ .input = "carrot fig apple", .expected_ids = { 1, 3, 6 } },

			/* leading characters and an incomplete trailing match */
			{ .input = "mumble mumble fig hazelnut banana xigua watermelo", .expected_ids = { 2, 6, 8 } },

			/* redundant matches */
			{ .input = "ube ube ube ube ube", .expected_ids = { 21 } },

			/* everything */
			{ .input =
			  "apple banana carrot durian eggplant fig grapefruit "
			  "hazelnut iceberg lettuce jicamaa kiwano lemon mango "
			  "nectarine orange plum quince radish strawberry "
			  "turnip ube vanilla watermelon xigua watermelon yam zucchini",
			  .expected_ids = {
				  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
				  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
			  },
			},
			/* everything, only spaces appearing in patterns */
			{ .input =
			  "applebananacarrotdurianeggplantfiggrapefruit"
			  "hazelnuticeberg lettucejicamaakiwanolemonmango"
			  "nectarineorangeplumquinceradishstrawberry"
			  "turnipubevanillawatermelonxigua watermelonyamzucchini",
			  .expected_ids = {
				  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
				  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
			  },
			},
		},
	};

	/* truncate patterns to the first N */
	if (force_endids) {
		assert(force_endid_count > 0 && force_endid_count <= 26);
		test.patterns[force_endid_count] = NULL;

		/* truncate test inputs to just the first couple, since
		 * later inputs use later patterns */
		test.inputs[5].input = NULL;
	}

	return run_test(&test, false, force_endids);
}
