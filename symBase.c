#include "symBase.h"
#include <stdlib.h>

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

void addVariable(varList *list, varInfo *var)
{
    if (list->pos == list->size) {
        list->size += LISTINCSIZE;
        list->data = realloc(sizeof(varInfo *) * list->size);
    }
    list->data[list->pos++] = var;
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
}

void getInScope()
{

}

void getOutScope()
{

}