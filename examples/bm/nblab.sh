#!/bin/sh

# generate text approximately however many characters long
# $1 - a.blab
# $2 - 5

t=''
while [ ${#t} -lt $(($2 / 2)) -o ${#t} -gt $(($2 * 2))  ]; do
	t="`blab -f $(($2 * 2)) $1`"
done

echo -n "$t"

