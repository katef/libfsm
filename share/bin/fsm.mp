
pair    S[];       % centre of state
path    S[].perim; % outline of state
numeric S[].diam;  % diameter
numeric S[].loops; % number of self-edges

edgegap := 0bp; % gap between edge endpoints and states. e.g. 10bp

labeloffset := 4bp;

% find a point some distance along a path's extension
secondarydef p projectby d =
	(point 0 of p) + (d / arclength p) * ((point 1 of p) - (point 0 of p))
enddef;

vardef fsmstart(suffix n) =
	pair start;

	% TODO: assert n is a pair

	% TODO: share with fsmedge perhaps
	% TODO: parameterise 1.75
	start := n shifted (-1.75n.diam - edgegap, 0);

	path e; e := start -- n;

	(t0, whatever) = e intersectiontimes n.perim;

	drawarrow subpath(0, t0) of e withpen pencircle scaled 1bp;
enddef;

vardef fsmstate(suffix n)(expr d, final, lab) expr c =
	n       := c;
	n.diam  := d;
	n.loops := 0;

	% TODO: assert c is a pair

	n.perim = fullcircle scaled n.diam shifted n;
	draw n.perim withpen pencircle scaled 1bp;
	if final:
		n.diam := n.diam + 8bp;
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
		pair bl;
		pair ml; ml := .5[tail, head];

		if t <> -1:
			bl := point t of e;
%			draw bl withpen pencircle scaled 4bp withcolor blue;
		else:
			% XXX: find midpoint by arclength instead
			bl := point (length e / 2) of e;
%			draw bl withpen pencircle scaled 2bp withcolor blue;
		fi;

%		draw tail -- head withpen pencircle scaled 0.5bp withcolor green dashed evenly;

		% TODO: plus label delta, distancing it from point. make a label function
		% TODO: maybe an "extend" function, to extend a path in its direction. use dotprod for that?

		% lft | rt | top | bot | ulft | urt | llft | lrt
		% TODO: round label coordinates to grid? quantize rather

		% labels ought to be below, if they're below the straight line from head-tail, or above otherwise
		% TODO: find what quadrant the line is, and make this .rt/.lft instead of .top/.bot
		% TODO: threshold for considering "below"
		if ypart bl < ypart ml:
			label.bot(lab, bl);
		else:
			label.top(lab, bl);
		fi;
	fi;

enddef;

vardef fsmloop(suffix n)(expr t, lab) =
	% TODO: parameterise 0.5 for whatever suitable arc. This is unitless;
	% doesn't need to be proportional to node diam,
	% because n.perim is already relative to that.
	p := point (t - 0.5) of n.perim;
	q := point (t + 0.5) of n.perim;

	path pp; pp := n -- 0.5[p, q];
%	draw p -- q withpen pencircle scaled 0.5bp withcolor green;
%	draw 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;
%	draw n -- 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;

	% TODO: assert loops is integer
	% TODO: assert loops >= 1

	n.loops := n.loops + 1;

	% count number of self-loops for this node, and increment extension
	% parameters here are relative to node diameter
	quotient  := 0.5; % < 0 is more linear, > 1 more loggy
	expansion := 2.5;
	minradius := 0.5;
	l := length pp;
	d := (minradius + expansion * mlog(quotient * n.loops + 1) / 256) * n.diam;

	pair m; m := pp projectby d;
%	draw n -- m withpen pencircle scaled 0.5bp withcolor green;

	fsmedge(n, n)(-1, lab) n .. q .. m .. p .. n;

enddef;

