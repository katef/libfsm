
pair    S[];       % centre of state
numeric S[].diam;  % diameter
numeric S[].loops; % number of self-edges

% find a point some distance along a path's extension
secondarydef p projectby d =
	(point 0 of p) + (d / arclength p) * ((point 1 of p) - (point 0 of p))
enddef;

vardef fsmstart(suffix n) =
	pair start;

	% TODO: assert n is a pair

	% TODO: share with fsmedge perhaps
	% TODO: parameterise 1.5
	start := n shifted (-1.5n.diam, 0);

	fsmedge(start, n)(-1, "") start -- n;
enddef;

vardef fsmstate(suffix n)(expr d, final, lab) expr c =
	n       := c;
	n.diam  := d;
	n.loops := 0;

	% TODO: assert c is a pair

	draw fullcircle scaled n.diam shifted n;
	if final:
		n.diam := n.diam + 8bp;
		draw fullcircle scaled n.diam shifted n;
	fi;

	if lab <> \"\":
		label(lab, n);
	fi;
enddef;

vardef fsmedge(suffix tail, head)(expr t, lab) expr e =

	drawarrow e withpen pencircle scaled 1bp;

	% TODO: assert tail, head are pairs
	% TODO: assert t is a time
	% TODO: assert lab is a string
	% TODO: assert e is a path

	% TODO: account for node diameter here

	if lab <> \"\":
		pair bl;
		pair ml; ml := .5[tail, head];

		if t <> -1:
			bl := point t of e;
%			draw bl withpen pencircle scaled 4bp withcolor blue;
		else:
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
			label.bot(lab, bl shifted (0, -3bp));
		else:
			label.top(lab, bl shifted (0, +3bp));
		fi;
	fi;

enddef;

vardef fsmloop(suffix n)(expr p, q, lab) =
	path pp; pp := n -- 0.5[p, q];
%	draw p -- q withpen pencircle scaled 0.5bp withcolor green;
%	draw 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;
%	draw node -- 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;

	% TODO: assert loops is integer
	% TODO: assert loops >= 1

	% count number of self-loops for this node, and increment extension
	% parameters here are relative to node diameter
	quotient  := 0.5; % < 0 is more linear, > 1 more loggy
	expansion := 2.5;
	minradius := 0.5;
	l := length pp;
	d := (minradius + expansion * mlog(quotient * n.loops + 1) / 256) * n.diam;

	pair m; m := pp projectby d;
%	draw n -- m withpen pencircle scaled 0.5bp withcolor green;

	% TODO: when cutting is done in fsmedge, no need to do it here
	path b; b := n .. q .. m .. p .. n;
	fsmedge(n, n)(-1, lab) b cutbefore q cutafter p;

enddef;

