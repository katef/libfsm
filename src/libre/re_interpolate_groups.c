/*
 * Copyright 2026 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include <re/re.h>
#include <re/groups.h>

// TODO
#define OUT_CHAR(c) do { if (outn < 1) { goto error; } *outs++ = (c); outn--; } while (0)
#define OUT_GROUP(s) do { if (outn < strlen((s))) { goto error; } outs += sprintf(outs, "%s", (s)); outn -= strlen((s)); } while (0)

// TODO: return values: syntax error, nonexistent group error (digit overflow is the same thing), success
bool
re_interpolate_groups(const char *fmt, char esc,
	const char *group0, unsigned groupc, const char *groupv[], const char *nonexistent,
	char *outs, size_t outn,
	struct re_pos *pos)
{
	unsigned group; // 0 meaning group0, 1 meaning groupv[0], etc
	const char *p;

	enum {
		STATE_LIT,
		STATE_ESC,
		STATE_DIGIT
	} state;

	assert(esc != '\0');
	assert(group0 != NULL || groupc == 0);
	assert(groupc < UINT_MAX / 10 - 1);
	assert(outs != NULL);

	state = STATE_LIT;
	group = 0;

	p = fmt;
	do {
		switch (state) {
		case STATE_LIT:
			if (*p == '\0') {
				break;
			}

			if (*p == esc) {
				state = STATE_ESC;
				continue;
			}

			OUT_CHAR(*p);
			continue;

		case STATE_ESC:
			if (*p == '\0') {
				goto error;
			}

			if (*p == esc) {
				OUT_CHAR(esc);
				state = STATE_LIT;
				continue;
			}

			if (isdigit((unsigned char) *p)) {
				group = *p - '0';
				state = STATE_DIGIT;
				continue;
			}

			goto error;

		case STATE_DIGIT:
			if (isdigit((unsigned char) *p)) {
				group *= 10;
				group += *p - '0';

// TODO: explain this
// digit overflow, we cap to groupc + 1
// groupc + 1 is always out of bounds
// this is a simple way to avoid needing to handle digit overflow for subsequent digits,
// assuming groupc *= 10 is <= UINT_MAX
				if (group > groupc) {
					group = groupc + 1;
				}
				continue;
			}

			if (group == 0) {
				OUT_GROUP(group0);
			} else if (group <= groupc) {
				assert(groupv[group - 1] != NULL);
				OUT_GROUP(groupv[group - 1]);
			} else if (nonexistent == NULL) {
// TODO: maybe want to indicate this independently from syntax errors
// TODO: no need, you can pre-check the entire syntax by running with 0 groups
				goto error;
			} else {
				OUT_GROUP(nonexistent);
			}

			group = 0;
			state = STATE_LIT;

			if (*p == '\0') {
				break;
			}

			if (*p == esc) {
				state = STATE_ESC;
				continue;
			}

			OUT_CHAR(*p);
			continue;

		default:
			assert(!"unreached");
			goto error;
		}
	} while (*p++);

	if (state != STATE_LIT) {
		goto error;
	}

	OUT_CHAR('\0');

	return true;

error:

	// TODO: track start,end independently
	if (pos != NULL) {
		pos->byte = p - fmt;
	}

	return false;
}

