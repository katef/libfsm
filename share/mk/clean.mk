
REMOVE ?= rm -f
RMDIR  ?= rm -r

.for dir in ${DIR}
clean::
.if !empty(CLEAN:M${dir}/*:M*.mk)
	${REMOVE} ${CLEAN:M${dir}/*:M*.mk}
.endif
.if !empty(CLEAN:M${dir}/*:N*.mk)
	${REMOVE} ${CLEAN:M${dir}/*:N*.mk}
.endif
.endfor

.for dir in ${DIR} ${BUILD}
.if exists(${dir})
clean::
	${RMDIR} ${dir}
.endif
.endfor

