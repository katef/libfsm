UCD_FTP ?= http://www.unicode.org/Public/10.0.0/ucd/

WGET ?= wget

UCD_DATA += UnicodeData.txt
UCD_DATA += CaseFolding.txt
UCD_DATA += Scripts.txt

gen:: ${UCD_DATA}

.for data in ${UCD_DATA}
${data}:
	${WGET} ${UCD_FTP}/${data}
.endfor

