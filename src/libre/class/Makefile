.include "../../../share/mk/top.mk"

UCD ?= share/ucd
UTF8DFA ?= examples/utf8dfa/utf8dfa

UTF8_SCRIPT != cat ${UCD}/Scripts.txt \
	| sed 's/\#.*//' | cut -d';' -f2 | sort | uniq

.for script in ${UTF8_SCRIPT}

src/libre/class/utf8_${script}.c:
	cat ${UCD}/Scripts.txt \
	| sed 's/\#.*//' \
	| tr -d ' ' \
	| grep -v '^$$' \
	| awk -F';' -v script='^${script}' '$$2 ~ script { print $$1 }' \
	| ${UTF8DFA} -e utf8_${script}_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_${script}.c

.endfor

# XXX: Cc excluded
UTF8_CAT != cat ${UCD}/UnicodeData.txt \
	| cut -d';' -f3 | sort | uniq | grep -v 'Cc'

.for group in ${UTF8_CAT:C/(.)./\1/:u}

# This could be faster if we kept the minimized DFA for each category
src/libre/class/utf8_${group}.c:
	cat ${UCD}/UnicodeData.txt \
	| awk -F';' -v group='^${group}' '$$3 ~ group { print $$1 }' \
	| ${UTF8DFA} -e utf8_${group}_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_${group}.c

.endfor

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

