
SID ?= sid

SID_CFLAGS += -s no-numeric-terminals -s no-terminals

.for parser in ${PARSER}

${parser:R}.c ${parser:R}.h: ${parser} ${parser:R}.act
	${SID} ${SID_CFLAGS} $> ${parser:R}.c ${parser:R}.h \
		|| { rm -f ${parser:R}.c ${parser}.h; false; }

${parser:R}.h: ${parser} ${parser:R}.act
${parser:R}.c: ${parser} ${parser:R}.act

test::
	${SID} -l test ${parser}

gen:: ${parser:R}.c ${parser:R}.h

.endfor

