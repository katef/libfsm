UCD_URL ?= https://www.unicode.org/Public/15.1.0/ucd/

WGET ?= wget

UCD_DATA += UnicodeData.txt
UCD_DATA += CaseFolding.txt
UCD_DATA += Scripts.txt

gen:: ${UCD_DATA}

.for data in ${UCD_DATA}
${data}:
	${WGET} ${UCD_URL}/${data}
.endfor

