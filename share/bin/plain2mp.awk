#!/usr/bin/awk

# TODO:
# cat nfa.dot | dot -Tplain > /tmp/a.mp \
# && mpost -s 'outputformat="svg"' -s 'outputtemplate="%j-%c.%o"' /tmp/a.mp

# Only intended to consume Dot generated from libfsm's .dot output,
# and not graphviz diagrams in general.

# graph 1 5.75 0.93056
# node start 0.15278 0.41667 0.30556 0.5 "" solid none black lightgrey
# node S6 0.97222 0.41667 0.30556 0.30556 "" solid circle black lightgrey
# node S1 5.5417 0.20833 0.41667 0.41667 "" solid doublecircle black lightgrey
# edge S3 S6 10 3.5244 0.20119 3.2727 0.19014 2.7322 0.17358 2.2778 0.20833 1.873 0.23929 1.7755 0.28067 1.375 0.34722 1.3389 0.35322 1.3009 0.35974 1.2635 0.3663 <&#x3B5;> 2.3264 0.3125 solid black
# edge S3 S2 4 3.835 0.20833 3.9555 0.20833 4.1328 0.20833 4.2831 0.20833 <y> 4.1319 0.3125 solid black
# stop

function sname(s)
{
	if (s == "start") {
		return "S0";
	}

	return s;
}

BEGIN {

	# TODO: would be simpler to gvpr to rename start to S0
	# TODO: common stuff for multiple graphs
	print "pair S[];"
	print "pair q[];"
	print "path nS[];"
	print "path e;"

	# diameter of node
	print "numeric dS[];"

	# number of self-edges
	print "numeric lS[];"

	print "ahlength := 10bp;"
	print "ahangle  := 45;"
}

/^graph / {
	printf "beginfig(%s)\n", $2
}

# node S6 0.97222 0.41667 0.30556 0.30556 "" solid circle black lightgrey
# node name x y width height label style shape color fillcolor
/^node / {
	name   = sname($2);
	x      = $3;
	y      = $4;
	width  = $5;
	height = $6;
	label  = $7;
	style  = $8;
	shape  = $9;
	# TODO: label delimited by <>, may contain spaces

	printf "\tl%s := 0;", name

	printf "\t%s := (%fin, %fin);\n", name, x, y

	# TODO: explain this is diameter
	printf "\td%s = %fin * 0.75;\n", name, (height + width) / 2;

	if (shape == "doublecircle") {
# TODO: would rather && together paths to n%s or somesuch
		printf "\tdraw fullcircle scaled (d%s - 8bp) shifted %s withpen pencircle scaled 1bp;\n", name, name
	}

	printf "\tn%s := fullcircle scaled d%s shifted %s;\n", name, name, name

	if ($2 != "start") {
		printf "\tdraw n%s withpen pencircle scaled 1bp;\n", name
	}
}

# edge tail head n x1 y1 .. xn yn [label xl yl] style color
# edge S3 S6 10 3.5244 0.20119 3.2727 0.19014 2.7322 0.17358 2.2778 0.20833 1.873 0.23929 1.7755 0.28067 1.375 0.34722 1.3389 0.35322 1.3009 0.35974 1.2635 0.3663 <&#x3B5;> 2.3264 0.3125 solid black
/^edge / {
	tail = sname($2);
	head = sname($3);
	n    = $4;

	v = split($0, a, " ");
	# TODO: assert n < v - whatever
	# TODO: assert n >= 4 (knot, control, control)

	# TODO: note we assume -Etailclip=false -Eheadclip=false -Edir=none
	# TODO: explain that requirement


	# things i wish the graphviz documentation said about -Tplain:
	# graphviz always outputs cubic b-splines
	# 3i+1 points total. format is { knot, control, control }* knot
	# bezier is ordered tail to head
	# first two points (knot, control) are ontop of each other, in the tail node
	# last two points (control, knot) are ontop of each other, in the head node
	# coordinates are in inches

	printf "\te := ";
	for (i = 1; i <= n - 1; i += 3) {
		# knot
		ox = a[(i + 0) * 2 + 3];
		oy = a[(i + 0) * 2 + 4];

		# control
		c1x = a[(i + 1) * 2 + 3];
		c1y = a[(i + 1) * 2 + 4];

		# control
		c2x = a[(i + 2) * 2 + 3];
		c2y = a[(i + 2) * 2 + 4];

		printf "(%fin, %fin) .. controls (%fin, %fin) and (%fin, %fin) .. ", ox, oy, c1x, c1y, c2x, c2y;
	}
	# TODO: knot at end, which i haven't drawn
	# last point:
	{
		# knot
		ox = a[(n + 0) * 2 + 3];
		oy = a[(n + 0) * 2 + 4];
		printf "(%fin, %fin);", ox, oy;
	}

	printf "draw e withpen pencircle scaled 0.25bp withcolor red;\n";

	# TODO: better explain this
	# TODO: explain reversed for q, else we'd find p again, where head == tail
	printf "pair p; p = e intersectionpoint n%s;\n", head;
	printf "pair q; q = (reverse e) intersectionpoint n%s;\n", tail;

	if (head == tail) {
		printf "path pp; pp = %s -- 0.5[p, q];\n", head;
#		printf "draw p -- q withpen pencircle scaled 0.5bp withcolor green;\n";

		# extend loop 1.25 * diameter
		# count number of self-loops for this node, and increment extension
		printf "\tl%s := l%s + 1;", head, head
		print "l := length pp;"
		printf "pair m; m := (0.5 + 2 * mlog(l%s + 1) / 256) * d%s * unitvector(direction l of pp) shifted %s;\n", head, head, head
		printf "draw %s -- m withpen pencircle scaled 0.25bp withcolor green;\n", head

		printf "path b; b = %s .. p .. m .. q .. %s;\n", head, tail;
	} else {
		printf "path b; b = %s .. p .. q .. %s;\n", head, tail;
	}

	printf "drawarrow reverse (b cutbefore p cutafter q) withpen pencircle scaled 1bp;\n";
}

/^stop$/ {
	# XXX: ought to be per graph; stop is per file
	print "endfig;"
}

END {
	print "end."
}

