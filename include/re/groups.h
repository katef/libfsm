/*
 * Copyright 2026 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_GROUPS_H
#define RE_GROUPS_H

struct re_pos;

/*
 * esc is the character for escaping group references,
 * typically '\\' or '$'.
 *
 * group0 is passed separately for caller convenience,
 * so you don't have to construct a single array for
 * all groups. It's supposed to be the entire string
 * that matched. group0 may not be NULL.
 *
 * groupv is 0-indexed meaning group $1 onwards.
 * groupc is the count of elements in groupv.
 *
 * nonexistent is what to do about references to groups
 * that are outside the bounds of the array. NULL means
 * to error, otherwise the string value will be used.
 * Typically this would be passed as "".
 *
 * start,end are only populated on error.
 *
 * You can distinguish compile-time errors (that is,
 * syntax errors in the format string) vs. runtime errors
 * (that is, nonexistent groups) by calling
 * re_interpolate_groups() ahead of time with groupc = 0
 * and passing a non-NULL nonexistent value.
 *
 * The output string will always be less than or equal in
 * length to the format string. The output is \0-terminated.
 * outn includes the \0.
 *
 * outs may be NULL in which case outn must be 0, and no
 * output is made.
 */
bool
re_interpolate_groups(const char *fmt, char esc,
	const char *group0, unsigned groupc, const char *groupv[], const char *nonexistent,
	char *outs, size_t outn,
	struct re_pos *start, struct re_pos *end);

#endif

