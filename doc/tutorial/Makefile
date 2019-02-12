
RE  ?= ../../build/bin/re -pl dot
DOT ?= dot -Tsvg -Efontname=sans -Nfontname=sans -Efontsize=15 | rsvg-convert -az .55 -f svg
DIA ?= dia -t svg -e -

all: re0.svg
all: re2.svg
all: re3.svg
all: re4.svg
all: re5.svg
all: trie.svg trie.c
all: libfsm.svg
all: phases.svg
all: phase-ast.svg phase-nfa.svg phase-dfa.svg phase-ir.svg phase-code.c
all: phase-rrd.svg

re0.svg:
	${RE} '^a.b([0-9]|ef)*$$' | ${DOT} > $@

re2.svg:
	${RE}     '^abc$$' '^abcde+fx$$' '^afx$$' | ${DOT} > $@

re3.svg:
	${RE} -z  '^abc$$' '^abcde+fx$$' '^afx$$' | ${DOT} > $@

re4.svg:
	${RE} -uz '^xyz*$$' '^xyz$$' '^xy?z?$$' | sed 's/label = <#.,#./fixedsize = true, width = 1.25, &/' | ${DOT} > $@

re5.svg:
	${RE} -r literal 'abc' | ${DOT} > $@

phase-ast.svg:
	${RE} -pl ast '^ab+c?.?$$' | dot -Tsvg -Efontname=sans -Nfontname=sans -Efontsize=14 | rsvg-convert -az .55 -f svg > $@

phase-rrd.svg:
	${RE} -pl abnf '^ab+c?.?$$' \
	| kgt -l abnf -e svg > $@

phase-nfa.svg:
	${RE} -np '^ab+c?.?$$' | fsm -pt trim | fsm -capl dot | ${DOT} > $@

phase-dfa.svg:
	${RE} -apl dot '^ab+c?.?$$' | ${DOT} > $@

phase-ir.svg:
	${RE} -pl ir '^ab+c?.?$$' | ${DOT} > $@

phase-code.c:
	${RE} -pl c '^ab+c?.?$$' > $@

trie.svg:
	grep '^fine' /usr/share/dict/words \
	| grep -E '^[a-z]+$$' \
	| xargs ${RE} -z -r literal \
	| sed 's/shape = doublecircle, /shape = doublecircle, fixedsize = true, width=0.5, /' \
	| ${DOT} > trie.svg

trie.c:
	grep '^fine' /usr/share/dict/words \
	| grep -E '^[a-z]+$$' \
	| xargs ${RE} -zpl c -r literal \
	> trie.c

libfsm.svg:
	${RE} -cb '[Ll]ibf+(sm)*' '[Ll]ibre' | ${DOT} > $@

phases.svg: phases.dia
	${DIA} $< | rsvg-convert -az .45 -f svg > $@

