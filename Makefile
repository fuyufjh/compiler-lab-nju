CC		= gcc
CFLAGS	= -lfl -ly -g
CFILES	= $(shell find -name "*.c")
HFILES	= $(shell find -name "*.h")

parser: syntax.tab.c lex.yy.c $(CFILES) $(HFILES)
	$(CC) *.c -o parser $(CFLAGS)
	@ctags -R

syntax.tab.h: syntax.y
	bison -d syntax.y

syntax.tab.c: syntax.y
	bison -d syntax.y

lex.yy.c: lexical.l syntax.tab.h
	flex lexical.l

clean:
	rm -f *.tab.c *.yy.c *.tab.h parser *.output
