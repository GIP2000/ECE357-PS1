all: kitty
PHONY: clean

kitty: kitty.c
	gcc -o kitty kitty.c

clean: 
	rm -f kitty *.txt
