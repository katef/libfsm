#!/usr/bin/awk -f

#
# Copyright 2020 Katherine Flavel
#
# See LICENCE for the full copyright terms.
#

# convert one regexp per line to reperf format test cases:
#
# cat list-o-regexps | awk -f share/bin/reperf.awk | ./build/bin/reperf -qtH min - \
# | awk 'NR > 1 { print $2, $3, $4, $5 }' | iguff

{
	print "-", NR
	print "M", $0
	print "D pcre"
	print "N 1"
	print "R 0"
	print "X"
	print ""
}

