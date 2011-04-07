#!/bin/sh

FSM=$HOME/svn/libfsm3/src/cli/fsm
DOT='dot -Efontname=courier'

mkdir -p /tmp/fsm-view

echo "<style>tr:hover td { background-color: #f7f7f7; } td { border: solid 1px #ccc; padding: 1em; } img { margin-bottom: 1em; } td { text-align: center; font-family: monospace; }</style>" > /tmp/fsm-view/index.html
echo "<table><tr><th></th><th>Input</th><th>Expected Output</th><th>Actual Output</th></tr>" >> /tmp/fsm-view/index.html

for i in `ls -1 in*.fsm | sed 's/^in//g;s/\.fsm$//g'`; do
	echo "in$i.fsm -> out$i.fsm"
	$FSM -cal dot in$i.fsm  | $DOT -Tpng > /tmp/fsm-view/in$i.png
	$FSM -cal dot out$i.fsm | $DOT -Tpng > /tmp/fsm-view/expected$i.png
	$FSM -m in$i.fsm | ${FSM} -cal dot | $DOT -Tpng > /tmp/fsm-view/actual$i.png

	mogrify -resize '70%' -trim -transparent white /tmp/fsm-view/in$i.png
	mogrify -resize '70%' -trim -transparent white /tmp/fsm-view/expected$i.png
	mogrify -resize '70%' -trim -transparent white /tmp/fsm-view/actual$i.png

	re="`grep '^# [^$]' in$i.fsm | sed 's/^# //g'`"
	re="<a href='http://dioptre.org/re/?$re'>$re</a>"

	echo "<tr>" >> /tmp/fsm-view/index.html
	echo "<th>$i</th>" >> /tmp/fsm-view/index.html
	echo "<td><img src='in$i.png'/><br/>$re</td>" >> /tmp/fsm-view/index.html
	echo "<td><img src='expected$i.png'/><br/>$re</td>" >> /tmp/fsm-view/index.html
	echo "<td><img src='actual$i.png'/><br/>$re</td>" >> /tmp/fsm-view/index.html
	echo "</tr>" >> /tmp/fsm-view/index.html
done


