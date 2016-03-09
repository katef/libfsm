#!/usr/bin/awk

# TODO:
# cat nfa.dot | dot -Tplain > /tmp/a.mp \
# && mpost -s 'outputformat="svg"' -s 'outputtemplate="%j-%c.%o"' /tmp/a.mp

# Only intended to consume Dot generated from libfsm's .dot output,
# and not graphviz diagrams in general.

function sname(s)
{
	if (s == "start") {
		return "S0";
	}

	return s;
}

# TODO: convert label from graphviz's html to a utf8
# This is either <xyz> or abc (with no quotes)
function slabel(s)
{
	# XXX: placeholder
	if (s == "<>") {
		return "";
	}
	if (s == "\"\"") {
		return "";
	}

# XXX: placeholder
gsub(/&#x3B5;/, "e", s); # XXX: why not?
gsub(/"/, "", s)
gsub(/</, "", s)
gsub(/>/, "", s)

	return s;
}

BEGIN {

	# TODO: would be simpler to gvpr to rename start to S0
	# TODO: common stuff for multiple graphs
	print "pair S[];"
	print "pair q[];"
	print "path e;"

	print "ahlength := 10bp;"
	print "ahangle  := 45;"

	print "path S[].n;"

	# diameter of node
	print "numeric S[].diam;"

	# number of self-edges
	print "numeric S[].loops;"


#	print "secondarydef v projectedalong w =";
#	print "	if pair(v) and pair(w):";
#	print "		(v dotprod w) / (w dotprod w) * w";
#	print "	else:";
#	print "		errmessage \"arguments must be pairs\"";
#	print "	fi";
#	print "enddef;";

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
	label  = slabel($7);
	style  = $8;
	shape  = $9;
	# TODO: label delimited by <>, may contain spaces

	printf "\t%s.loops := 0;\n", name

	printf "\t%s := (%fin, %fin);\n", name, x, y

# TODO: when plotting in graphviz, assume all nodes have the same width (not zero!). enough for "SXX" perhaps
# that should align them all on the same centre line. and pass fixedsize=true

	# TODO: explain this is diameter
	# TODO: store 8bp as a constant somewhere
# TODO: explain we take diameter from label presence and endstateness.
# ignore graphviz height and width, because it scales with label width and that looks bad for S1 vs S5
	diam = 0.3 + length(label) * 0.05;

	if (shape == "doublecircle") {
		printf "\t%s.diam = %fin + 8bp;\n", name, diam;
	} else {
		printf "\t%s.diam = %fin;\n", name, diam;
	}

	if (shape == "doublecircle") {
# TODO: would rather && together paths to %s.n or somesuch
		printf "\tdraw fullcircle scaled (%s.diam - 8bp) shifted %s withpen pencircle scaled 1bp;\n", name, name
	}

	printf "\t%s.n := fullcircle scaled %s.diam shifted %s;\n", name, name, name

	if ($2 != "start") {
		printf "\tdraw %s.n withpen pencircle scaled 1bp;\n", name

		if (label != "") {
			printf "\tlabel(\"%s\", %s);\n", label, name
		}
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
	# TODO: assert n >= 4 (knot, control, control); mind trailing stuff... label etc

	# TODO: note we assume -Etailclip=false -Eheadclip=false -Edir=none
	# TODO: explain that requirement


	# things i wish the graphviz documentation said about -Tplain:
	# graphviz always outputs cubic b-splines
	# 3i+1 points total. format is { knot, control, control }* knot
	# bezier is ordered tail to head
	# first two points (knot, control) are ontop of each other, in the tail node
	# last two points (control, knot) are ontop of each other, in the head node
	# coordinates are in inches

	printf "\te :=\n";
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

		printf "\t(%fin, %fin) .. controls (%fin, %fin) and (%fin, %fin) ..\n", ox, oy, c1x, c1y, c2x, c2y;
	}
	# TODO: knot at end, which i haven't drawn
	# last point:
	{
		# knot
		ox = a[(n + 0) * 2 + 3];
		oy = a[(n + 0) * 2 + 4];
		printf "\t(%fin, %fin);\n", ox, oy;
	}

	printf "\tdraw e withpen pencircle scaled 0.25bp withcolor red;\n";

	# TODO: better explain this
	# TODO: explain reversed for q, else we'd find p again, where head == tail
	printf "pair p; p = e intersectionpoint %s.n;\n", head;
	printf "pair q; q = (reverse e) intersectionpoint %s.n;\n", tail;

	# XXX: assuming no space in label
label = slabel(a[n * 2 + 5]);
lx    = (a[n * 2 + 6]);
ly    = (a[n * 2 + 7]);

	if (n * 2 == v - 4 - 2) {
		label = "";
	}

	if (head == tail) {
		printf "path pp; pp = %s -- 0.5[p, q];\n", head;
printf "draw p -- q withpen pencircle scaled 0.5bp withcolor green;\n";
printf "draw 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;\n";
printf "draw %s -- 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;\n", head;

		# extend loop 1.25 * diameter
		# count number of self-loops for this node, and increment extension
		printf "\t%s.loops := %s.loops + 1;", head, head
		print "l := length pp;"
		printf "\tpair m; m := (0.5 + 2 * mlog(%s.loops + 1) / 256) * %s.diam * unitvector(direction l of pp) shifted %s;\n", head, head, head
# u3 = u1 projectedalong u2;
		printf "\tdraw %s -- m withpen pencircle scaled 0.5bp withcolor green;\n", head

		printf "\tpath b; b = %s .. q .. m .. p .. %s;\n", tail, head;

		if (label != "") {
			# TODO: top or bottom, depending on green line direction
			printf "\tlabel.top(\"%s\", m);\n", slabel(label)
		}
	} else {
		printf "\tpath b; b = %s .. q .. p .. %s;\n", tail, head;

		# TODO: explain this. we permit a 2% lengthening threshold relative to graphviz's b-spline
		# this affects NFA especially
		print "\tr := arclength b / arclength e;\n";
		print "\tboolean g; g := (r > 1.02) or (r < 0.93);"; # XXX: or (arclength b > 6in);"; - no, wigglyness instead

		print "\tif g:";
		print "\t\tb := e;";
		print "\tfi;";

# if it wouldn't have been a straight line, go via the label point instead:
# XXX: this idea doesn't work, because i don't know which side of the line the label point would be on
#printf "\t\tb := %s .. q .. (%fin, %fin) .. p .. %s;\n", tail, lx, ly, head;

		if (label != "") {
			printf "\tpair bl;\n";
			printf "\tpair ml; ml := .5[%s, %s];\n", tail, head;

# TODO: new idea; draw right angle from midpoint, and use that to find intersection with b
# this way it should always align for multiple edges, even if on a slant

printf "draw (%fin, %fin) withpen pencircle scaled 5bp withcolor red;\n", lx, ly
printf "draw point (length b / 2) of b withpen pencircle scaled 4bp withcolor blue;\n"
#printf "draw %s -- %s withpen pencircle scaled 1bp withcolor green;\n", tail, head
#printf "draw ml withpen pencircle scaled 5bp withcolor green;\n"

			print "\tif g:";
				printf "\t\tbl := (%fin, %fin);", lx, ly;
			print "\telse:";
				printf "\t\tbl := point (length b / 2) of b;\n";
			print "\tfi;";

# TODO: plus label delta, distancing it from point. make a label function
# TODO: maybe an "extend" function, to extend a path in its direction. use dotprod for that?

			# lft | rt | top | bot | ulft | urt | llft | lrt
			# labels ought to be below, if they're below the straight line from head-tail, or above otherwise
			# TODO: round label coordinates to grid? quantize rather
# XXX: if using graphviz's point, label exactly on the spot
print "\tif g:";
# TODO: still want to distance by +/- 4bp or so
			printf "\t\tlabel(\"%s\", bl);\n", label
print "\telse:";
			printf "\tif ypart bl < ypart ml:\n" # TODO: threshold for considering "below"
			printf "\t\tlabel.bot(\"%s\", bl shifted (0, -3bp));\n", label
			printf "\telse:\n"
			printf "\t\tlabel.top(\"%s\", bl shifted (0, +3bp));\n", label
			printf "\tfi;\n"
print "\tfi;";
		}
	}

	printf "drawarrow b cutbefore q cutafter p withpen pencircle scaled 1bp;\n";
}

/^stop$/ {
	# XXX: ought to be per graph; stop is per file
	print "endfig;"
}

END {
	print "end."
}

