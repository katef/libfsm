
.for dir in ${SUBDIR}

.include "${dir}/Makefile"

DIR += ${BUILD}/${dir}

.endfor

