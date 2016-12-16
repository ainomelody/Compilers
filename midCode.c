#include "midCode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TEMPVARNUM 10

static codeCollection allCodes;
static funcCode *curFunc;
int labelNum;
static int tempVarIndex;
static int tempVarUsed[TEMPVARNUM];
static int isTempVar(valueSt *st);
static expTransInfo execOpr(int op, expTransInfo *arg1, expTransInfo *arg2);
static int mergeOp(int target, int op, valueSt *st);

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

void addCode(int op, int target, valueSt *arg1, valueSt *arg2)
{
    tripleCode *newCode = malloc(sizeof(tripleCode));

    newCode->op = op;
    newCode->target = target;
    newCode->arg1 = *arg1;
    newCode->arg2 = *arg2;

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

tripleCode *getLastCode()
{
    return curFunc->code->prev;
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
    expTransInfo exp1, exp2, ret = {{1, 0}, {1, 0}, 0, 0, NULL};
    int childNum = node->childNum;
    symNode *searchedSym;
    varInfo *searchedVar;

    node = node->child;
    switch(childNum) {
        case 1:
            if (!strcmp(node->type, "ID")) {
                ret.base.isImm = 0;
                searchedSym = searchSymbol(node->data.id, 0, NULL);
                searchedVar = (varInfo *)searchedSym->info;
                ret.base.value = (int)searchedVar;
                ret.hasOffset = searchedVar->isArray;
                ret.toTimes = copyArrInfo(searchedVar->arrInfo);
                if (ret.hasOffset)
                    ret.toTimes = removeOneDim(ret.toTimes);
                ret.type = searchedVar->type;
                return ret;
            } else {    //int
                ret.base.isImm = 1;
                ret.base.value = node->data.intValue;
                return ret;
            }
            break;
        case 2:

            break;
        case 3:
            break;
        case 4:
            break;
    }
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

void printCodes()
{

}

int processOffset(valueSt *base, valueSt *offset)
{
    int temp;
    valueSt st1, st2;

    temp = getTempVar();
    st1.isImm = 0;
    st1.value = temp;
    addCode(7, temp, base, base); //get the address of variable pointed by base
    addCode(3, temp, &st1, offset);
    
    if (!offset->isImm)
        releaseTempVar(offset->value);
    return temp;
}

//op: 3+ 4- 5* 6/
expTransInfo execOpr(int op, expTransInfo *arg1, expTransInfo *arg2)
{
    valueSt st1, st2;
    expTransInfo ret = {{1, 0}, {1, 0}, 0, 0, NULL};

    if (arg1->base.isImm && arg2->base.isImm) {
        switch(op) {
            case 3:
                ret.base.value = arg1->base.value + arg2->base.value;
                break;
            case 4:
                ret.base.value = arg1->base.value - arg2->base.value;
                break;
            case 5:
                ret.base.value = arg1->base.value * arg2->base.value;
                break;
            case 6:
                ret.base.value = arg1->base.value / arg2->base.value;
        }
        return ret;
    } else {
        if (isTempVar(&arg1->base)) {
            if (mergeOp(arg1->base.value, op, &arg2->base)) {   //arg2 is imm & can merge
                ret.base = arg1->base;
                return ret;
            }
            if (arg2->hasOffset) {   //access array or structure
                st1.isImm = 0;
                st1.value = processOffset(&arg2->base, &arg2->offset);
                addCode(8, st1.value, &st1, &st1);
            } else
                st1 = arg2->base;
            addCode(op, arg1->base.value, &arg1->base, &st1);
            if (isTempVar(&arg2->base))
                releaseTempVar(arg2->base.value);
            ret.base = arg1->base;
            return ret;
        }
        else if (isTempVar(&arg2->base)) {
            if (mergeOp(arg2->base.value, op, &arg1->base)) {
                ret.base = arg2->base;
                return ret;
            }
            if (arg1->hasOffset) {
                st1.isImm = 0;
                st1.value = processOffset(&arg1->base, &arg1->offset);
                addCode(8, st1.value, &st1, &st1);
            } else
                st1 = arg2->base;
            addCode(op, arg2->base.value, &arg2->base, &st1);
            ret.base = arg2->base;
            return ret;
        }
        else {
            int tar[2];
            int i = 0;
            if (arg1->hasOffset) {
                st1.isImm = 0;
                st1.value = tar[i++] = processOffset(&arg1->base, &arg1->offset);
                addCode(8, st1.value, &st1, &st1);
                st2 = arg2->base;
            }
            if (arg2->hasOffset) {
                st1.isImm = 0;
                st1.value = tar[i++] = processOffset(&arg1->base, &arg1->offset);
                addCode(8, st1.value, &st1, &st1);
                st2 = arg1->base;
            }
            
            if (!i) {
                tar[0] = getTempVar();
                addCode(op, tar[0], &arg1->base, &arg2->base);
            } else if (i == 1) {
                addCode(op, tar[0], &st1, &st2);
            } else {
                st1.isImm = st2.isImm = 0;
                st1.value = tar[0];
                st2.value = tar[1];
                addCode(op, tar[0], &st1, &st2);
                releaseTempVar(tar[1]);
            }

            ret.base.isImm = 0;
            ret.base.value = tar[0];
            return ret;
        }
    }
}

static int isTempVar(valueSt *st)
{
    return !st->isImm && st->value < TEMPVARNUM;
}

static int mergeOp(int target, int op, valueSt *st)
{
    tripleCode *last = getLastCode();
    valueSt *mergeTar;

    if (target != last->target)
        return 0;
    if (!st->isImm)
        return 0;
    if (last->op != op && last->op + op != 7)
        return 0;
    if (!last->arg1.isImm && !last->arg2.isImm)
        return 0;

    if (last->arg1.isImm)
        mergeTar = &last->arg1;
    else if (last->arg2.isImm)
        mergeTar = &last->arg2;
    else
        return 0;

    switch(last->op) {
        case 5:
            if (op == 5)
                mergeTar->value += st->value;
            else
                mergeTar->value -= st->value;
            break;
        case 6:
            if (op == 5)
                mergeTar->value -= st->value;
            else
                mergeTar->value += st->value;
            break;
        case 7:
        case 8:
            mergeTar->value *= st->value;
            break;
    }
    return 1;
}