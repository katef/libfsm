all: grid.css typography.css grid.png

.PHONY: grid.png

grid.png:
	convert -size 120x18 xc:aliceblue                 \
	    -fill '#dfe' -draw 'rectangle 100,0 120,100%' \
	    -fill '#dee' -draw 'line 20,0 20,100%'        \
	    -fill '#9cf' -draw 'line 0,17 100%,17'        \
	    -alpha set -channel A -evaluate set 80%       \
	    grid.png

grid.css: grid.less
	lessc grid.less > ${.TARGET}

typography.css: typography.less
	lessc typography.less > ${.TARGET}

grid.less: config.less
typography.less: config.less

