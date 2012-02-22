/* $Id$ */

var Abbrtitle = new (function () {

	var abbrs = [
		[ 'API', 'Application Programer Interface' ],	/* TODO: right? */
		[ 'CLI', 'Command Line Interface'          ]
	];

	function settitle(a) { 
		for (var i in abbrs) {
			if (a.innerHTML == abbrs[i][0]) {
				a.setAttribute('title', abbrs[i][1]);
			}
		}
	}

	this.init = function (root) {
		var a;

		a = root.getElementsByTagName('abbr');
		for (var i = 0; i < a.length; i++) {
			if (!a[i].getAttribute('title')) {
				settitle(a[i]);
			}
		}

		a = root.getElementsByTagName('acronym');
		for (var i = 0; i < a.length; i++) {
			if (!a[i].getAttribute('title')) {
				settitle(a[i]);
			}
		}
	}

});

