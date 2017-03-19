#!/usr/bin/awk

#
# Copyright 2008-2017 Katherine Flavel
#
# See LICENCE for the full copyright terms.
#

BEGIN {
	print "<?xml version='1.0'?>"
	print "<tests>"
}

{
	printf "\t<test input=    '%s'\n"    \
	       "\t      expected= '%s'\n"    \
	       "\t      actual=   '%s'/>\n", \
	       $1, $2, $3
}

END {
	print "</tests>"
}

