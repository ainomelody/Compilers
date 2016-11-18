%error-verbose
%locations
%{
#define YYSTYPE YYVALTYPE
#include "tree.h"
extern void yyerror(const char *msg);
int yylex();
%}

%token INT FLOAT ID SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left DOT LP RP LB RB

%%
Program : ExtDefList {$$ = newNode("Program", $1->lineNum); addChild($$, 1, $1); printTree($$, 1); freeTree($$);}
;

ExtDefList : {$$ = newNode("ExtDefList", -1);}
| ExtDef ExtDefList {$$ = newNode("ExtDefList", $1->lineNum); addChild($$, 2, $1, $2);}
;

ExtDef : Specifier ExtDecList SEMI {$$ = newNode("ExtDef", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Specifier SEMI {$$ = newNode("ExtDef", $1->lineNum); addChild($$, 2, $1, $2);}
| Specifier FunDec CompSt {$$ = newNode("ExtDef", $1->lineNum); addChild($$, 3, $1, $2, $3);}
;

ExtDecList : VarDec {$$ = newNode("ExtDecList", $1->lineNum); addChild($$, 1, $1);}
| VarDec COMMA ExtDecList {$$ = newNode("ExtDecList", $1->lineNum); addChild($$, 3, $1, $2, $3);}
;

Specifier : TYPE {$$ = newNode("Specifier", $1->lineNum); addChild($$, 1, $1);}
| StructSpecifier {$$ = newNode("Specifier", $1->lineNum); addChild($$, 1, $1);}
;

StructSpecifier : STRUCT OptTag LC DefList RC {$$ = newNode("StructSpecifier", $1->lineNum); addChild($$, 5, $1, $2, $3, $4, $5);}
| STRUCT Tag {$$ = newNode("StructSpecifier", $1->lineNum); addChild($$, 2, $1, $2);}
;

OptTag : {$$ = newNode("OptTag", -1);}
| ID {$$ = newNode("OptTag", $1->lineNum); addChild($$, 1, $1);}
;

Tag : ID {$$ = newNode("Tag", $1->lineNum); addChild($$, 1, $1);}
;

VarDec : ID {$$ = newNode("VarDec", $1->lineNum); addChild($$, 1, $1);}
| VarDec LB INT RB {$$ = newNode("VarDec", $1->lineNum); addChild($$, 4, $1, $2, $3, $4);}
;

FunDec : ID LP VarList RP {$$ = newNode("FunDec", $1->lineNum); addChild($$, 4, $1, $2, $3, $4);}
| ID LP RP {$$ = newNode("FunDec", $1->lineNum); addChild($$, 3, $1, $2, $3);}
;

VarList : ParamDec COMMA VarList {$$ = newNode("VarList", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| ParamDec  {$$ = newNode("VarList", $1->lineNum); addChild($$, 1, $1);}
;

ParamDec : Specifier VarDec {$$ = newNode("ParamDec", $1->lineNum); addChild($$, 2, $1, $2);}
;

CompSt : LC DefList StmtList RC {$$ = newNode("CompSt", $1->lineNum); addChild($$, 4, $1, $2, $3, $4);}
;

StmtList : {$$ = newNode("StmtList", -1);}
| Stmt StmtList {$$ = newNode("StmtList", $1->lineNum); addChild($$, 2, $1, $2);}
;

Stmt : Exp SEMI {$$ = newNode("Stmt", $1->lineNum); addChild($$, 2, $1, $2);}
| CompSt {$$ = newNode("Stmt", $1->lineNum); addChild($$, 1, $1);}
| RETURN Exp SEMI {$$ = newNode("Stmt", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| IF LP Exp RP Stmt {$$ = newNode("Stmt", $1->lineNum); addChild($$, 5, $1, $2, $3, $4, $5);}
| IF LP Exp RP Stmt ELSE Stmt {$$ = newNode("Stmt", $1->lineNum); addChild($$, 7, $1, $2, $3, $4, $5, $6, $7);}
| WHILE LP Exp RP Stmt {$$ = newNode("Stmt", $1->lineNum); addChild($$, 5, $1, $2, $3, $4, $5);}
;

DefList : {$$ = newNode("DefList", -1);}
| Def DefList {$$ = newNode("DefList", $1->lineNum); addChild($$, 2, $1, $2);}
;

Def : Specifier DecList SEMI {$$ = newNode("Def", $1->lineNum); addChild($$, 3, $1, $2, $3);}
;

DecList : Dec {$$ = newNode("DecList", $1->lineNum); addChild($$, 1, $1);}
| Dec COMMA DecList {$$ = newNode("DecList", $1->lineNum); addChild($$, 3, $1, $2, $3);}
;

Dec : VarDec {$$ = newNode("Dec", $1->lineNum); addChild($$, 1, $1);}
| VarDec ASSIGNOP Exp {$$ = newNode("Dec", $1->lineNum); addChild($$, 3, $1, $2, $3);}
;

Exp : Exp ASSIGNOP Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp AND Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp OR Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp RELOP Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp PLUS Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp MINUS Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp STAR Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp DIV Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| LP Exp RP {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| MINUS Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 2, $1, $2);}
| NOT Exp {$$ = newNode("Exp", $1->lineNum); addChild($$, 2, $1, $2);}
| ID LP Args RP {$$ = newNode("Exp", $1->lineNum); addChild($$, 4, $1, $2, $3, $4);}
| ID LP RP {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp LB Exp RB {$$ = newNode("Exp", $1->lineNum); addChild($$, 4, $1, $2, $3, $4);}
| Exp DOT ID {$$ = newNode("Exp", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| ID {$$ = newNode("Exp", $1->lineNum); addChild($$, 1, $1);}
| INT {$$ = newNode("Exp", $1->lineNum); addChild($$, 1, $1);}
| FLOAT {$$ = newNode("Exp", $1->lineNum); addChild($$, 1, $1);}
;

Args : Exp COMMA Args {$$ = newNode("Args", $1->lineNum); addChild($$, 3, $1, $2, $3);}
| Exp {$$ = newNode("Args", $1->lineNum); addChild($$, 1, $1);}
;

%%
