#!/bin/sh

FSM=$HOME/svn/libfsm2/src/cli/fsm
DOT='dot -Efontname=courier'

mkdir -p /tmp/fsm-view

echo "<style>tr:hover td { background-color: #f7f7f7; } td { border: solid 1px #ccc; padding: 1em; } img { margin-bottom: 1em; } td { text-align: center; font-family: monospace; }</style>" > /tmp/fsm-view/index.html
echo "<table><tr><th></th><th>Input</th><th>Expected Output</th><th>Actual Output</th></tr>" >> /tmp/fsm-view/index.html

for i in `ls -1 nfa*.fsm | sed 's/^nfa//g;s/\.fsm$//g'`; do
	echo "nfa$i.fsm -> dfa$i.fsm"
	$FSM -al dot nfa$i.fsm  | $DOT -Tpng > /tmp/fsm-view/nfa$i.png
	$FSM -al dot dfa$i.fsm | $DOT -Tpng > /tmp/fsm-view/expected$i.png
	$FSM -sm nfa$i.fsm | sed "s/'//g" | ${FSM} -al dot | $DOT -Tpng > /tmp/fsm-view/actual$i.png

	mogrify -resize '70%' -trim -transparent white /tmp/fsm-view/nfa$i.png
	mogrify -resize '70%' -trim -transparent white /tmp/fsm-view/expected$i.png
	mogrify -resize '70%' -trim -transparent white /tmp/fsm-view/actual$i.png

#	re="`grep '^# [^$]' nfa$i.fsm | sed 's/^# //g'`"
#	re="<a href='http://dioptre.org/re/?$re'>$re</a>"

	echo "<tr>" >> /tmp/fsm-view/index.html
	echo "<th>$i</th>" >> /tmp/fsm-view/index.html
	echo "<td><img src='nfa$i.png'/><br/>$re</td>" >> /tmp/fsm-view/index.html
	echo "<td><img src='expected$i.png'/><br/>$re</td>" >> /tmp/fsm-view/index.html
	echo "<td><img src='actual$i.png'/><br/>$re</td>" >> /tmp/fsm-view/index.html
	echo "</tr>" >> /tmp/fsm-view/index.html
done


