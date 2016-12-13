#ifndef _SYMBASE_H

#define _SYMBASE_H

#include "tree.h"

#define LISTSIZE 4
#define LISTINCSIZE 4
#define MAXSTRLEN 40

extern int structIndex;

/*Linear table stores information of array dimension*/
typedef struct{
    int *data;
    int size;
    int pos;
}arrayInfo;

/*Struct stores information of a variable*/
typedef struct{
    int type;   //0: int, 1: float, more than 1: a pointer to structDefInfo, -1: undefined structure, -2: undefined symbol or other errors
    int offset;
    int isArray;
    char *name;
    Node *initExp;
    int lineNum;
    arrayInfo *arrInfo;
}varInfo;

/*Linear table stores information of variables as regions of a struct
  or  parameters of a function*/
typedef struct{
    varInfo **data;
    int size;
    int pos;
}varList;

typedef struct{
/*notes: Function declarations & definitions are only admitted to appear in global scope.*/
    char *name;
    int flag;   //1: only declaration, 2: only definition, 3: both definition & declaration
    int retType;    // type of return value, same as type in varInfo (symBase.h)
    int lineNum;    //records the smaller one
    varList *param;
}funcInfo;

/*Infomation of struct type definition is arranged in another tree rather than symbol table */
typedef struct structDefInfo{
    char name[MAXSTRLEN];
    varList *region;
    int size;
    struct structDefInfo *left, *right;
}structDefInfo;

typedef struct symNode{
    char *name;
    int type;   //0: function, 1: variable
    void *info; //varInfo * or funcInfo *
    struct symNode *left, *right;
}symNode;

typedef struct{
    symNode **symData;
    structDefInfo **stData;
    int size;
    int pos;    
}symTableStack;

extern symNode **globalSymTable;

arrayInfo *newArrayInfo();
void addArrayDim(arrayInfo *info, int ubound);
void freeArrayInfo(arrayInfo **info);
varList *newVarList();
int addVariable(varList *list, varInfo *var);       //If found variable has the same name, return -1
void freeVarList(varList **list);
void initScopeStack();          //init scope stack, add a symbol table storing global symbols.
void getInScope();
void getOutScope();
void freeSymTable(symNode *table);
void freeStInfo(structDefInfo *info);
structDefInfo *searchStruct(char *name, int onlyCurScope, structDefInfo **parent);
symNode *searchSymbol(char *name, int onlyCurScope, symNode **parent);
void addStructInfo(structDefInfo *info, structDefInfo *parent);     //insert a struct info into tree
void addSymbol(symNode *sym, symNode *parent);
void freeVarInfo(varInfo *info);
int matchVarList(varList *list1, varList *list2);   //check if two list is same in type
int matchArrInfo(arrayInfo *info1, arrayInfo *info2);
int getDimNum(varInfo *var);
varInfo *searchRegion(structDefInfo *st, char *name);
arrayInfo *copyArrInfo(arrayInfo *toCopy);
arrayInfo *removeOneDim(arrayInfo *info);
int checkTypeConsist(int type1, int type2);
int sizeOfVar(varInfo *var);
int sizeOfType(int type);

#endif