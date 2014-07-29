
MKDIR ?= mkdir

mkdir:: ${DIR}

.for dir in ${DIR}
${dir}:
	${MKDIR} -p ${dir}
.endfor

