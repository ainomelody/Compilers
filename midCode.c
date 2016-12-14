#include "midCode.h"
#include <stdlib.h>

#define TEMPVARNUM 10

static codeCollection allCodes;
static funcCode *curFunc;
int labelNum;
static int tempVarIndex;
static int tempVarUsed[TEMPVARNUM];

void initCodeCollection()
{
    allCodes.data = malloc(sizeof(funcCode *) * LISTSIZE);
    allCodes.size = LISTSIZE;
    allCodes.pos = 0;
}

void addFunction(funcInfo *func)
{
    if (allCodes.size == allCodes.pos) {
        allCodes.size += LISTINCSIZE;
        allCodes.data = realloc(allCodes.data, sizeof(funcCode *) * allCodes.size);
    }
    curFunc = malloc(sizeof(funcCode));
    allCodes.data[allCodes.pos++] = curFunc;
    curFunc->space = 0;
    curFunc->code = NULL;
    curFunc->info = func;
    curFunc->toAlloc = newVarList();
}

void addLocalVar(varInfo *var)
{
    var->offset = curFunc->space;
    curFunc->space += sizeOfVar(var);
}

void addCode(int op, int target, int arg1, int arg2)
{
    tripleCode *newCode = malloc(sizeof(tripleCode));

    newCode->op = op;
    newCode->target = target;
    newCode->arg1 = arg1;
    newCode->arg2 = arg2;

    if (curFunc->code == NULL) {
        curFunc->code = newCode;
        newCode->prev = newCode->next = newCode;
    } else {
        tripleCode *tail = curFunc->code->prev;
        newCode->prev = tail;
        tail->next = newCode;
        newCode->next = curFunc->code;
        curFunc->code->prev = newCode;
    }
}

void modifyPrevTarget(int target)
{
    curFunc->code->prev->target = target;
}

int getTempVar()
{
    while (tempVarUsed[tempVarIndex])
        tempVarIndex = (tempVarIndex + 1) % TEMPVARNUM;
    tempVarUsed[tempVarIndex] = 1;
    return tempVarIndex;
}

void releaseTempVar(int index)
{
    tempVarUsed[index] = 0;
}

expTransInfo translateExp(Node *node)
{

}

void addIOFunc()
{
    funcInfo *readInfo, *writeInfo;
    symNode *symRead, *symWrite, *insLoc;
    varInfo *param;

    readInfo = malloc(sizeof(funcInfo));
    writeInfo = malloc(sizeof(funcInfo));
    symRead = malloc(sizeof(symNode));
    symWrite = malloc(sizeof(symNode));

    readInfo->name = symRead->name = "read";
    readInfo->flag = 2;
    readInfo->retType = 0;
    readInfo->param = NULL;
    symRead->info = readInfo;
    symRead->type = 0;
    symRead->left = symRead->right = NULL;
    searchSymbol(symRead->name, 1, &insLoc);
    addSymbol(symRead, insLoc);

    writeInfo->name = symWrite->name = "write";
    writeInfo->flag = 2;
    writeInfo->retType = 0;
    writeInfo->param = newVarList();
    symWrite->info = writeInfo;
    symWrite->type = 0;
    symWrite->left = symWrite->right = NULL;
    param = malloc(sizeof(varInfo));
    param->type = param->isArray = 0;
    param->arrInfo = NULL;
    param->name = "num";
    addVariable(writeInfo->param, param);

    searchSymbol(symWrite->name, 1, &insLoc);
    addSymbol(symWrite, insLoc);
}