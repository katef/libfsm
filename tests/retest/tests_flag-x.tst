# RE_EXTENDED flag: /x

# only in PCRE
R pcre

M x
O &
O +e
~^test this # comment here
+testthis
-test this
-test this # comment here

M 0
O &
O +e
~^test this # comment here
-testthis
-test this
+test this # comment here

M x
O &
O +e
~^test this #comment here\nno comment$
+testthisnocomment
-testthisnocommentextra
-testthis
-test this
-test thisno comment
-test this no comment
-test this #comment here\nno comment

# tests for (?x) and (?-x)

M 0
O &
O +e
~^test this (?x)and this$
+test this andthis
-testthisandthis
-test this and this

M 0
O &
O +e
~^test this (?x)and this # but not this\nalso that$
+test this andthisalsothat
-test this andthisalsothatextra
-testthisandthisalsothat
-test this and this # but not this\nalso that
-test this and this\nalso that

M 0
O &
O +e
~^test this (?x)and this # but not this\n(?-x)also that$
+test this andthisalso that
-test this andthisalso thatextra
-test this andthisalsothat
-test this andthisalsothatextra
-testthisandthisalso that
-test this and this # but not this\nalso that
-test this and this\nalso that

# (?-x) is in the comment, so it has no effect
M 0
O &
O +e
~^test this (?x)and this # but not this (?-x)\nalso that$
+test this andthisalsothat
-test this andthisalso that
-test this andthisalsothatextra
-testthisandthisalsothat
-test this and this # but not this\nalso that
-test this and this\nalso that

# start with /x, (?-x) turns it off
M x
O &
O +e
~^test this (?-x)and this # but not this
+testthisand this # but not this
+testthisand this # but not this and extra
-testthisandthis
-test this and this # but not this
