/* $Id$ */

/*
 * A javascript implementation of align and valign for <col/>.
 *
 * This carries through the alignment attributes for each cell in a table,
 * aligning them as per their respective <col/> declaration.
 *
 * At the time of writing (2010), only IE and Opera appear to implement these
 * attributes for <col/> - other browsers ignore alignment entirely, and are
 * likely to remain so as attention further falls to CSS for alignment.
 * However, I want to use <col/>, so here it is.
 *
 * Rowspan handling adapted from Csaba Gabor,
 * http://bytes.com/topic/javascript/answers/519470-misleading-cellindex-values-rowspan-colspan
 */

var Colalign = new (function () {

	function colalign(t) { 
		var cells = []; 
		var cols  = []; 

		function setcell(cell, y, x) {
			for (var i = 0; i < (cell.rowSpan || 1); i++) {
				if (!cells[y + i]) {
					cells[y + i] = [];
				}

				for (var j = 0; j < (cell.colSpan || 1); j++) {
					cells[y + i][x + j] = cell;
				}
			}

			if (!cols[x]) {
				return;
			}

			if (!cell.getAttribute('align') && cols[x].align)  {
				cell.align  = cols[x].align;
			}

			if (!cell.getAttribute('vAlign') && cols[x].vAlign) {
				cell.vAlign = cols[x].vAlign;
			}
		}

		function nextcol(y) {
			for (var x = 0; cells[y] && cells[y][x]; x++)
				;

			return x;
		}

		var w = t.getElementsByTagName('col');
		if (w.length == 0) {
			return;
		}

		for (var n = 0, x = 0; n < w.length; n++) {
			for (var j = 0; j < (w[n].span || 1); j++) {
				cols[x++] = w[n];
			}
		}

		for (var y = 0; y < t.rows.length; y++) { 
			for (var n = 0; n < t.rows[y].cells.length; n++) { 
				setcell(t.rows[y].cells[n], y, nextcol(y));
			}
		} 
	}

	this.init = function (root) {
		var a;

		a = root.getElementsByTagName('table');
		for (var i = 0; i < a.length; i++) {
			colalign(a[i]);
		}
	}

});

