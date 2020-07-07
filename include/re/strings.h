/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_STRINGS_H
#define RE_STRINGS_H

struct fsm;
struct fsm_options;

struct re_strings;

enum re_strings_flags {
	RE_STRINGS_ANCHOR_LEFT  = 1 << 0,
	RE_STRINGS_ANCHOR_RIGHT = 1 << 1,

	/*
	 * This signals that the automaton should be a standard Aho-Corasick
	 * automaton. An Aho-Corasick automaton graph is equivalent
	 * to RE_STIRNGS_ANCHOR_RIGHT, but the execution is slightly different.
	 *
	 * With standard FSM automatons, a match is only reported when the FSM
	 * ends on an end state.
	 *
	 * In Aho-Corasick, each time an end state is encountered, the state
	 * machine should record/report a match. This is not yet implemented
	 * and intended for future development.
	 */
	RE_STRINGS_AC_AUTOMATON = 1 << 2
};

/*
 * A convenience to iterate over an array of strings, and call
 * re_strings_add_str() for each.
 */
struct fsm *
re_strings(const struct fsm_options *opt, const char *a[], size_t n,
	enum re_strings_flags flags);

struct re_strings *
re_strings_new(void);

void
re_strings_free(struct re_strings *g);

int
re_strings_add_raw(struct re_strings *g, const void *p, size_t n);

int
re_strings_add_str(struct re_strings *g, const char *s);

int
re_strings_build(struct fsm *fsm, struct re_strings *g,
	enum re_strings_flags flags);

struct fsm *
re_strings_build_new(struct re_strings *g,
	const struct fsm_options *opt, enum re_strings_flags flags);

#endif

