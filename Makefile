all: tree.c lex.yy.c exp.tab.c symBase.c symTable.c
	gcc tree.c lex.yy.c exp.tab.c symBase.c symTable.c -gstabs -lfl -ly -o parser
lex.yy.c : exp.tab.h lex.l
	flex lex.l
exp.tab.h exp.tab.c : exp.y
	bison -v -d exp.y