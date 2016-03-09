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

	# TODO: centralise to my equivalent of automata.mp, and include that

	print "vardef fsmnode(expr node, diam, final, lab) =";
	print "	draw fullcircle scaled diam shifted node withpen pencircle scaled 1bp;"
	print "	if final:"
	print "		draw fullcircle scaled (diam - 8bp) shifted node withpen pencircle scaled 1bp;"
	print "	fi;"
	print "	if lab <> \"\":"
	print "		label(lab, node);"
	print "	fi;"
	print "enddef;";


	print "vardef fsmedge(expr tail, head, e, lab) =";

	print "	drawarrow e withpen pencircle scaled 1bp;";

	print "	if lab <> \"\":"
	print "		pair bl;";
	print "		pair ml; ml := .5[tail, head];"

	# TODO: new idea; draw right angle from midpoint, and use that to find intersection with b
	# this way it should always align for multiple edges, even if on a slant

	print "		draw point (length b / 2) of b withpen pencircle scaled 4bp withcolor blue;"
#	print "		draw tail -- head withpen pencircle scaled 1bp withcolor green;"
#	print "		draw ml withpen pencircle scaled 5bp withcolor green;"

	print "		bl := point (length b / 2) of b;"

	# TODO: plus label delta, distancing it from point. make a label function
	# TODO: maybe an "extend" function, to extend a path in its direction. use dotprod for that?

	# lft | rt | top | bot | ulft | urt | llft | lrt
	# TODO: round label coordinates to grid? quantize rather

	# labels ought to be below, if they're below the straight line from head-tail, or above otherwise
	# TODO: find what quadrant the line is, and make this .rt/.lft instead of .top/.bot
	print "		if ypart bl < ypart ml:" # TODO: threshold for considering "below"
	print "			label.bot(lab, bl shifted (0, -3bp));"
	print "		else:"
	print "			label.top(lab, bl shifted (0, +3bp));"
	print "		fi;"
	print "	fi;"

	print "enddef;";

	print "vardef fsmloop(expr node, diam, p, q, lab, loops) ="
	print "	path pp; pp = node -- 0.5[p, q];"
	print "	draw p -- q withpen pencircle scaled 0.5bp withcolor green;"
	print "	draw 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;"
	print "	draw node -- 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;"

	# extend loop 1.25 * diameter
	# count number of self-loops for this node, and increment extension
	print "	l := length pp;"
	print "	pair m; m := (0.5 + 2 * mlog(loops + 2) / 256) * diam * unitvector(direction l of pp) shifted node;"
	# u3 = u1 projectedalong u2;
	print "	draw node -- m withpen pencircle scaled 0.5bp withcolor green;"

	print "	path b; b := node .. q .. m .. p .. node;"
	print "	fsmedge(node, node, b cutbefore q cutafter p, lab);"

	print "enddef;";

	print "vardef fsmgvedge(expr tail, taildiam, head, headdiam, e, lab, loops) =";
	print "	draw e withpen pencircle scaled 0.25bp withcolor red;";

	# TODO: better explain this
	# TODO: explain reversed for q, else we'd find p again, where head == tail
	print "	path sn; sn := fullcircle scaled headdiam shifted head;" # outline
	print "	path st; st := fullcircle scaled taildiam shifted tail;" # outline
	print "	pair p; p = e intersectionpoint sn;\n";
	print "	pair q; q = (reverse e) intersectionpoint st;\n";

	print "	if head = tail:"
	print "		fsmloop(head, headdiam, p, q, lab, loops);"
	print "	else:"
	print "		path b; b = tail .. q .. p .. head;"
	# TODO: explain this. we permit a 2% lengthening threshold relative to graphviz's b-spline
	# this affects NFA especially
	# XXX: or (arclength b > 6in), but wigglyness would be a better metric
	print "		r := arclength b / arclength e;";
	print "		if (r > 1.02) or (r < 0.93):";
	print "			b := e;"
	print "		fi;"
	print "		fsmedge(tail, head, b cutbefore q cutafter p, lab);"
	print "	fi;"

	print "enddef;";
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

	# TODO: quantise node positions
	printf "\t%s := (%fin, %fin);\n", name, x, y
	printf "\t%s.loops := 0;\n", name

# TODO: when plotting in graphviz, assume all nodes have the same width (not zero!). enough for "SXX" perhaps
# that should align them all on the same centre line. and pass fixedsize=true

	# TODO: explain this is diameter
	# TODO: store 8bp as a constant somewhere
# TODO: explain we take diameter from label presence and endstateness.
# ignore graphviz height and width, because it scales with label width and that looks bad for S1 vs S5
	diam = 0.3 + length(label) * 0.05;

	final = shape == "doublecircle";

	if (final) {
		printf "\t%s.diam = %fin + 8bp;\n", name, diam;
	} else {
		printf "\t%s.diam = %fin;\n", name, diam;
	}

	if ($2 != "start") {
		printf "\tfsmnode(%s, %s.diam, %s, \"%s\");\n", name, name, final ? "true" : "false", label
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

	# XXX: assuming no space in label
label = slabel(a[n * 2 + 5]);
lx    = (a[n * 2 + 6]);
ly    = (a[n * 2 + 7]);

	if (n * 2 == v - 4 - 2) {
		label = "";
	}

	# TODO: find what time (lx,ly) is along graphviz's edge, e
	# then pass the time and use that for placing our label on our edge
	# use intersectiontimes to find the time of (lx,ly) along e
	# or find by e cutbefore (lx,ly) and use arctime to find the time wrt e

	printf "fsmgvedge(%s, %s.diam, %s, %s.diam, e, \"%s\", %s.loops);", tail, tail, head, head, label, head;

	if (head == tail) {
		print "	%s.loops := %s.loops + 1;", head, head
	}
}

/^stop$/ {
	# XXX: ought to be per graph; stop is per file
	print "endfig;"
}

END {
	print "end."
}

