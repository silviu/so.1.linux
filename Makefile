CFLAGS = -Wall -g


mini-shell: mini-shell.o parser/parser.tab.o parser/parser.yy.o

clean: 
	rm -rf mini-shell.o mini-shell
	cd parser; make clean
