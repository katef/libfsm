
% find a point some distance along a path's extension
secondarydef p projectby d =
	(point 0 of p) + (d / arclength p) * ((point 1 of p) - (point 0 of p))
enddef;

vardef fsmnode(expr node, diam, final, lab) =
	draw fullcircle scaled diam shifted node withpen pencircle scaled 1bp;
	if final:
		draw fullcircle scaled (diam - 8bp) shifted node withpen pencircle scaled 1bp;
	fi;
	if lab <> \"\":
		label(lab, node);
	fi;
enddef;


vardef fsmedge(expr tail, head, e, t, lab) =

	drawarrow e withpen pencircle scaled 1bp;

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

vardef fsmloop(expr node, diam, p, q, lab, loops) =
	path pp; pp = node -- 0.5[p, q];
%	draw p -- q withpen pencircle scaled 0.5bp withcolor green;
%	draw 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;
%	draw node -- 0.5[p, q] withpen pencircle scaled 0.8bp withcolor green;

	% extend loop 1.25 * diameter
	% count number of self-loops for this node, and increment extension
	l := length pp;
	d := (1.5 + 2 * mlog(loops + 2) / 256) * diam;
	pair m; m := pp projectby d;
%	draw node -- m withpen pencircle scaled 0.5bp withcolor green;

	path b; b := node .. q .. m .. p .. node;
	fsmedge(node, node, b cutbefore q cutafter p, -1, lab);

enddef;

