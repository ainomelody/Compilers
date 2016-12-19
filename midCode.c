#include "midCode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TEMPVARNUM 10

static codeCollection allCodes;
static funcCode *curFunc;
static int labelNum;
static int tempVarIndex;
static int tempVarUsed[TEMPVARNUM];

static expTransInfo execOp(int op, expTransInfo *arg1, expTransInfo *arg2);
static int mergeOp(int target, int op, valueSt *st);
static void printValueSt(valueSt *st);
static void printRelop(int relop);
static int isParam(varList *paramList, varInfo *param);

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
    curFunc->code = malloc(sizeof(tripleCode)); //an empty head node
    curFunc->code->prev = curFunc->code->next = curFunc->code;
    curFunc->info = func;
    curFunc->toAlloc = newVarList();
    curFunc->paramList = newVarList();
}

void addLocalVar(varInfo *var)
{
    char *temp;
    expTransInfo init;

    var->offset = curFunc->space;
    curFunc->space += sizeOfVar(var);
    if (var->type > 10 || var->isArray) {
        temp = var->name;
        var->name = NULL;
        addVariable(curFunc->toAlloc, var);
        var->name = temp;
    } else if (var->initExp != NULL) {
        init = translateExp(var->initExp);
        if (init.hasOffset) {
            valueSt addr;
            addr.isImm = 0;
            addr.value = processOffset(&init);
            addCode(8, (int)var, &addr, NULL);
        } else
            addCode(2, (int)var, &init.base, NULL);
    }
}

void addCode(int op, int target, valueSt *arg1, valueSt *arg2)
{
    tripleCode *newCode = malloc(sizeof(tripleCode));
    tripleCode *tail = curFunc->code->prev;

    newCode->op = op;
    newCode->target = target;
    if (arg1 != NULL)
        newCode->arg1 = *arg1;
    if (arg2 != NULL)
        newCode->arg2 = *arg2;

    newCode->prev = tail;
    tail->next = newCode;
    newCode->next = curFunc->code;
    curFunc->code->prev = newCode;
}

tripleCode *getLastCode()
{
    if (curFunc->code->prev == curFunc->code)
        return NULL;
    else
        return curFunc->code->prev;
}

int getTempVar()
{
    do
        tempVarIndex = (tempVarIndex + 1) % TEMPVARNUM;
    while (tempVarUsed[tempVarIndex]);
    tempVarUsed[tempVarIndex] = 1;
    return tempVarIndex;
}

void releaseTempVar(int index)
{
    if (index < TEMPVARNUM)
        tempVarUsed[index] = 0;
}

expTransInfo translateExp(Node *node)
{
    expTransInfo exp1, exp2, ret = {{1, 0}, {1, 0}, 0, 0, NULL};
    int childNum = node->childNum;
    symNode *searchedSym;
    varInfo *searchedVar;
    funcInfo *searchedFunc;

    node = node->child;
    switch(childNum) {
        case 1:
            if (!strcmp(node->type, "ID")) {
                ret.base.isImm = 0;
                searchedSym = searchSymbol(node->data.id, 0, NULL);
                searchedVar = (varInfo *)searchedSym->info;
                if (isParam(curFunc->paramList, searchedVar) && (searchedVar->type > 10 || searchedVar->isArray)) {
                    ret.hasOffset = 1;
                    ret.offset.isImm = 0;
                    ret.offset.value = (int)searchedVar;
                    ret.base.isImm = 1;
                } else {
                    ret.base.value = (int)searchedVar;
                    ret.hasOffset = 0;
                }
                ret.toTimes = copyArrInfo(searchedVar->arrInfo);
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
            exp1 = translateExp(node->sibling);
            exp2.hasOffset = 0;
            exp2.base.isImm = 1;
            exp2.base.value = 0;
            return execOp(4, &exp2, &exp1);
            break;
        case 3:
            if (!strcmp(node->type, "LP"))
                return translateExp(node->sibling);
            else if (!strcmp(node->type, "ID")) {
                int target = getTempVar();
                valueSt funcSt;

                if (!strcmp(node->data.id, "read")) {
                    addCode(15, target, &funcSt, NULL);
                    ret.base.isImm = 0;
                    ret.base.value = target;
                    return ret;
                }
                searchedSym = searchSymbol(node->data.id, 0, NULL);
                funcSt.value = (int)(searchedSym->info);
                addCode(14, target, &funcSt, NULL);
                ret.base.isImm = 0;
                ret.base.value = target;
                return ret;
            } else if (!strcmp(node->sibling->type, "DOT")) {
                exp1 = translateExp(node);
                searchedVar = searchRegion((structDefInfo *)exp1.type, node->sibling->sibling->data.id);
                ret.type = searchedVar->type;
                ret.hasOffset = 1;
                ret.toTimes = copyArrInfo(searchedVar->arrInfo);
                ret.toTimes = removeOneDim(ret.toTimes);
                if (exp1.base.isImm == 1) {
                    expTransInfo offsetPlus, offsetRet;

                    offsetPlus.base.isImm = 1;
                    offsetPlus.base.value = searchedVar->offset;
                    offsetPlus.hasOffset = 0;
                    exp1.base = exp1.offset;
                    exp1.hasOffset = 0;
                    offsetRet = execOp(3, &exp1, &offsetPlus);
                    ret.offset = offsetRet.base;
                    ret.base.isImm = 1;
                } else {
                    ret.base = exp1.base;
                    if (!exp1.hasOffset || exp1.offset.isImm) {
                        exp1.hasOffset = 1;
                        ret.offset.value = exp1.offset.value + searchedVar->offset;
                    } else {
                        valueSt offsetSt;
                        
                        offsetSt.isImm = 1;
                        offsetSt.value = searchedVar->offset;
                        addCode(3, exp1.offset.value, &exp1.offset, &offsetSt);
                        ret.offset.isImm = 0;
                        ret.offset.value = exp1.offset.value;
                    }
                }
                return ret;
            } else if (!strcmp(node->sibling->type, "ASSIGNOP")) {
                tripleCode *last;
                int assignTar;
                int assignOp = 2;

                exp1 = translateExp(node);
                if (exp1.hasOffset) {
                    assignTar = processOffset(&exp1);
                    assignOp = 9;
                    releaseTempVar(assignTar);
                    ret.base.isImm = 2;
                    ret.hasOffset = 1;
                }
                else {
                    ret.hasOffset = 0;
                    ret.base.isImm = 0;
                    assignTar = exp1.base.value;
                }

                exp2 = translateExp(node->sibling->sibling);
                last = getLastCode();
                if (isTempVar(&exp2.base) && assignOp != 9)     //replace the temp variable with variable
                    last->target = assignTar;
                else {
                    if (exp2.hasOffset) {
                        int assignSource = processOffset(&exp2);

                        if (assignOp == 2)
                            assignOp = 8;
                        else {
                            int middleTemp = getTempVar();
                            valueSt st;

                            st.isImm = 0;
                            st.value = assignSource;
                            addCode(8, middleTemp, &st, NULL);
                            exp2.base.value = middleTemp;
                            releaseTempVar(middleTemp);
                        }
                        releaseTempVar(assignSource);
                    }
                    addCode(assignOp, assignTar, &exp2.base, NULL);
                }
                ret.base.value = assignTar;
                return ret;
            } else {
                int op;

                if (!strcmp(node->sibling->type, "PLUS"))
                    op = 3;
                else if (!strcmp(node->sibling->type, "MINUS"))
                    op = 4;
                else if (!strcmp(node->sibling->type, "STAR"))
                    op = 5;
                else
                    op = 6;
                exp1 = translateExp(node);
                exp2 = translateExp(node->sibling->sibling);
                return execOp(op, &exp1, &exp2);
            }
            break;
        case 4:
            if (!strcmp(node->type, "ID")) {    //function call
                Node *arg = node->sibling->sibling->child;
                Node *argStack[40];
                int argNum = 0;
                int tarTemp;
                valueSt targetSt;

                while (arg) {
                    argStack[argNum++] = arg;
                    if (arg->sibling != NULL)
                        arg = arg->sibling;
                    else
                        arg = NULL;
                }

                if (!strcmp(node->data.id, "write")) {
                    exp1 = translateExp(argStack[0]);
                    if (exp1.hasOffset) {
                        targetSt.value = processOffset(&exp1);
                        targetSt.isImm = 2;
                        releaseTempVar(targetSt.value);
                    }
                    else
                        targetSt = exp1.base;
                    addCode(16, 10000, &targetSt, NULL);
                    return ret;
                }

                while (argNum--) {
                    exp1 = translateExp(argStack[argNum]);
                    if (exp1.hasOffset) {
                        targetSt.value = processOffset(&exp1);
                        targetSt.isImm = 2;
                        releaseTempVar(targetSt.value);
                    } else if (!exp1.base.isImm && exp1.base.value > 1000){
                        varInfo *var = (varInfo *)exp1.base.value;

                        if (var->isArray || var->type > 10)
                            targetSt.isImm = 3;
                        else 
                            targetSt.isImm = 0;
                        targetSt.value = exp1.base.value;
                    } else
                        targetSt = exp1.base;
                    addCode(13, 10000, &targetSt, NULL);   //ARG
                    if (isTempVar(&targetSt))
                        releaseTempVar(targetSt.value);
                }
                searchedSym = searchSymbol(node->data.id, 0, NULL);
                targetSt.isImm = 0;
                searchedFunc = (funcInfo *)searchedSym->info;
                targetSt.value = (int)searchedFunc;
                tarTemp = getTempVar();
                addCode(14, tarTemp, &targetSt, NULL);
                ret.hasOffset = 0;
                ret.type = searchedFunc->retType;
                ret.base.isImm = 0;
                ret.base.value = tarTemp;
                return ret;
            } else {    //array
                expTransInfo operand;
                int i, arrSize;

                exp1 = translateExp(node);
                exp2 = translateExp(node->sibling->sibling);
                arrSize = sizeOfType(exp1.type);
                ret.hasOffset = 1;
                ret.type = exp1.type;
                if (exp1.toTimes != NULL)   //times all the dimensions
                    for (i = 0; i < exp1.toTimes->pos; i++)
                        arrSize *= exp1.toTimes->data[i];
                ret.toTimes = removeOneDim(exp1.toTimes);
                operand.base.isImm = 1;
                operand.base.value = arrSize;
                operand.hasOffset = 0;
                if (exp2.base.isImm == 1 && !exp2.base.value)
                    operand.base.value = 0;
                else
                    operand = execOp(5, &exp2, &operand);   //times size, store in base

                if (exp1.base.isImm) {
                    exp1.base = exp1.offset;
                    exp1.hasOffset = 0;
                    ret.base.isImm = 1;

                    if (operand.base.isImm == 1 && !operand.base.value)
                        ret.offset = exp1.offset;
                    else if (exp1.offset.isImm == 1 && !exp1.offset.value)
                        ret.offset = operand.base;
                    else {
                        operand = execOp(3, &operand, &exp1);
                        ret.offset = operand.base;
                    }
                } else {
                    exp2.base = exp1.offset;
                    exp2.hasOffset = 0;
                    ret.base = exp1.base;

                    if (operand.base.isImm == 1 && !operand.base.value)
                        ret.offset = exp1.offset;
                    else if (exp1.offset.isImm == 1 && !exp1.offset.value)
                        ret.offset = operand.base;
                    else {
                        operand = execOp(3, &exp2, &operand);   //sum of offset
                        ret.offset = operand.base;
                    }
                }
                return ret;
            }
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
    int i, j;
    tripleCode *prtCode;
    funcCode *prtFunc;

    for (i = 0; i < allCodes.pos; i++) {
         prtFunc = allCodes.data[i];

        if (!strcmp(prtFunc->info->name, "main"))
            printf("FUNCTION main :\n");
        else
            printf("FUNCTION f%s :\n", prtFunc->info->name);
        for (j = 0; j < prtFunc->paramList->pos; j++)
            printf("PARAM v%d\n", (int)prtFunc->paramList->data[j]);
        
        for (j = 0; j < prtFunc->toAlloc->pos; j++)
            printf("DEC v%d %d\n", (int)prtFunc->toAlloc->data[j], sizeOfVar(prtFunc->toAlloc->data[j]));
        putchar('\n');

        prtCode = prtFunc->code->next;
        while (prtCode != prtFunc->code) {
            switch(prtCode->op) {
                case 1:
                    printf("LABEL label%d :\n", prtCode->arg1.value);
                    break;
                case 2:
                    printf("v%d := ", prtCode->target);
                    printValueSt(&prtCode->arg1);
                    putchar('\n');
                    break;
                case 3:
                    printf("v%d := ", prtCode->target);
                    printValueSt(&prtCode->arg1);
                    printf(" + ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    break;
                case 4:
                    printf("v%d := ", prtCode->target);
                    printValueSt(&prtCode->arg1);
                    printf(" - ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    break;
                case 5:
                    printf("v%d := ", prtCode->target);
                    printValueSt(&prtCode->arg1);
                    printf(" * ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    break;
                case 6:
                    printf("v%d := ", prtCode->target);
                    printValueSt(&prtCode->arg1);
                    printf(" / ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    break;
                case 7:
                    printf("v%d := &v%d\n", prtCode->target, prtCode->arg1.value);
                    break;
                case 8:
                    printf("v%d := *v%d\n", prtCode->target, prtCode->arg1.value);
                    break;
                case 9:
                    printf("*v%d := ", prtCode->target);
                    printValueSt(&prtCode->arg1);
                    putchar('\n');
                    break;
                case 10:
                    printf("GOTO label%d\n", prtCode->arg1.value);
                    break;
                case 12:
                    printf("RETURN ");
                    if (prtCode->arg1.isImm == 2)
                        printf("*v%d\n", prtCode->arg1.value);
                    else
                        printValueSt(&prtCode->arg1);
                    putchar('\n');
                    break;
                case 13:
                    printf("ARG ");
                    printValueSt(&prtCode->arg1);
                    putchar('\n');
                    break;
                case 14:
                    printf("v%d := CALL f%s\n", prtCode->target, ((funcInfo *)prtCode->arg1.value)->name);
                    break;
                case 15:
                    printf("READ v%d\n", prtCode->target)                 ;
                    break;
                case 16:
                    printf("WRITE ");
                    if (prtCode->arg1.isImm == 2)
                        printf("*v%d", prtCode->arg1.value);
                    else
                        printValueSt(&prtCode->arg1);

                    putchar('\n');
                    break;
                default:
                    printf("IF ");
                    printValueSt(&prtCode->arg1);
                    printRelop(prtCode->op - 11);
                    printValueSt(&prtCode->arg2);
                    printf(" GOTO label%d\n", prtCode->target);

            }
            prtCode = prtCode->next;
        }
        putchar('\n');
    }
}

int processOffset(expTransInfo *info)
{
    int temp;
    valueSt st1;

    if (!info->offset.value && info->offset.isImm == 1)
        return info->base.value;
    if (info->base.isImm == 1)
        return info->offset.value;

    temp = getTempVar();
    st1.isImm = 0;
    st1.value = temp;
    addCode(7, temp, &info->base, NULL); //get the address of variable pointed by base
    addCode(3, temp, &st1, &info->offset);
    info->hasOffset = 0;

    if (!info->offset.isImm)
        releaseTempVar(info->offset.value);
    return temp;
}

//op: 3+ 4- 5* 6/
//return value is an immediate number or temp variable
static expTransInfo execOp(int op, expTransInfo *arg1, expTransInfo *arg2)
{
    valueSt st1, st2;
    expTransInfo ret = {{1, 0}, {1, 0}, 0, 0, NULL};

    if (arg1->base.isImm && arg2->base.isImm && !arg1->hasOffset && !arg2->hasOffset) {
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
                st1.value = processOffset(arg2);
                addCode(8, st1.value, &st1, NULL);
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
                st1.value = processOffset(arg1);
                addCode(8, st1.value, &st1, NULL);
            } else
                st1 = arg1->base;
            addCode(op, arg2->base.value, &arg2->base, &st1);
            ret.base = arg2->base;
            return ret;
        }
        else {
            int tar[2];
            int i = 0;
            if (arg1->hasOffset) {
                st1.isImm = 2;
                st1.value = processOffset(arg1);
                if (isTempVar(&st1))
                    tar[i++] = st1.value;
            } else
                st1 = arg1->base;

            if (arg2->hasOffset) {
                st2.isImm = 2;
                st2.value = tar[i++] = processOffset(arg2);
            } else
                st2 = arg2->base;
            
            if (!i) {
                tar[0] = getTempVar();
                addCode(op, tar[0], &st1, &st2);
            } else if (i == 1) {
                addCode(op, tar[0], &st1, &st2);
            } else {
                addCode(op, tar[0], &st1, &st2);
                releaseTempVar(tar[1]);
            }

            ret.base.isImm = 0;
            ret.base.value = tar[0];
            return ret;
        }
    }
}

int isTempVar(valueSt *st)
{
    return (st->isImm != 1) && st->value < TEMPVARNUM;
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

void addParam()
{
    int i;
    symNode *searchedSym;

    for (i = 0; i < curFunc->info->param->pos; i++) {
        searchedSym = searchSymbol(curFunc->info->param->data[i]->name, 1, NULL);
        addVariable(curFunc->paramList, searchedSym->info);
    }
}

static void printValueSt(valueSt *st)
{
    if (st->isImm == 1)
        printf("#%d", st->value);
    else if (!st->isImm)
        printf("v%d", st->value);
    else if (st->isImm == 2)
        printf("*v%d", st->value);
    else
        printf("&v%d", st->value);
}

int getLabelNum()
{
    return labelNum++;
}

int translateRelop(char *relop)
{
    if (!strcmp(relop, ">"))
        return 10;
    else if (!strcmp(relop, "<"))
        return 20;
    else if (!strcmp(relop, ">="))
        return 30;
    else if (!strcmp(relop, "<="))
        return 40;
    else if (!strcmp(relop, "=="))
        return 50;
    else
        return 60;
}

static void printRelop(int relop)
{
    switch(relop) {
        case 10:
            printf(" > ");
            break;
        case 20:
            printf(" < ");
            break;
        case 30:
            printf(" >= ");
            break;
        case 40:
            printf(" <= ");
            break;
        case 50:
            printf(" == ");
            break;
        default:
            printf(" != ");
    }
}

static int isParam(varList *paramList, varInfo *param)
{
    int i;

    for (i = 0; i < paramList->pos; i++)
        if (paramList->data[i] == param)
            return 1;

    return 0;
}