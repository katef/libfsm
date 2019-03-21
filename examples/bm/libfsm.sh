#!/bin/sh -e

./libfsm $* | gcc -o /tmp/bm-$$ -xc -Wall -std=c89 -O3 -
/tmp/bm-$$ "$1"

