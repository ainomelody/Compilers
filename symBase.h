#ifndef _SYMBASE_H

#define _SYMBASE_H

#include "tree.h"

#define LISTSIZE 4
#define LISTINCSIZE 4
#define MAXSTRLEN 40

/*Linear table stores information of array dimension*/
typedef struct{
    int *data;
    int size;
    int pos;
}arrayInfo;

/*Struct stores information of a variable*/
typedef struct{
    int type;   //0: int, 1: float, more than 1: a pointer to structDefInfo
    int isArray;
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
    
    int flag;   //1: only definition, 2: only declaration, 3: both definition & declaration
    int retType;    // type of return value, same as type in varInfo (symBase.h)
    varList *param;
}funcInfo;

/*Infomation of struct type definition is arranged in another tree rather than symbol table */
typedef struct structDefInfo{
    char name[MAXSTRLEN];
    varList *region;
    struct structDefInfo *left, *right;
}structDefInfo;

typedef struct symNode{
    char *name;
    int lineNum;
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

arrayInfo *newArrayInfo();
void addArrayDim(arrayInfo *info, int ubound);
void freeArrayInfo(arrayInfo **info);
varList *newVarList();
void addVariable(varList *list, varInfo *var);
void freeVarList(varList **list);
void initScopeStack();
void getInScope();
void getOutScope();

#endif