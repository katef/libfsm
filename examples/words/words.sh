#!/bin/sh

# $1=5000 number of words
# $2=1,100 for varying length words

f() {
	blab -s $1 -e "([a-zA-Z0-9]{$2} '\n'){$1}" \
	| ./words -dt \
	| cut -f2 -d: | cut -f2 -d,
}

for i in `seq 1 $(($1 / 20)) $1`; do
	f $i $2
done

