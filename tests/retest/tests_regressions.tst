# Various tests for regressions

# off-by-one error in VM IR assembly
R pcre
O &
O +e
~^\xff
+\xff

R pcre
O &
~|
+xyz

R pcre
O &
~
+xyz

