all: pcre libfsm

# repetition to smooth out contextual noise
BM_MAX ?= 100
BM_CFLAGS += -DBM_MAX=${BM_MAX}

pcre: pcre.c
	gcc -o pcre -O3 -Wall -std=c99 ${BM_CFLAGS} pcre.c -lpcre

libfsm: libfsm.c
	gcc -o libfsm -O3 -Wall -std=c99 ${BM_CFLAGS} libfsm.c -I ../../include ../../build/lib/libre.a ../../build/lib/libfsm.a

clean:
	rm -f pcre libfsm

