%option yylineno
%option input
%{
#define YYSTYPE YYVALTYPE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "exp.tab.h"

int yycolumn = 1;
#define YY_USER_ACTION 	{\
	yylloc.first_line = yylloc.last_line = yylineno; \
	yylloc.first_column = yycolumn; \
	yylloc.last_column = yycolumn + yyleng - 1; \
	yycolumn += yyleng; \
}

%}
OCTAL 0[0-7]+
DECIMAL ([1-9][0-9]*)|0
HEX 0x[0-9A-Fa-f]+
INT {OCTAL}|{DECIMAL}|{HEX}
NUMFLOAT ([0-9]+\.[0-9]*)|(\.[0-9]+)
INDEXFLOAT (([0-9]+(\.)?[0-9]*)|(\.[0-9]+))(e|E)(\+|\-)?[0-9]+
FLOAT {NUMFLOAT}|{INDEXFLOAT}

character [a-zA-Z_]
blank ([\t\r\f\x20\x0B])*
SEMI ;
COMMA ,
ASSIGNOP =
RELOP >|<|(>=)|(<=)|(==)|(!=)
PLUS \+
MINUS \-
STAR \*
DIV \/
AND &&
OR \|\|
DOT \.
NOT !
TYPE (int)|(float)
LP \(
RP \)
LB \[
RB \]
LC \{
RC \}
STRUCT struct
RETURN return
IF if
ELSE else
WHILE while
ID {character}[A-Za-z_0-9]*
%%

\n {yycolumn = 1; }
{INT} {yylval = newNode("INT", yylineno); yylval->data.intValue = strtol(yytext, NULL, 0); return INT;}
{FLOAT} {yylval = newNode("FLOAT", yylineno); yylval->data.floatValue = atof(yytext); return FLOAT;}
(\-|\+)?[0-9][0-9a-zA-Z\.]* {printf("Error type A at Line %d: Invalid number!\n", yylineno); exit(0);}
{blank} {}
{SEMI} {yylval = newNode("SEMI", yylineno); return SEMI;}
{COMMA} {yylval = newNode("COMMA", yylineno); return COMMA;}
{ASSIGNOP} {yylval = newNode("ASSIGNOP", yylineno); return ASSIGNOP;}
{RELOP} {yylval = newNode("RELOP", yylineno); return RELOP;}
{PLUS} {yylval = newNode("PLUS", yylineno); return PLUS;}
{MINUS} {yylval = newNode("MINUS", yylineno); return MINUS;}
{STAR} {yylval = newNode("STAR", yylineno); return STAR;}
{DIV} {yylval = newNode("DIV", yylineno); return DIV;}
{AND} {yylval = newNode("AND", yylineno); return AND;}
{OR} {yylval = newNode("OR", yylineno); return OR;}
{DOT} {yylval = newNode("DOT", yylineno); return DOT;}
{NOT} {yylval = newNode("NOT", yylineno); return NOT;}
{TYPE} {char *ptr = malloc(8); yylval = newNode("TYPE", yylineno); strcpy(yylval->data.type = ptr, yytext); return TYPE;}
{LP} {yylval = newNode("LP", yylineno); return LP;}
{RP} {yylval = newNode("RP", yylineno); return RP;}
{LB} {yylval = newNode("LB", yylineno); return LB;}
{RB} {yylval = newNode("RB", yylineno); return RB;}
{LC} {yylval = newNode("LC", yylineno); return LC;}
{RC} {yylval = newNode("RC", yylineno); return RC;}
{STRUCT} {yylval = newNode("STRUCT", yylineno); return STRUCT;}
{RETURN} {yylval = newNode("RETURN", yylineno); return RETURN;}
{IF} {yylval = newNode("IF", yylineno); return IF;}
{ELSE} {yylval = newNode("ELSE", yylineno); return ELSE;}
{WHILE} {yylval = newNode("WHILE", yylineno); return WHILE;}
{ID} {char *ptr = malloc(36); yylval = newNode("ID", yylineno); strncpy(yylval->data.id = ptr, yytext, 31); ptr[31] = '\0'; return ID;}
\/\*(.|\n)*\*\/ {char *loc = strrchr(yytext, '\n'); if (loc) yycolumn = strlen(loc);}
\/\/(.)*\n {yycolumn = 1;}
. {printf("Error type A at line %d: Mystirious charachter '%s'\n", yylineno, yytext); exit(0);}
%%

int main(int argc, char *argv[])
{
	if (argc > 1)
		if (!(yyin = fopen(argv[1], "r"))){
			perror(argv[1]);
			return 1;
		}
	yyparse();
	return 0;
}

void yyerror(const char *msg)
{
	printf("Error type B at Line %d: %s\n", msg, yylineno);
}
