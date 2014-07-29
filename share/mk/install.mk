
INSTALL ?= install

.for stage in ${STAGE_BUILD} ${STAGE_COPY}

DIR_${stage}  ?= ${stage:H}
MODE_${stage} ?= 644

.for dir in ${DIR_${stage}}
STAGE_DIR += ${dir}
MODE_${dir} ?= 755
.endfor

.endfor

# some awkwardness to avoid :O:u for OpenBSD make(1)
STAGE_DIR != echo ${STAGE_DIR} | tr ' ' '\n' | sort | uniq

install::
.for dir in ${STAGE_DIR}
	${INSTALL} -m ${MODE_${dir}} -d ${PREFIX}/${dir}
.endfor
.for stage in ${STAGE_BUILD}
	${INSTALL} -m ${MODE_${stage}} ${BUILD}/${stage} ${PREFIX}/${DIR_${stage}}/${stage:T}
.endfor
.for stage in ${STAGE_COPY}
	${INSTALL} -m ${MODE_${stage}} ${stage} ${PREFIX}/${DIR_${stage}}/${stage:T}
.endfor

.for stage in ${STAGE_BUILD}
install:: ${BUILD}/${stage}
.endfor
.for stage in ${STAGE_COPY}
install:: ${stage}
.endfor

