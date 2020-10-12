#ifndef TYPE_INFO_CAPTURE_STRING_SET_H
#define TYPE_INFO_CAPTURE_STRING_SET_H

#include "theft_libfsm.h"

#define MAX_CAPTURE_STRINGS 8
#define MAX_CAPTURES 4
#define MAX_CAPTURE_STRING_LENGTH 16
#define MAX_TOTAL_CAPTURES (MAX_CAPTURE_STRINGS * MAX_CAPTURES)

/* abcd* */

struct capture_env {
	char tag;
	uint8_t letter_count;	/* e.g. 4 = "abcd"; 0 < count <= 26 */
	uint8_t string_maxlen;	/* must be < MAX_CAPTURE_STRING_LENGTH */
};

struct css_end_opaque {
#define CSS_END_OPAQUE_TAG 'C'
	unsigned char tag;
	unsigned ends;		/* bit set */
};

struct capture_string_set {
	uint8_t count;
	struct capstring {
		char string[MAX_CAPTURE_STRING_LENGTH];
		uint8_t capture_count;
		struct capture {
			uint8_t offset;
			uint8_t length;
		} captures[MAX_CAPTURES];
	} capture_strings[MAX_CAPTURE_STRINGS];
};

extern const struct theft_type_info type_info_capture_string_set;

#endif
