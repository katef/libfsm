#!/usr/bin/awk -f

#
# Copyright 2019 Katherine Flavel
#
# See LICENCE for the full copyright terms.
#

# Two kinds of input line:
# 11A50
# 11A51..11A56
#
# The input is assumed to be sorted, and to not contain overlaps.
#
# Run as awk -v class=xyz

BEGIN {
	print "/* generated */"
	print ""

	print "#include \"class.h\""
	print ""

	print "static const struct range ranges[] = {"

	FS = "\\.\\."

	x = -1
	y = -1
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

function dispatch(x, y, comma) {
	printf "\t{ 0x%04XUL, 0x%04XUL }%s\n", x, y, comma ? "," : ""
}

function range(i, n) {
	if (x == -1) {
		x = i
		y = x + n
		return
	}

	if (y + 1 == i) {
		y += n + 1
		return
	}

	dispatch(x, y, 1)

	x = i
	y = x + n
}

/^./ {
	a = hex($1)
	b = hex($2)

	range(a, $2 ? b - a : 0)
}

END {
	if (x != -1) {
		dispatch(x, y, 0)
	}

	print "};"
	print ""

	printf "const struct class %s = {\n", class
	printf "\tranges,\n"
	printf "\tsizeof ranges / sizeof *ranges\n"
	printf "};\n"
	printf "\n"
}

