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

BEGIN {
	# TODO: common stuff for multiple graphs
	print "pair start;"
	print "pair S[];"
	print "pair q[];"
	print "path nS[];"
	print "path nstart;"
	print "path e;"
	print "ahlength := .1in;"
}

/^graph / {
	printf "beginfig(%s)\n", $2
}

/^node / {
	# node S6 0.97222 0.41667 0.30556 0.30556 "" solid circle black lightgrey
	# node name x y width height label style shape color fillcolor

	name = $2;
	x = $3;
	y = $4;
	width = $5;
	height = $6;
	# TODO: label delimited by <>, may contain spaces

	diameter = (height + width) / 2;

#	printf "\t%% %s %s\n", name, $0
#	printf "\t%s := fullcircle scaled %fin shifted (%fin, %fin);\n", name, diameter, x, y
#	printf "\tdraw %s;\n", name
	printf "\t%s := (%fin, %fin);\n", name, x, y

	# TODO: style for doublecircle
	# TODO: style for start state

	printf "\tn%s := fullcircle scaled %fin shifted %s;\n", name, diameter, name

	printf "\tdraw n%s;\n", name
}

#/^edge S1 S5 / {
/^edge / {
	# edge tail head n x1 y1 .. xn yn [label xl yl] style color
	# edge S3 S6 10 3.5244 0.20119 3.2727 0.19014 2.7322 0.17358 2.2778 0.20833 1.873 0.23929 1.7755 0.28067 1.375 0.34722 1.3389 0.35322 1.3009 0.35974 1.2635 0.3663 <&#x3B5;> 2.3264 0.3125 solid black

	tail = $2;
	head = $3;
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

#	printf "\te := %s .. ", tail;
#	for (i = 1; i <= n; i++) {
#		# control point
#		cx = a[i * 2 + 3];
#		cy = a[i * 2 + 4];
#
#		# knot
##		ox = a[i * 2 + 5];
##		oy = a[i * 2 + 6];
#
#		printf "(%sin, %sin) .. ", cx, cy;
##		printf "(%sin, %sin) .. controls (%sin, %sin) and (%sin, %sin) ..", ox, oy, cx, cy, cx, cy;
#	}
#	printf "%s;\n", head;

	#printf "draw e withcolor green;\n";
	#printf "draw point .5 of e withpen pencircle scaled 3bp withcolor blue;\n";
	#printf "draw e cutafter %s withcolor black;\n", head;

	for (i = 1; i <= n; i++) {
		ox = a[i * 2 + 3];
		oy = a[i * 2 + 4];
		printf "draw (%sin, %sin) withpen pencircle scaled 4bp withcolor black;\n", ox, oy;
#		printf "label.top(\"%d\", (%sin, %sin));\n", i, ox, oy;
	}

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

		printf "draw (%fin, %fin) withpen pencircle scaled 3bp withcolor red;\n", ox, oy;
		printf "draw (%fin, %fin) withpen pencircle scaled 2bp withcolor red + green;\n", c1x, c1y;
		printf "draw (%fin, %fin) withpen pencircle scaled 2bp withcolor red + green;\n", c2x, c2y;
	}
	# TODO: knot at end, which i haven't drawn
	# last point:
	{
		# knot
		ox = a[(n + 0) * 2 + 3];
		oy = a[(n + 0) * 2 + 4];
		printf "draw (%fin, %fin) withpen pencircle scaled 1bp withcolor red;\n", ox, oy;
	}


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


## TODO: better explain this
printf "pair p; p = e intersectionpoint n%s;\n", head;
printf "pair q; q = e intersectionpoint n%s;\n", tail;

##printf "draw p withpen pencircle scaled 3bp withcolor blue;\n";
##printf "draw q withpen pencircle scaled 3bp withcolor blue;\n";

printf "path b; b = %s .. p .. q .. %s;\n", head, tail;

printf "drawarrow b cutbefore p cutafter q withpen pencircle scaled 0.5bp withcolor blue;\n";
##printf "\tdraw fullcircle scaled %fin shifted %s;\n", diameter, name
#print "endfig;end."
}

/^stop$/ {
	print "endfig;"
}

END {
	print "end."
}

