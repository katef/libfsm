
pair    S[];       % centre of state
path    S[].perim; % outline of state
numeric S[].diam;  % diameter
numeric S[].loops; % number of self-edges

edgegap := 0bp; % gap between edge endpoints and states. e.g. 10bp
endgap  := 8bp; % gap between inner and outer circle for end states

% time (as a subpath of fullcircle) of the arc between loop endpoints.
% larger means a wider stance for the loop; 0 means the loop will be
% a straight line. Negatives will cause the loop to be drawn in reverse.
looptime := 1.0;

% self-loop parameters here are relative to node diameter
quotient  := 0.5; % < 0 is more linear, > 1 more loggy
expansion := 2.5;
minradius := 0.5;

% metapost variables
labeloffset := 8bp;
ahlength    := 8bp; % TODO: multiple of space between double circles
ahangle     := 45;

% find a point some distance along a path's extension
secondarydef p projectby d =
	(point 0 of p) + (d / arclength p) * ((point 1 of p) - (point 0 of p))
enddef;

vardef fsmlabel(suffix tail, head)(expr t, lab) expr e =
	% TODO: assert t is a time
	% TODO: assert lab is a string
	% TODO: assert e is a path

	% The position for the label is placed according to the angle at that
	% point in the curve, outset to either side. The side from which to outset
	% the label depends on whether the curve is convex or concave at that point.
	% This is found here by shifting and roating the curve to be horizontal from
	% the origin, and seeing if the label's point is above or below 0.

	% TODO: round label coordinates to grid? quantize rather
	% TODO: fuzzy threshold for considering near enough to be on the line
	% TODO: account for label string length

	% tail is the start point of the line. so shifting to -tail negates that offset,
	% effectively moving the line to start from the origin.
	pair clt; clt = tail shifted -tail;
	pair clh; clh = head shifted -tail;
%	draw clt -- clh withcolor green dashed evenly;

	% draw (tail shifted -tail) -- (head shifted -tail) withcolor green dashed evenly;
	%draw e shifted -tail withcolor green;

	% now angle(clh) should be the angle of the clt -- clh.
	% i think that could also be calculated as angle(head - tail)
	numeric a;

	if clh = (0, 0):
		a = 0;
	else:
		a = angle clh;
	fi;

	transform tt;
	tt = identity shifted -tail rotated -a;

	path ce; ce = e transformed tt;
	draw ce withpen pencircle scaled 0.5bp withcolor green;

	pair bl; bl = point t of ce;
	draw bl withpen pencircle scaled 3bp withcolor blue;

	pair c;
	c = (0, if ypart bl < 0: - fi labeloffset);

	numeric gradient; gradient = (angle direction t of ce); % mod 180;

	pair cc; cc = c rotated gradient shifted bl;
	draw bl -- cc withpen pencircle scaled 0.5bp;

	label(lab, cc transformed inverse tt);
enddef;

vardef fsmstart(suffix n) =
	pair start;

	% TODO: assert n is a pair

	% TODO: share with fsmedge perhaps
	% TODO: parameterise 1.75
	start = n shifted (-1.75n.diam - edgegap, 0);

	path e; e = start -- n;

	(t0, whatever) = e intersectiontimes n.perim;

	drawarrow subpath(0, t0) of e withpen pencircle scaled 1bp;
enddef;

vardef fsmstate(suffix n)(expr d, isend, lab) expr c =
	n       := c;
	n.diam  := d;
	n.loops := 0;

	% TODO: assert c is a pair
	% TODO: assert isend is boolean
	% TODO: assert lab is a string

	n.perim = fullcircle scaled n.diam shifted n;
	draw n.perim withpen pencircle scaled 1bp;
	if isend:
		n.diam := n.diam + endgap;
		n.perim := fullcircle scaled n.diam shifted n;
		draw n.perim withpen pencircle scaled 1bp;
	fi;

	n.perim := fullcircle scaled (n.diam + edgegap) shifted n;

	if lab <> \"\":
		label(lab, n);
	fi;
enddef;

vardef fsmedge(suffix tail, head)(expr t, lab) expr e =
	numeric ta, tb;

	% TODO: assert tail, head are pairs
	% TODO: assert t is a time
	% TODO: assert lab is a string
	% TODO: assert e is a path

%	draw e withcolor red;

	% Edges are drawn from node centre points for caller simplicity. Here we
	% find the subpath which excludes the regions inside the nodes at each end,
	% so as to draw an arrow from the perimeter of one node to the next.
	%
	% For self loops, the head is the same as the tail. It's also possible for
	% an edge to intersect a node multiple times. But intersectiontimes always
	% finds the first intersection, so the usual approach would be to iteratively
	% cut the path after each point to find each of the remaining intersections.
	%
	% However, we're interested in just the first and last intersection points.
	% My solution here is to find the latter point along the reverse of e,
	% and then to subtract that from e's length to give a time in terms of
	% e in its forward direction.

	(           ta, whatever) =         e intersectiontimes tail.perim;
	(length e - tb, whatever) = reverse e intersectiontimes head.perim;

	drawarrow subpath(ta, tb) of e withpen pencircle scaled 1bp;

	if lab <> \"\":
		numeric tx; % time at which to place label

		if t <> -1:
			tx = t;
		else:
			tx = arctime (arclength e / 2) of e;
		fi;

		fsmlabel(tail, head)(tx, lab) e;
	fi;
enddef;

vardef fsmloop(suffix n)(expr t, lab) =
	pair p, q;
	pair m;
	numeric l;
	numeric d;

	% TODO: assert lab is a string
	% TODO: assert loops is integer
	% TODO: assert loops >= 0

	p = point (t - looptime / 2) of n.perim;
	q = point (t + looptime / 2) of n.perim;

	pp := n -- 0.5[p, q];
%	draw p -- q withpen pencircle scaled 0.5bp withcolor green;
%	draw 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;
%	draw n -- 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;

	n.loops := n.loops + 1;

	% count number of self-loops for this node, and increment extension
	l = length pp;
	d = (minradius + expansion * mlog(quotient * n.loops + 1) / 256) * n.diam;

	m = pp projectby d;
%	draw n -- m withpen pencircle scaled 0.5bp withcolor green;

	fsmedge(n, n)(-1, lab) n .. q .. m .. p .. n;

enddef;

