# RE_SINGLE flag: /s

R like
# /s is the default for this dialect

M 0
~x_y
+x#y
+x\ny

M s
~x_y
+x#y
+x\ny

R sql
# /s is the default for this dialect

M 0
~x_y
+x#y
+x\ny
+x_y

M s
~x_y
+x#y
+x\ny
+x_y

M 0
~x%y
+x##y
+x\n\ny
+x%%y

M s
~x%y
+x##y
+x\n\ny
+x%%y

R literal
# this dialect has no wildcards

M 0
~x.y
-x_y
-x\ny
-x#y
+x.y

M s
~x.y
-x_y
-x\ny
-x#y
+x.y

R glob
# /s is the default for this dialect

M 0
~x?y
+x_y
+x\ny
+x.y

M s
# setting /s has no effect, and there's no way to un-set it
~x?y
+x_y
+x\ny
+x#y

R native
# /s is the default for this dialect

M 0
~x.y
-x\ny
+x#y

M s
~x.y
+x\ny
+x#y

R pcre
# /s is the default for this dialect

M 0
~x.y
+x#y
-x\ny
+x.y

M s
~x.y
+x#y
+x\ny
+x#y

M 0
~x.(?s).y
+x##y
+x#\ny
-x\n#y
-x\n\ny

