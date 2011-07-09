CFLAGS = -Wall -g

build: mini-shell

mini-shell: mini-shell.o parser/parser.tab.o parser/parser.yy.o

test: build
	make -C test/ -f Makefile.checker
clean: 
	rm -rf mini-shell.o mini-shell
	cd parser; make clean
