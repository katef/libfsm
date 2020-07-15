/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include <re/re.h>
#include <re/strings.h>

#include "ac.h"

struct fsm *
re_strings(const struct fsm_options *opt, const char *a[], size_t n,
	enum re_strings_flags flags)
{
	struct re_strings *g;
	struct fsm *fsm;
	fsm_state_t start;
	size_t i;

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		return NULL;
	}

	g = re_strings_new();
	if (g == NULL) {
		fsm_free(fsm);
		return NULL;
	}

	for (i = 0; i < n; i++) {
		if (!re_strings_add_str(g, a[i])) {
			goto error;
		}
	}

	if (!re_strings_build(fsm, &start, g, flags)) {
		goto error;
	}

	re_strings_free(g);

	fsm_setstart(fsm, start);

	return fsm;

error:

	fsm_free(fsm);
	re_strings_free(g);

	return NULL;
}

struct re_strings *
re_strings_new(void)
{
	return (struct re_strings *) trie_create();
}

void
re_strings_free(struct re_strings *g)
{
	if (g != NULL) {
		trie_free((struct trie_graph *) g);
	}
}

int
re_strings_add_raw(struct re_strings *g, const void *p, size_t n)
{
	assert(p != NULL);
	assert(n > 0);

	return trie_add_word((struct trie_graph *) g, p, n) != NULL;
}

int
re_strings_add_str(struct re_strings *g, const char *s)
{
	assert(s != NULL);

	return re_strings_add_raw(g, s, strlen(s));
}

/* internal convenience to avoid constructing a string */
int
re_strings_add_concat(struct re_strings *g, const struct ast_expr **a, size_t n)
{
	assert(a != NULL);
	assert(n > 0);

	return trie_add_concat((struct trie_graph *) g, a, n) != NULL;
}

int
re_strings_build(struct fsm *fsm, fsm_state_t *start,
	struct re_strings *g, enum re_strings_flags flags)
{
	fsm_state_t end;
	int have_end;

	assert(fsm != NULL);
	assert(start != NULL);

	if ((flags & RE_STRINGS_ANCHOR_LEFT) == 0) {
		if (trie_add_failure_edges((struct trie_graph *) g) < 0) {
			goto error;
		}
	}

	have_end = 0;

	if ((flags & RE_STRINGS_AC_AUTOMATON) == 0 && (flags & RE_STRINGS_ANCHOR_RIGHT) == 0) {
		if (!fsm_addstate(fsm, &end)) {
			goto error;
		}

		have_end = 1;
		fsm_setend(fsm, end, 1);

		if (!fsm_addedge_any(fsm, end, end)) {
			goto error;
		}
	} else {
		end = (unsigned) -1; /* appease clang */
	}

	if (!trie_to_fsm(fsm, start, (struct trie_graph *) g, have_end, end)) {
		goto error;
	}

	return 1;

error:

	return 0;
}

