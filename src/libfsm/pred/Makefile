.include "../../../share/mk/top.mk"

# predicates
SRC += src/libfsm/pred/isany.c
SRC += src/libfsm/pred/isdfa.c
SRC += src/libfsm/pred/isend.c
SRC += src/libfsm/pred/iscomplete.c
SRC += src/libfsm/pred/epsilonsonly.c
SRC += src/libfsm/pred/hasincoming.c
SRC += src/libfsm/pred/hasoutgoing.c
SRC += src/libfsm/pred/hasepsilons.c
SRC += src/libfsm/pred/hasnondeterminism.c

.for src in ${SRC:Msrc/libfsm/pred/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

