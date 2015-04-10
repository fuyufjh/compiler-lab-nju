CC		= gcc
CFLAGS	= -lfl -ly
CFILES	= $(shell find -name "*.c")

parser: syntax.tab.c lex.yy.c $(CFILES)
	$(CC) *.c -o parser $(CFLAGS)

syntax.tab.h: syntax.y
	bison -d syntax.y

syntax.tab.c: syntax.y
	bison -d syntax.y

lex.yy.c: lexical.l syntax.tab.h
	flex lexical.l

clean:
	rm -f *.tab.c *.yy.c *.tab.h parser *.output
