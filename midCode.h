#ifndef _MIDCODE_H

#define _MIDCODE_H

#include "symBase.h"

/* meaning of op:
1. Define lable, target assigns the number of label.Labels are all named after labelx.
2. Assignment operation. target := arg1
3. Add operation. target := arg1 + arg2
4. Minus operation. target := arg1 - arg2
5. Times operation. target := arg1 * arg2
6. Division. target := arg1 / arg2
7. Get address. target := &arg1
8. target := *arg1
9. *target := arg1
10. goto label-target
11. if arg1 goto label target
12. return target
13. arg target
14. target := call arg1
15. READ target
16. WRITE target
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
    int space;  //space the local variable needs
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

extern int labelNum;

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
int processOffset(valueSt *base, valueSt *offset);

#endif