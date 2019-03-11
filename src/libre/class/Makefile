.include "../../../share/mk/top.mk"

UCD ?= share/ucd
UTF8DFA ?= examples/utf8dfa/utf8dfa

UNI_SCRIPT != cat ${UCD}/Scripts.txt \
	| sed 's/\#.*//' | cut -d';' -f2 | sort | uniq

.for script in ${UNI_SCRIPT}

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
UNI_CAT != cat ${UCD}/UnicodeData.txt \
	| cut -d';' -f3 | sort | uniq | grep -v 'Cc'

# some awkwardness to avoid :O:u for OpenBSD make(1)
UNI_CAT_UNIQ != echo ${UNI_CAT:C/(.)./\1/} | tr ' ' '\n' | sort | uniq

.for group in ${UNI_CAT_UNIQ}

# This could be faster if we kept the minimized DFA for each category
src/libre/class/utf8_${group}.c:
	cat ${UCD}/UnicodeData.txt \
	| awk -F';' -v group='^${group}' '$$3 ~ group { print $$1 }' \
	| ${UTF8DFA} -e utf8_${group}_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_${group}.c

.endfor

.for cat in ${UNI_CAT}

src/libre/class/utf8_${cat}.c:
	cat ${UCD}/UnicodeData.txt \
	| awk -F';' -v cat='${cat}' '$$3 == cat { print $$1 }' \
	| ${UTF8DFA} -e utf8_${cat}_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_${cat}.c

.endfor

src/libre/class/utf8_private.c:
	printf "F0000..FFFFD\n100000..10FFFD\n" \
	| ${UTF8DFA} -e utf8_private_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_private.c

# Plane 15/16 private use range start/end explicitly excluded here
src/libre/class/utf8_assigned.c:
	cat ${UCD}/UnicodeData.txt \
	| grep -v -E '^(F0000|FFFFD|100000|10FFFD)$$' \
	| cut -d';' -f1 \
	| ${UTF8DFA} -e utf8_assigned_ \
	> ${.TARGET}

SRC += src/libre/class/utf8_assigned.c

# borrowed from https://www.w3.org/International/questions/qa-forms-utf-8
src/libre/class/utf8_all.c:
	echo -n > ${BUILD}/${.TARGET:R}.re
	echo -n '[\x09\x0A\x0D\x20-\x7E]'            >> ${BUILD}/${.TARGET:R}.re # ASCII
	echo -n '|[\xC2-\xDF][\x80-\xBF]'            >> ${BUILD}/${.TARGET:R}.re # non-overlong 2-byte
	echo -n '|\xE0[\xA0-\xBF][\x80-\xBF]'        >> ${BUILD}/${.TARGET:R}.re # excluding overlongs
	echo -n '|[\xE1-\xEC\xEE\xEF][\x80-\xBF]{2}' >> ${BUILD}/${.TARGET:R}.re # straight 3-byte
	echo -n '|\xED[\x80-\x9F][\x80-\xBF]'        >> ${BUILD}/${.TARGET:R}.re # excluding surrogates
	echo -n '|\xF0[\x90-\xBF][\x80-\xBF]{2}'     >> ${BUILD}/${.TARGET:R}.re # planes 1-3
	echo -n '|[\xF1-\xF3][\x80-\xBF]{3}'         >> ${BUILD}/${.TARGET:R}.re # planes 4-15
	echo -n '|\xF4[\x80-\x8F][\x80-\xBF]{2}'     >> ${BUILD}/${.TARGET:R}.re
	${RE} -pl api -e utf8_all_ -y ${BUILD}/src/libre/class/utf8_all.re \
	> ${.TARGET}

SRC += src/libre/class/utf8_all.c

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

gen:: src/libre/class.h
src/libre/class.h:
	printf "/* generated */\n" > ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "struct fsm_options;\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#ifndef RE_CLASS_H\n" >> ${.TARGET}
	printf "#define RE_CLASS_H\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	for class in ${SRC:Msrc/libre/class/*.c:T:R:Mutf8_*}; do \
		printf "struct fsm *%s_fsm(const struct fsm_options *);\n" $${class}; \
	done >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	for class in ${SRC:Msrc/libre/class/*.c:T:R:Nutf8_*}; do \
		printf "struct fsm *%s_fsm(const struct fsm_options *);\n" class_$${class}; \
	done >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#endif\n" >> ${.TARGET}

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

# libfsm generates unsigned constants which are converted to char
.if ${CC:T:Mgcc}
.for src in ${SRC:Msrc/libre/class/*.c}
CFLAGS.${src} += -Wno-overflow
.endfor
.endif

