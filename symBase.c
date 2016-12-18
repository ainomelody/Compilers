#include "symBase.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

symTableStack *scopeStack;
symNode **globalSymTable;

int structIndex = 0;    //name assign to the anonymous struct type, @struct+structIndex
arrayInfo *newArrayInfo()
{
    arrayInfo *ret = malloc(sizeof(arrayInfo));
    ret->size = LISTSIZE;
    ret->pos = 0;
    ret->data = malloc(sizeof(int) * LISTSIZE);

    return ret;
}

void addArrayDim(arrayInfo *info, int ubound)
{
    if (info->pos == info->size) {
        info->size += LISTINCSIZE;
        info->data = realloc(info->data, sizeof(int) * info->size);
    }

    info->data[info->pos++] = ubound;
}

void freeArrayInfo(arrayInfo **info)
{
    free((*info)->data);
    free(*info);
    *info = NULL;
}

varList *newVarList()
{
    varList *ret = malloc(sizeof(varList));
    ret->size = LISTSIZE;
    ret->pos = 0;
    ret->data = malloc(sizeof(varInfo *) *  LISTSIZE);

    return ret;
}

int addVariable(varList *list, varInfo *var)
{
    int i;

    if (list->pos == list->size) {
        list->size += LISTINCSIZE;
        list->data = realloc(list->data, sizeof(varInfo *) * list->size);
    }
    if (var->name != NULL)
        for (i = 0; i < list->pos; i++)
            if (!strcmp(list->data[i]->name, var->name))
                return -1;

    list->data[list->pos++] = var;

    return 0;
}

void freeVarList(varList **list)
{
    int i;

    if (*list == NULL)
        return;
    for (i = 0; i < (*list)->pos; i++)
        freeVarInfo((*list)->data[i]);
    free((*list)->data);
    free(*list);
    *list = NULL;
}

void initScopeStack()
{
    scopeStack = malloc(sizeof(symTableStack));
    scopeStack->size = 10;
    scopeStack->symData = malloc(sizeof(symNode *) * 10);
    scopeStack->stData = malloc(sizeof(structDefInfo *) * 10);
    scopeStack->pos = 0;
    getInScope();
    globalSymTable = scopeStack->symData;
}

void getInScope()
{
    if (scopeStack->pos == scopeStack->size) {
        scopeStack->size += LISTINCSIZE;
        scopeStack->symData = realloc(scopeStack->symData, sizeof(symNode *) * scopeStack->size);
        scopeStack->stData = realloc(scopeStack->stData, sizeof(structDefInfo *) * scopeStack->size);
    }
    scopeStack->symData[scopeStack->pos] = NULL;
    scopeStack->stData[scopeStack->pos] = NULL;
    scopeStack->pos++;
}

void getOutScope()
{
    scopeStack->pos--;
    // freeStInfo(scopeStack->stData[scopeStack->pos]);
    // freeSymTable(scopeStack->symData[scopeStack->pos]);
    scopeStack->stData[scopeStack->pos] = NULL;
    scopeStack->symData[scopeStack->pos] = NULL;
}

structDefInfo *searchStruct(char *name, int onlyCurScope, structDefInfo **parent)
{
    int i;
    structDefInfo *node;

    if (parent != NULL)
        *parent = NULL;

    for (i = scopeStack->pos - 1; i >= 0; i--) {
        node = scopeStack->stData[i];
        while (node != NULL) {
            int cmp = strcmp(name, node->name);

            if (cmp == 0)
                return node;
            if (i == scopeStack->pos - 1 && parent != NULL)
                *parent = node;

            if (cmp < 0)
                node = node->left;
            else
                node = node->right;
        }
        if (onlyCurScope)
            break;
    }

    return NULL;
}

symNode *searchSymbol(char *name, int onlyCurScope, symNode **parent)
{
    int i;
    symNode *node;

    if (parent != NULL)
        *parent = NULL;

    for (i = scopeStack->pos - 1; i >= 0; i--) {
        node = scopeStack->symData[i];
        while (node != NULL) {
            int cmp = strcmp(name, node->name);

            if (cmp == 0)
                return node;
            if (i == scopeStack->pos - 1 && parent != NULL)
                *parent = node;

            if (cmp < 0)
                node = node->left;
            else
                node = node->right;
        }
        if (onlyCurScope)
            break;
    }

    return NULL;
}

void freeSymTable(symNode *table)
{
    if (table == NULL)
        return;
    if (table->left != NULL)
        freeSymTable(table->left);
    if (table->right != NULL)
        freeSymTable(table->right);
    free(table->info);
    free(table);
}

void freeStInfo(structDefInfo *info)
{
    if (info == NULL)
        return;

    if (info->left != NULL)
        freeStInfo(info->left);
    if (info->right != NULL)
        freeStInfo(info->right);
    free(info->region);
    free(info);
}

void addStructInfo(structDefInfo *info, structDefInfo *parent)
{
    if (parent == NULL)
        scopeStack->stData[scopeStack->pos - 1] = info;
    else {
        if (strcmp(info->name, parent->name) < 0) {
            info->left = parent->left;
            parent->left = info;
        } else {
            info->right = parent->right;
            parent->right = info;
        }
    }
}

void addSymbol(symNode *sym, symNode *parent)
{
    if (parent == NULL)
        scopeStack->symData[scopeStack->pos - 1] = sym;
    else {
        if (strcmp(sym->name, parent->name) < 0) {
            sym->left = parent->left;
            parent->left = sym;
        } else {
            sym->right = parent->right;
            parent->right = sym;
        }
    }
}

void freeVarInfo(varInfo *info)
{
    if (info->isArray)
        free(info->arrInfo);
    free(info);
}

int matchVarList(varList *list1, varList *list2)
{
    int i;

    if (list1->pos != list2->pos)
        return 0;
    for (i = 0; i < list1->pos; i++) {
        varInfo *info1 = list1->data[i], *info2 = list2->data[i];

        if (info1->isArray != info2->isArray)
            return 0;
        if (info1->isArray && !matchArrInfo(info1->arrInfo, info2->arrInfo))
            return 0;
        if (!checkTypeConsist(info1->type, info2->type))
                return 0;
    }

    return 1;
}

int matchArrInfo(arrayInfo *info1, arrayInfo *info2)
{
    int i;

    if (info1->pos != info2->pos)
        return 0;
    for (i = 0; i < info1->pos; i++)        //In C language, the first dimension may not match.The ubound of loop is info1->pos - 1
        if (info1->data[i] != info2->data[i])
            return 0;

    return 1;
}

int getDimNum(varInfo *var)
{
    if (!var->isArray)
        return 0;
    else
        return var->arrInfo->pos - 1;
}

varInfo *searchRegion(structDefInfo *st, char *name)
{
    int i;

    if (st == NULL)
        return NULL;
    for (i = 0; i < st->region->pos; i++)
        if (!strcmp(st->region->data[i]->name, name))
            return st->region->data[i];

    return NULL;
}

arrayInfo *copyArrInfo(arrayInfo *toCopy)
{
    int i;
    arrayInfo *ret;

    if (toCopy == NULL)
        return NULL;

    ret = malloc(sizeof(arrayInfo));
    ret->size = toCopy->size;
    ret->pos = toCopy->pos;
    ret->data = malloc(sizeof(int) * ret->size);
    for (i = 0; i < ret->pos; i++)
        ret->data[i] = toCopy->data[i];

    return ret;
}

arrayInfo *removeOneDim(arrayInfo *info)
{
    int i;

    if (info == NULL)
        return NULL;
    
    if (info->pos == 1) {
        freeArrayInfo(&info);
        return NULL;
    }

    info->pos--;

    return info;
}

int checkTypeConsist(int type1, int type2)
{
    int ret, i;

    if (type1 == type2)
        return 1;
    if (type1 < 2 || type2 < 2)
        return 0;

    return matchVarList(((structDefInfo *)type1)->region, ((structDefInfo *)type2)->region);
}

int sizeOfVar(varInfo *var)
{
    int size;
    int i;

    if (var == NULL)
        return 0;
    size = sizeOfType(var->type);
    
    if (!var->isArray)
        return size;
    for (i = 0; i < var->arrInfo->pos; i++)
        size *= var->arrInfo->data[i];

    return size;
}

int sizeOfType(int type)
{
    int i;
    int total = 0;
    structDefInfo *st;

    if (type < 0)
        return 0;
    if (type < 10)
        return 4;
    st = (structDefInfo *)type;
    // for (i = 0; i < st->region->pos; i++)
    //     total += sizeOfVar(st->region->data[i]);

    // return total;
    return st->size;
}