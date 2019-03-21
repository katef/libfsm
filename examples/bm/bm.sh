#!/bin/sh

# $1 ./pcre
# $2 /usr/share/dict/words
# $3 re.blab
# $4 1000

for i in `seq 1 $(($4 / 100)) $4`; do
	for j in `seq 1 10`; do
		re="`./nblab.sh $3 $i`"
		($1 "$re" < $2 2> /dev/null || echo 0) | tr -d '\n'
		echo -n ' '
	done
	echo
done

