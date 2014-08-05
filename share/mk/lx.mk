
LX ?= lx

LX_CFLAGS +=

.for lexer in ${LEXER}

.for ext in c h

${lexer:R}.${ext}: ${lexer}
	${LX} -l ${ext} ${LX_CFLAGS} ${LX_CFLAGS_${lexer}} < ${.ALLSRC:M*.lx} > ${lexer:R}.${ext} \
		|| { rm -f ${lexer:R}.${ext}; false; }

gen:: ${lexer:R}.${ext}

.endfor

test::
	${LX} -l test < ${lexer}

.endfor

