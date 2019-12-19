#!/usr/bin/awk -f

#
# Copyright 2019 Katherine Flavel
#
# See LICENCE for the full copyright terms.
#

# Format single characters and ranges of the form x-y to codepoint values
# in the style of the UCD formatting.
# One or both endpoints may be \xXXXX hex escapes.

BEGIN {
    FS = "-"

	for (i = 0; i <= 0xFF; i++) {
		ord[sprintf("%c", i)] = i
	}
}

function hex(x) {
	x = tolower(x)
	l = length(x)
	n = 0

	for (i = 1; i <= l; i++) {
		n *= 16
		n += index("123456789abcdef", substr(x, i, 1))
	}

	return n
}

function codepoint(c) {
	return length(c) >= 2 ? hex(c) : ord[c]
}

/-/ {
	printf "%04X..%04X\n", codepoint($1), codepoint($2)
}

/^[^-]+$/ {
	printf "%04X\n", codepoint($1)
}

