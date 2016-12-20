#ifndef _MIDCODE_H

#define _MIDCODE_H

#include "symBase.h"

/* meaning of op:
1. Define lable, arg1 assigns the number of label.Labels are all named after labelx.
2. Assignment operation. target := arg1
3. Add operation. target := arg1 + arg2
4. Minus operation. target := arg1 - arg2
5. Times operation. target := arg1 * arg2
6. Division. target := arg1 / arg2
7. Get address. target := &arg1
8. target := *arg1
9. *target := arg1
10. goto label-arg1
11. if arg1 relop(op - 11) arg2 goto target
12. return arg1    target = 10000
13. arg arg1       target = 10000
14. target := call arg1
15. READ target
16. WRITE arg1    target = 10000
*/

typedef struct{
    int isImm;  //is immediate number or not
    int value;  //immediate number or points to a variable
}valueSt;

typedef struct tripleCode{
    int target;
    int op;
    valueSt arg1, arg2;
    struct tripleCode *prev, *next;
}tripleCode;

typedef struct {
    tripleCode *code;
    funcInfo *info;
    varList *toAlloc;   //array or structure needs space allocation.
    varList *paramList; //points to the params in symbol table
    int space;  //space the local variable needs
    int paramSpace;
}funcCode;

typedef struct{
    funcCode **data;
    int size;
    int pos;
}codeCollection;

typedef struct{
    valueSt base, offset;
    int hasOffset;
    int type;
    arrayInfo *toTimes;
}expTransInfo;

extern codeCollection allCodes;

void initCodeCollection();
void addFunction(funcInfo *func);
void addLocalVar(varInfo *var);
void addCode(int op, int target, valueSt *arg1, valueSt *arg2);
tripleCode *getLastCode();
int getTempVar();
void releaseTempVar(int index);
expTransInfo translateExp(Node *node);
void addIOFunc();
void printCodes();
int processOffset(expTransInfo *info);
void addParam();
int getLabelNum();
int translateRelop(char *relop);
int isTempVar(valueSt *st);
int hasEffect(Node *node);

#endif