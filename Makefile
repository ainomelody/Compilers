all: tree.c lex.yy.c exp.tab.c
	gcc tree.c lex.yy.c exp.tab.c -lfl -ly -o test/parser
lex.yy.c : exp.tab.h lex.l
	flex lex.l
exp.tab.h exp.tab.c : exp.y
	bison -d exp.y