#! /bin/sh

jq --raw-output \
    '.[]                              | 
     select(.parsed)                  | 
     [ .pattern, 
       (.matches|map("+" + .)|join("\n")), 
       (.not_matches|map("-" + .)|join("\n")) 
     ]                                | 
     select(.[0] != null)             |
     select(.[1] != "" or .[2] != "") | 
     [ (.[]|select(. != "")) ]        |
     join("\n") + "\n"                |
     @text' $@

