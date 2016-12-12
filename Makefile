all: tree.c lex.yy.c exp.tab.c symBase.c symTable.c
	gcc tree.c lex.yy.c exp.tab.c symBase.c symTable.c -g -lfl -ly -o test/2/parser
lex.yy.c : exp.tab.h lex.l
	flex lex.l
exp.tab.h exp.tab.c : exp.y
	bison -d exp.y