#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <assert.h>

#include <re/re.h>
#include <fsm/fsm.h>

#include "ac.h"

struct fsm *
re_strings(const struct fsm_options *opt, const char *sv[], size_t n, enum re_strings_flags flags)
{
	struct trie_graph *g;
	struct fsm *fsm = NULL;
	size_t i;

	g = re_strings_new();
	if (g == NULL) {
		goto done;
	}

	for (i = 0; i < n; i++) {
		if (!re_strings_add(g, sv[i])) {
			goto done;
		}
	}

	fsm = re_strings_builder_build(g, opt, flags);

done:

	if (g != NULL) {
		re_strings_free(g);
	}

	return fsm;
}

struct trie_graph *
re_strings_new(void)
{
	return trie_create();
}

void
re_strings_free(struct trie_graph *g)
{
	if (g != NULL) {
		trie_free(g);
	}
}

int
re_strings_add_data(struct trie_graph *g, const char *w, size_t wlen)
{
	assert(w != NULL);
	assert(wlen > 0);

	return trie_add_word(g, w, wlen) != NULL;
}

int
re_strings_add(struct trie_graph *g, const char *w)
{
	assert(w != NULL);

	return re_strings_add_data(g, w, strlen(w));
}

struct fsm *
re_strings_builder_build(struct trie_graph *g,
	const struct fsm_options *opt, enum re_strings_flags flags)
{
	struct fsm *fsm = NULL;
	struct fsm_state *end = NULL;

	if ((flags & RE_STRINGS_ANCHOR_LEFT) == 0) {
		if (trie_add_failure_edges(g) < 0) {
			goto error;
		}
	}

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		goto error;
	}

	end = NULL;
	if ((flags & RE_STRINGS_AC_AUTOMATON) == 0 && (flags & RE_STRINGS_ANCHOR_RIGHT) == 0) {
		int sym;

		end = fsm_addstate(fsm);
		if (end == NULL) {
			goto error;
		}

		fsm_setend(fsm,end,1);

		for (sym=0; sym <= UCHAR_MAX; sym++) {
			if (!fsm_addedge_literal(fsm, end, end, sym)) {
				goto error;
			}
		}
	}

	if (!trie_to_fsm(fsm, g, end)) {
		goto error;
	}

	return fsm;

error:
	if (end != NULL) {
		fsm_removestate(fsm,end);
	}

	if (fsm != NULL) {
		fsm_free(fsm);
	}

	return NULL;
}



