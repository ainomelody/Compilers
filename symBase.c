#include "symBase.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

symTableStack *scopeStack;

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
        info->data = realloc(sizeof(int) * info->size);
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
        list->data = realloc(sizeof(varInfo *) * list->size);
    }
    for (i = 0; i < list->pos; i++)
        if (!strcmp(list->data[i]->name, var->name))
            return -1;
    list->data[list->pos++] = var;

    return 0;
}

void freeVarList(varList **list)
{
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
}

void getInScope()
{
    if (scopeStack->pos == scopeStack->size) {
        scopeStack->size += LISTINCSIZE;
        scopeStack->symData = realloc(sizeof(symNode *) * scopeStack->size);
        scopeStack->stData = realloc(sizeof(structDefInfo *) * scopeStack->size);
    }
    scopeStack->symData[scopeStack->pos] = scopeStack->stData[scopeStack->pos] = NULL;
    scopeStack->pos++;
}

void getOutScope()
{
    scopeStack->pos--;
    freeStInfo(scopeStack->stData[scopeStack->pos]);
    freeSymTable(scopeStack->symData[scopeStack->pos]);
    scopeStack->stData[scopeStack->pos] = scopeStack->symData[scopeStack->pos] = NULL;
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
    free(info);
}

int getExpType(Node *exp)
{

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