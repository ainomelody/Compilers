#include "midCode.h"

static codeCollection allCodes;
static funcCode *curFunc;

void initCodeCollection()
{
    allCodes->data = malloc(sizeof(funcCode *) * LISTSIZE);
    allCodes->size = LISTSIZE;
    allCodes->pos = 0;
}

void addFunction(funcInfo *func)
{
    if (allCodes->size == allCodes->pos) {
        allCodes->size += LISTINCSIZE;
        allCodes->data = relloc(allCodes->data, sizeof(funcCode *) * allCodes->size);
    }
    curFunc = malloc(sizeof(funcCode));
    allCodes->data[allCodes->pos++] = curFunc;
    curFunc->space = 0;
    curFunc->code = NULL;
    curFunc->info = func;
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