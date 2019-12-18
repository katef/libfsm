/* generated */

#include "class.h"

static const struct range ranges[] = {
	{ 0x0000UL, 0x0008UL },
	{ 0x000EUL, 0x0019UL },
	{ 0x0021UL, 0x00FFUL }
};

const struct class class_notspace = {
	"notspace",
	ranges,
	sizeof ranges / sizeof *ranges
};

