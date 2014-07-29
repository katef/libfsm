
SID ?= sid

SID_CFLAGS += -s no-numeric-terminals -s no-terminals

.for parser in ${PARSER}

SID_${parser} ?= ${parser:R}.sid
ACT_${parser} ?= ${parser:R}.act

${parser:R}.c ${parser:R}.h: ${SID_${parser}} ${ACT_${parser}}
	${SID} ${SID_CFLAGS} SID_CFLAGS_${parser} $> ${parser:R}.c ${parser:R}.h \
		|| { rm -f ${parser:R}.c ${parser}.h; false; }

${parser:R}.h: ${SID_${parser}} ${ACT_${parser}}
${parser:R}.c: ${SID_${parser}} ${ACT_${parser}}

test::
	${SID} -l test SID_${parser}

gen:: ${parser:R}.c ${parser:R}.h

.endfor

