.include "../../share/mk/top.mk"

SRC += src/fsm/main.c
# SRC += src/fsm/wordgen.c
# SRC += src/fsm/xoroshiro256starstar.c

PROG += fsm

.for src in ${SRC:Msrc/fsm/main.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for lib in ${LIB:Mlibfsm}
${BUILD}/bin/fsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/fsm/*.c}
${BUILD}/bin/fsm: ${BUILD}/${src:R}.o
.endfor


UNAME ?= uname
UNAME_SYSTEM != ${UNAME} -s
SYSTEM ?= ${UNAME_SYSTEM}

.if ${SYSTEM} == Linux

.if exists(/lib/libc.so.6)
LDD_BLURB?= /lib/libc.so.6 2>&1
.else
LDD_BLURB?= ldd --version  2>&1
.endif

LDD_NAME!=                          \
    ${LDD_BLURB}                    \
    | { read v && case "$$v" in     \
        *EGLIBC*)    echo eglibc;;  \
        *GLIBC*)     echo glibc;;   \
        *GNU\ libc*) echo glibc;;   \
        *GNU\ C*)    echo glibc;;   \
        *musl*)      echo musl;;    \
        *)           echo unknown;; \
    esac }

LDD_VER!=                        \
    ${LDD_BLURB}                 \
    | { read v && case "$$v" in  \
        *GLIBC*)     echo "$$v" | sed -n 's/^ldd (.* E\?GLIBC .*) //p';;  \
        *GNU\ libc*) echo "$$v" | sed -n 's/^ldd (GNU libc) //p';;        \
        *GNU\ C*)    echo "$$v" | sed -n 's/^.*version \(.*\), .*/\1/p';; \
        MUSL)        echo "$$v" | sed -n 's/^Version \(.*\)/\1/p';;       \
        *)           echo unknown;; \
    esac }

.if ${LDD_NAME} == "glibc" || ${LDD_NAME} == "eglibc"

# clock_gettime(2):
# Link with -lrt (only for glibc versions before 2.17).
.if ${LDD_VER} < 2.17
LFLAGS.fsm += -lrt
.endif

.endif

.endif

