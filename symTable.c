#include "symTable.h"
#include "symBase.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void parseExtDef(Node *tree);
static int getSpecifierType(Node *node);        //node->type == "Specifier"

/*node->type == "DefList", it should always be called until the returned value is NULL.The first time pass in node, then NULL*/
static varInfo *parseDefList(Node *node);   
static varInfo *parseVarDec(Node *node);        //node->type == "VarDec"   
static funcInfo *parseFuncDec(Node *node); 
static void parseCompSt(Node *node, varList *args);

int hasError;

void analyse(Node *tree)
{
    initScopeStack();
    tree = tree->child;
    while (tree->lineNum != -1) {
        parseExtDef(tree->child);
        tree = tree->child->sibling;
    }
    //clean
}

static void parseExtDef(Node *node)
{
    int type = getSpecifierType(node->child);

    if (node->childNum == 2)
        return;
    node = node->child->sibling; //ExtDecList or FunDec
    if (!strcmp(node->type, "ExtDecList")) {
        node = node->child;     //VarDec
        while (node) {
            varInfo *var = parseVarDec(node);
            symNode *insertLoc, *insNode;

            if (searchSymbol(var->name, 1, &insertLoc)) {
                printf("Error type 3 at Line %d: Redefined variable \"%s\".\n", var->lineNum, var->name);
                hasError = 1;
                freeVarInfo(var);
            } else if (searchStruct(var->name, 1, NULL)) {
                printf("Error type 3 at Line %d: Redefined symbol \"%s\".\n", var->lineNum, var->name);
                hasError = 1;
            } else {
                var->type = type;
                insNode = malloc(sizeof(symNode));
                insNode->type = 1;
                insNode->name = var->name;
                insNode->left = insNode->right = NULL;
                insNode->info = var;
                addSymbol(insNode, insertLoc);
            }

            if (node->sibling == NULL)
                node = NULL;
            else
                node = node->sibling->sibling->child;
        }
    } else {    //FunDec
        funcInfo *func = parseFuncDec(node);
        symNode *insertLoc;
        symNode *searchRes = searchSymbol(func->name, 1, &insertLoc);

        if (!strcmp(node->sibling->type, "SEMI"))
            func->flag = 1;
        else
            func->flag = 2;
        func->retType = type;

        if (searchRes) {
            if (searchRes->type) { //variable
                printf("Error type 4 at Line %d: Redefined symbol \"%s\".\n", func->lineNum, func->name);
                hasError = 1;
            }  else {   //function
                funcInfo *prev = (funcInfo *)(searchRes->info);

                if ((prev->flag & 2) && func->flag == 2) {  //redefine
                    printf("Error type 4 at Line %d: Redefined function \"%s\".\n", func->lineNum, func->name);
                    hasError = 1;
                } else {    //prev def + decl or prev decl + def or prev decl + decl
                    if (prev->retType != func->retType || !matchVarList(prev->param, func->param)) { //parameters or retType can't match
                        if (prev->flag & 2) //prev def + decl
                            printf("Error type 19 at Line %d: Inconsistent declaration of function \"%s\".\n", func->lineNum, func->name);
                        else if (func->flag == 2) { //prev decl + def
                            printf("Error type 19 at Line %d: Inconsistent declaration of function \"%s\".\n", prev->lineNum, prev->name);
                            prev->retType = func->retType;
                            prev->param = func->param;
                            prev->lineNum = func->lineNum;
                            prev->flag = 2;
                            parseCompSt(node->sibling, func->param);
                        } else //prev decl + decl
                            printf("Error type 19 at Lien %d: Inconsistent declaration of function \"%s\".\n", func->lineNum, func->name); 
                    } else    //can match
                        prev->flag |= func->flag;
                }
                freeVarList(&(func->param));
                free(func);
            } 
        } else {
            symNode *insNode = malloc(sizeof(symNode));
            insNode->name = func->name;
            insNode->type = 0;
            insNode->left = insNode->right = NULL;
            insNode->info = func;
            addSymbol(insNode, insertLoc);
            if (func->flag == 2)
                parseCompSt(node->sibling, func->param);
        }
    }
}

static int getSpecifierType(Node *node)
{
    /*This function reports error 16, 17*/

    structDefInfo *stInfo, *parent, *newSt;
    char tempName[20];
    char *name = NULL;
    Node *defList = NULL;
    symNode *symInfo;
    varInfo *region;

    node = node->child;
    if (!strcmp(node->type, "TYPE")) {
        if (!strcmp(node->data.type, "int"))
            return 0;
        else
            return 1;
    }
    //node->type = "StructSpecifier"
    node = node->child->sibling; //OptTag or Tag
    if (!strcmp(node->type, "OptTag")) {
        if (node->lineNum >= 0) {   //assign id
            name = node->child->data.id;
            stInfo = searchStruct(name, 1, &parent);
            symInfo = searchSymbol(name, 1, NULL);
            if (stInfo || symInfo) {
                printf("Error type 16 at Line %d: Duplicated name \"%s\".\n", node->child->lineNum, name);
                hasError = 1;
            }
            if (stInfo) //There exists structure which has the same name.
                return (int)stInfo;
        } else {    //anonymous structure
            sprintf(tempName, "@struct%d", structIndex++);
            name = tempName;
            searchStruct(name, 1, &parent);
        }
        defList = node->sibling->sibling;
    } else {    //tag
        name = node->child->data.id;
        stInfo = searchStruct(name, 0, NULL);
        if (stInfo)
            return (int)stInfo;
        printf("Error type 17 at Line %d: Undefined structure \"%s\".\n", node->child->lineNum, name);
        hasError = 1;
        return -1;
    }
    newSt = malloc(sizeof(structDefInfo));
    strcpy(newSt->name, name);
    newSt->left = newSt->right = NULL;
    newSt->size = 0;
    newSt->region = newVarList();
    addStructInfo(newSt, parent);

    region = parseDefList(node->sibling->sibling);
    while (region != NULL) {
        int result = addVariable(newSt->region, region);
        if (result) {
            printf("Error type 15 at Line %d: Redefined field \"%s\".\n", region->lineNum, region->name);
            hasError = 1;
        }
        if (region->initExp != NULL) {
            printf("Error type 15 at Line %d: Initialize field \"%s\" when defining it.\n", region->initExp->lineNum, region->name);
            hasError = 1;
            region->initExp = NULL;
        }
        region = parseDefList(NULL);
    }

    return (int)newSt;
}

static varInfo *parseDefList(Node *node)
{
    static Node *defList, *decList;
    static int type;
    Node *varDec;
    varInfo *ret;

    if (node != NULL) {
        defList = node;
        decList = NULL;
    }
    while (defList->lineNum != -1 || decList != NULL) {
        if (decList == NULL) {
            type = getSpecifierType(defList->child->child);
            decList = defList->child->child->sibling;
            defList = defList->child->sibling;
        }
        varDec = decList->child->child;
        ret = parseVarDec(varDec);
        ret->type = type;

        if (varDec->sibling != NULL)
            ret->initExp = varDec->sibling->sibling;
        else
            ret->initExp = NULL;

        if (decList->child->sibling == NULL)
            decList = NULL;
        else
            decList = decList->child->sibling->sibling;
        return ret;
    }
    return NULL;
}

static varInfo *parseVarDec(Node *node)
{
    varInfo *ret = malloc(sizeof(varInfo));
    ret->lineNum = node->lineNum;
    ret->offset = 0;
    ret->initExp = NULL;
    if (node->child->sibling == NULL) {
        ret->name = node->child->data.id;
        ret->isArray = 0;
        ret->arrInfo = NULL;
        return ret;
    }

    ret->isArray = 1;
    ret->arrInfo = newArrayInfo();
    while (node->child->sibling != NULL) {
        addArrayDim(ret->arrInfo, node->child->sibling->sibling->data.intValue);
        node = node->child;
    }

    ret->name = node->child->data.id;

    return ret;
}

static funcInfo *parseFuncDec(Node *node)
{
    funcInfo *ret = malloc(sizeof(funcInfo));
    Node *paramDec;
    int type, result;
    varInfo *param;
    
    ret->name = node->child->data.id;
    ret->flag = 0;
    ret->lineNum = node->lineNum;
    if (node->childNum == 3) {
        ret->param = NULL;
        return ret;
    }

    ret->param = newVarList();
    paramDec = node->child->sibling->sibling->child;
    while (paramDec) {
        type = getSpecifierType(paramDec->child);
        param = parseVarDec(paramDec->child->sibling);
        param->type = type;
        result = addVariable(ret->param, param);
        if (result) {
            printf("Error type 3 at Line %d: Duplicated name \"%s\" in the parameters.\n", param->lineNum, param->name);
            hasError = 1;
        }
        if (paramDec->sibling == NULL)
            paramDec = NULL;
        else
            paramDec = paramDec->sibling->sibling->child;
    }

    return ret;
}

static void parseCompSt(Node *node, varList *args)
{

}