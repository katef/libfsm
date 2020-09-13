/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <re/re.h>
#include <re/strings.h>

/* This is a heavily customized version of examples/words */

static struct fsm_options opt;

struct word_list {
	char **list;
	size_t len;
	size_t cap;
};

void wordlist_init(struct word_list *words, size_t cap)
{
	static const struct word_list zero;

	*words = zero;

	words->cap = cap;
	words->list = malloc(cap * sizeof(words->list[0]));
	if (words->list == NULL) {
		perror("allocating word list");
		exit(EXIT_FAILURE);  /* YOLO! */
	}
}

void wordlist_add(struct word_list *words, const char *w0) {
	size_t wlen;
	char *w;

	if (words->len >= words->cap) {
		size_t newcap;
		char **wl;

		if (words->cap < 16384) {
			newcap = words->cap * 2;
		} else {
			newcap = words->cap + words->cap/4;
		}

		wl = malloc(newcap * sizeof wl[0]);
		if (wl == NULL) {
			perror("adding word");
			exit(EXIT_FAILURE);
		}

		memcpy(wl, words->list, words->len * sizeof words->list[0]);
		free(words->list);

		words->list = wl;
		words->cap = newcap;
	}

	wlen = strlen(w0);
	w = malloc(wlen+1);
	if (w == NULL) {
		perror("adding word");
		exit(EXIT_FAILURE);  /* YOLO! */
	}

	memcpy(w,w0,wlen+1);
	words->list[words->len++] = w;
}

void wordlist_finalize(struct word_list *words)
{
	static const struct word_list zero;
	size_t i;

	assert(words != NULL);

	for (i=0; i < words->len; i++) {
		free(words->list[i]);
	}

	free(words->list);
	*words = zero;
}

int main(int argc, char *argv[]) {
	struct fsm *fsm;
	char s[BUFSIZ];
	void (*print)(FILE *, const struct fsm *);

	const char *pname = argv[0];

	int anchored_left  = 1;
	int anchored_right = 1;

	enum re_strings_flags flags;

	struct word_list words;

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	print = fsm_print_fsm;

	{
		int c;

		while (c = getopt(argc, argv, "h" "d" "lru"), c != -1) {
			switch (c) {
			case 'u': anchored_left  = 0;
				  anchored_right = 0;  break;

			case 'l': anchored_left  = 1;  break;
			case 'r': anchored_right = 1;  break;

			case 'd': print = fsm_print_dot; break;

			case 'h':
			case '?':
			default:
				goto usage;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc > 0) {
		goto usage;
	}

	wordlist_init(&words, 1024);

	while (fgets(s, sizeof s, stdin) != NULL) {
		const char *p = s;

		s[strcspn(s, "\n")] = '\0';
		wordlist_add(&words, p);
	}

	flags = 0;
	if (anchored_left) {
		flags |= RE_STRINGS_ANCHOR_LEFT;
	}

	if (anchored_right) {
		flags |= RE_STRINGS_ANCHOR_RIGHT;
	}

	fsm = re_strings(&opt, (const char **)words.list, words.len, flags);
	wordlist_finalize(&words);

	if (fsm == NULL) {
		perror("converting trie to fsm");

		exit(EXIT_FAILURE);
	}

	if (print != NULL) {
		print(stdout, fsm);
	}

	fsm_free(fsm);

	return 0;

usage:

	fprintf(stderr, "usage: %s [-dMN]\n", pname);
	fprintf(stderr, "       %s -h\n", pname);

	return 1;
}

