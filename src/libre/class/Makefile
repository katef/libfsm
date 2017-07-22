.include "../../../share/mk/top.mk"

UCD ?= share/ucd
UTF8DFA ?= examples/utf8dfa/utf8dfa

.for cat in ${UTF8_CAT}

src/libre/class/utf8_${cat}.c:
	cat ${UCD}/UnicodeData.txt \
	| awk -F';' -v cat='${cat}' '$$3 == cat { print $$1 }' \
	| ${UTF8DFA} -e utf8_${cat}_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_${cat}.c

.endfor

CLASS_alnum  = [[:alpha:][:digit:]]
CLASS_alpha  = [[:upper:][:lower:]]
CLASS_any    = .
CLASS_ascii  = [\x00-\x7F]
CLASS_blank  = [ \t]
CLASS_cntrl  = [\x00-\x1F\x7F]
CLASS_digit  = [0-9]
CLASS_graph  = [\x21-\x7E]
CLASS_lower  = [a-z]
CLASS_print  = [\x20-\x7E]
CLASS_punct  = [!"\#$$%&'\''()*+,\-./:;<=>?@\\\^_`{|}~]|\[|]
CLASS_space  = [ \t\r\n\v\f]
CLASS_spchr  = [ ]
CLASS_upper  = [A-Z]
CLASS_word   = [[:alnum:]_]
CLASS_xdigit = [A-Fa-f0-9]

.for class in alnum alpha any ascii blank cntrl digit graph lower \
	print punct space spchr upper word xdigit

src/libre/class/${class}.c:
	${RE} -pl api -e class_${class}_ '${CLASS_${class}}' > ${.TARGET}

SRC += src/libre/class/${class}.c

.endfor

.for src in ${SRC:Msrc/libre/class/*.c}
gen:: ${src}
.endfor

.for src in ${SRC:Msrc/libre/class/*.c}
CFLAGS.${src} += -I src/libre # XXX: for class.h
DFLAGS.${src} += -I src/libre # XXX: for class.h
.endfor

.for src in ${SRC:Msrc/libre/class/*.c}
CFLAGS.${src} += -D LF_HEADER='"class.h"'
DFLAGS.${src} += -D LF_HEADER='"class.h"'
.endfor

.for src in ${SRC:Msrc/libre/class/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

