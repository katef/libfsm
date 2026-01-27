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

#define OUT_CHAR(c)                  \
    do if (outs != NULL) {           \
        if (outn < 1) goto overflow; \
        *outs++ = (c);               \
        outn--;                      \
    } while (0)

#define OUT_GROUP(s)                 \
    do if (outs != NULL) {           \
        size_t n = strlen((s));      \
        if (outn < n) goto overflow; \
        (void) memcpy(outs, s, n);   \
        outs += n;                   \
        outn -= n;                   \
    } while (0)

bool
re_interpolate_groups(const char *fmt, char esc,
	const char *group0, unsigned groupc, const char *groupv[], const char *nonexistent,
	char *outs, size_t outn,
	struct re_pos *start, struct re_pos *end)
{
	unsigned group; // 0 meaning group0, 1 meaning groupv[0], etc
	char *outs_orig;
	const char *p;

	enum {
		STATE_LIT,
		STATE_ESC,
		STATE_DIGIT
	} state;

	assert(esc != '\0');
	assert(group0 != NULL || groupc == 0);
	assert(groupc < UINT_MAX / 10 - 1);
	assert(outs != NULL || outn == 0);

	state = STATE_LIT;
	group = 0;

	outs_orig = outn > 0 ? outs : NULL;

	if (start != NULL) {
		start->byte = 0;
	}

	p = fmt;

	do {
		switch (state) {
		case STATE_LIT:
			if (*p == '\0') {
				break;
			}

			if (*p == esc) {
				if (start != NULL) {
					start->byte = p - fmt;
				}

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

				/*
				 * We need to handle numeric overflow somehow here,
				 * as we would with using strtol() or similar. But
				 * we don't need to distinguish this as a special
				 * error code, semantically it's the same as a group
				 * that doesn't exist.
				 *
				 * groupc + 1 is always out of bounds. So we cap to that,
				 * using it as a simple way to avoid needing to handle
				 * numeric overflow for subsequent digits. This assumes
				 * groupc *= 10 is <= UINT_MAX.
				 */
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
				/*
				 * We could indicate this independently from syntax errors,
				 * with some way to return different error codes.
				 *
				 * But there's no need, you can pre-check the fmt syntax
				 * by running ahead of time with groupc == 0 and pass
				 * nonexistent != NULL, because that eliminates the
				 * possibility for group-related errors.
				 */
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
				if (start != NULL) {
					start->byte = p - fmt;
				}

				state = STATE_ESC;
				continue;
			}

			OUT_CHAR(*p);
			continue;

		default:
			assert(!"unreached");
			goto error;
		}
	} while (*p != '\0' && p++);

	if (state != STATE_LIT) {
		goto error;
	}

	OUT_CHAR('\0');

	return true;

overflow:

	/* we're blaming the entire fmt string for overflow */
	if (start != NULL) {
		start->byte = 0;
	}

error:

	if (end != NULL) {
		end->byte = p - fmt;
	}

	if (outs_orig != NULL) {
		*outs_orig = '\0';
	}

	return false;
}

