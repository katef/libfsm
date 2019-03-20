all: pcre libfsm

pcre: pcre.c
	gcc -o pcre -O3 -Wall -std=c89 pcre.c -lpcre

libfsm: libfsm.c
	gcc -o libfsm -O3 -Wall -std=c89 libfsm.c -I ../../include ../../build/lib/libre.a ../../build/lib/libfsm.a

