#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <assert.h>

#include <re/re.h>
#include <fsm/fsm.h>

#include "ac.h"

struct re_strings_builder {
	struct trie_graph *g;
	const struct fsm_options *opt;
	enum re_strings_flags flags;
};

struct fsm *
re_strings(const struct fsm_options *opts, const char *sv[], size_t n, enum re_strings_flags flags)
{
	struct re_strings_builder *b = NULL;
	struct fsm *fsm = NULL;
	size_t i;

	b = re_strings_new(opts, flags);
	if (b == NULL) {
		goto finish;
	}

	for (i=0; i < n; i++) {
		if (!re_strings_add(b, sv[i])) {
			goto finish;
		}
	}

	fsm = re_strings_builder_build(b);

finish:
	if (b != NULL) {
		re_strings_free(b);
	}

	return fsm;
}

struct re_strings_builder *
re_strings_new(const struct fsm_options *opt, enum re_strings_flags flags)
{
	struct re_strings_builder *b;
	static const struct re_strings_builder init;

	b = malloc(sizeof *b);
	if (b == NULL) {
		return NULL;
	}

	*b = init;

	b->g = trie_create();
	if (b->g == NULL) {
		free(b);
		return NULL;
	}

	b->opt = opt;
	b->flags = flags;

	return b;
}

void
re_strings_free(struct re_strings_builder *b)
{
	if (b->g != NULL) {
		trie_free(b->g);
		b->g = NULL;
	}

	free(b);
}

int
re_strings_add_data(struct re_strings_builder *b, const char *w, size_t wlen)
{
	assert(w != NULL);
	assert(wlen > 0);

	return (trie_add_word(b->g, w, wlen) != NULL);
}

int
re_strings_add(struct re_strings_builder *b, const char *w)
{
	assert(w != NULL);
	return re_strings_add_data(b,w,strlen(w));
}

struct fsm *
re_strings_builder_build(struct re_strings_builder *b)
{
	struct fsm *fsm = NULL;
	struct fsm_state *end = NULL;

	if ((b->flags & RE_STRINGS_ANCHOR_LEFT) == 0) {
		if (trie_add_failure_edges(b->g) < 0) {
			goto error;
		}
	}

	fsm = fsm_new(b->opt);
	if (fsm == NULL) {
		goto error;
	}

	end = NULL;
	if ((b->flags & RE_STRINGS_AC_AUTOMATON) == 0 && (b->flags & RE_STRINGS_ANCHOR_RIGHT) == 0) {
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

	if (!trie_to_fsm(fsm, b->g, end)) {
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



