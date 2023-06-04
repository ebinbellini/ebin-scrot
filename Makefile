ebin-scrot: *.c 
	tcc *.c drw/*.c -lX11 -lXft -lpng \
	-lfontconfig -lm \
	-I/usr/include/freetype2 \
	-o ebin-scrot


all: ebin-scrot


install: all
	cp -f ./ebin-scrot /usr/bin/
	chmod 755 /usr/bin/ebin-scrot
