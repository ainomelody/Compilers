#include "symTable.h"
#include "symBase.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int type;
    int isLeft;
    arrayInfo *dims;
}expTypeInfo;

static void parseExtDef(Node *tree);
static int getSpecifierType(Node *node);        //node->type == "Specifier"

/*node->type == "DefList", it should always be called until the returned value is NULL.The first time pass in node, then NULL*/
static varInfo *parseDefList(Node *node);   
static varInfo *parseVarDec(Node *node);        //node->type == "VarDec"   
static funcInfo *parseFuncDec(Node *node); 
static void parseCompSt(Node *node, varList *args);
static expTypeInfo parseExp(Node *node);         //return the type of exp
static void parseStmt(Node *node);               //return the type of exp in RETURN statement
static void checkFuncDecl(symNode *node);                    //check if every declaration has a definition
static int checkTypeConsist(int type1, int type2);  //check if two types are consistent

int hasError;
static int curFuncRetType;                      //The ret type of current function.

void analyse(Node *tree)
{
    initScopeStack();
    tree = tree->child;
    while (tree->lineNum != -1) {
        parseExtDef(tree->child);
        tree = tree->child->sibling;
    }
    if (*globalSymTable != NULL)
        checkFuncDecl(*globalSymTable);
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
            if (func->flag == 2) {
                curFuncRetType = type;
                parseCompSt(node->sibling, func->param);
            }
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
    expTypeInfo retType;

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

        if (varDec->sibling != NULL) {
            ret->initExp = varDec->sibling->sibling;
            if (retType = parseExp(ret->initExp), retType.type != type || retType.dims) {
                printf("Error type 5 at Line %d: Type mismatched for assignment.\n", varDec->sibling->lineNum);
                hasError = 1;
            }
        }
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
    int i;
    symNode *insNode, *insLoc, *prev;
    varInfo *var;

    getInScope();
    if (args != NULL)
        for (i = 0; i < args->pos; i++) {
            searchSymbol(args->data[i]->name, 1, &insLoc);
            insNode = malloc(sizeof(symNode));
            insNode->type = 1;
            insNode->left = insNode->right = NULL;
            insNode->info = args->data[i];
            addSymbol(insNode, insLoc);
        }

    node = node->child->sibling;    //DefList
    var = parseDefList(node);
    while (var) {
        if (prev = searchSymbol(var->name, 1, &insLoc)) {
            if (prev->type)
                printf("Error type 3 at Line %d: Redefined variable \"%s\".\n", var->lineNum, var->name);
            else
                printf("Error type 3 at Line %d: Redefined symbol \"%s\".\n", var->lineNum, var->name);
            hasError = 1;
        } else {
            insNode = malloc(sizeof(symNode));
            insNode->type = 1;
            insNode->left = insNode->right = NULL;
            insNode->info = var;
            addSymbol(insNode, insLoc);
        }
        var = parseDefList(NULL);
    }

    node = node->sibling;       //StmtList
    while (node->lineNum != -1) {
        parseStmt(node->child);
        node = node->child->sibling;
    }
    getOutScope();
}

static void parseStmt(Node *node)
{
    expTypeInfo expType;

    switch (node->childNum) {
        case 1: parseCompSt(node->child, NULL); break;
        case 2: parseExp(node->child); break;
        case 3: if (expType = parseExp(node->child->sibling), expType.type != curFuncRetType || expType.dims) {
                    printf("Error type 8 at Line %d: Type mismatched for return.\n", node->lineNum);
                    hasError = 1;
                }
                break;
        case 5: node = node->child->sibling->sibling;       //Exp
                if (expType = parseExp(node->child->sibling), expType.type != 0 || expType.dims) {
                    printf("Error type 7 at Line %d: Type mismatched for conditional statement.\n", node->lineNum);
                    hasError = 1;
                }
                parseStmt(node->sibling->sibling);
                break;
        case 7: node = node->child->sibling->sibling;
                if (expType = parseExp(node->child->sibling), expType.type != 0 || expType.dims) {
                    printf("Error type 7 at Line %d: Type mismatched for conditional statement.\n", node->lineNum);
                    hasError = 1;
                }
                node = node->sibling->sibling;
                parseStmt(node);
                parseStmt(node->sibling->sibling);
                break;
        default:
            return;
    }
}

/*retValue: lower than zero represents error.
Legal address represents structure
*/

static expTypeInfo parseExp(Node *node)
{
    expTypeInfo exp1, exp2, ret = {0, 0, NULL};

    switch (node->childNum)
    {
        case 1:
            node = node->child;
            if (!strcmp(node->type, "ID")) {
                symNode *varNode = searchSymbol(node->data.id, 0, NULL);
                if (varNode == NULL) {
                    printf("Error type 1 at Line %d: Undefined variable \"%s\".\n", node->lineNum, node->data.id);
                    hasError = 1;
                    ret.isLeft = 0;
                    ret.type = -2;
                    return ret;
                } else {    //found symbol
                    if (varNode->type) {   //variable
                        varInfo *info = (varInfo *)(varNode->info);

                        ret.isLeft = 1;
                        ret.dims = copyArrInfo(info->arrInfo);
                        ret.type = info->type;
                        return ret;
                    } else {    //function
                        printf("Error type 9 at Line %d: Function name cannot be used directly.\n", node->lineNum);
                        hasError = 1;
                        ret.type = -2;
                        return ret;
                    }
                }
            } else {
                if (!strcmp(node->type, "INT"))
                    ret.type = 0;
                else
                    ret.type = 1;

                return ret;
            }
            break;
        
        case 2:
            node = node->child;
            exp1 = parseExp(node->sibling);
            if (exp1.dims || exp1.type > 1) {
                printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->sibling->lineNum);
                hasError = 1;
                exp1.type = -2;
            }
            exp1.isLeft = 0;
            return exp1;
            break;
        
        case 3:
            node = node->child;
            if (!strcmp(node->type, "ID")) {
                symNode *func = searchSymbol(node->data.id, 0, NULL);

                if (func == NULL) {
                    printf("Error type 2 at Line %d: Undefined function \"%s\".\n", node->lineNum, node->data.id);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                } else if (func->type) {    //find a variable
                    printf("Error type 11 at Line %d: \"%s\" is not a function.\n", node->lineNum, node->data.id);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                } else {
                    if (((funcInfo *)func->info)->param != NULL) {
                        printf("Error type 9 at Line %d: Function \"%s\" needs parameters.\n", node->lineNum, node->data.id);
                        hasError = 1;
                    }
                    ret.type = ((funcInfo *)(func->info))->retType;
                    return ret;
                }
            } else if (!strcmp(node->type, "LP")) { //LP Exp RP
                return parseExp(node->sibling); 
            } else if (!strcmp(node->sibling->type, "DOT")) {   //Exp DOT ID
                varInfo *region;

                exp1 = parseExp(node);
                region =  = searchRegion((structDefInfo *)exp1.type, node->sibling->sibling->data.id);
                
                if (exp1.type < 1) {
                    printf("Error type 13 at Line %d: Illegal use of \".\".\n", node->sibling->lineNum);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                } else if (region == NULL){
                    node = node->sibling->sibling;
                    printf("Error type 14 at Line %d: Non-existent field \"%s\".\n", node->lineNum, node->data.id);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                } else {
                    ret.type = region->type;
                    ret.isLeft = 1;
                    ret.dims = copyArrInfo(region->arrInfo);

                    return ret;
                }
            } else {
                exp1 = parseExp(node);
                exp2 = parseExp(node->sibling->sibling);

                if (!strcmp(node->sibling->type, "ASSIGNOP")) {
                    if (!exp1.isLeft || exp1.dims) {
                        printf("Error type 6 at Line %d: The left-hand side of an assignment must be a variable.\n", node->lineNum);
                        hasError = 1;
                        ret.type = -2;
                        return ret;
                    }
                }
                if (exp1.type != exp2.type || exp1.dims != NULL || exp2.dims != NULL) {
                    printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->lineNum);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                }
                ret.type = exp1.type;
                return ret;
            }
        case 4:
            node = node->child;
            if (!strcmp(node->type, "ID")) { //function call
                symNode *func = searchSymbol(node->data.id, 0, NULL);
                varList *args;
                int lineNum;

                if (func == NULL) {
                    printf("Error type 2 at Line %d: Undefined function \"%s\".\n", node->lineNum, node->data.id);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                } else if (func->type) {    //find a variable
                    printf("Error type 11 at Line %d: \"%s\" is not a function.\n", node->lineNum, node->data.id);
                    hasError = 1;
                    ret.type = -2;
                    return ret;
                }

                args = newVarList();
                node = node->sibling->sibling->child;
                lineNum = node->lineNum;
                while (node) {
                    expTypeInfo info = parseExp(node);
                    varInfo *var = malloc(sizeof(varInfo));

                    var->type = info.type;
                    var->arrInfo = info.dims;
                    if (info.dims) 
                        var->isArray = 1;
                    else
                        var->isArray = 0;
                    addVariable(args, var);

                    if (node->sibling == NULL)
                        node = NULL;
                    else
                        node = node->sibling->sibling->child;
                }
                if (!matchVarList(args, ((funcInfo *)func->info)->param)) {
                    printf("Error type 9 at Line %d: Wrong type of parameters for function \"%s\".\n", lineNum,func->name);
                    hasError = 1;
                }
                ret.type = ((funcInfo *)func->info)->retType;

                return ret;
            } else {    //array
                exp1 = parseExp(node);
                exp2 = parseExp(node->sibling->sibling);

                if (exp1.dims == NULL) {
                    printf("Error type 10 at Line %d: Use too many [].\n", node->lineNum);
                    ret.type = -2;
                    return ret;
                }

                if (exp2.type != 0 || exp2.dims) {
                    printf("Error type 12 at Line %d: The index of array should be an integer.\n", node->sibling->sibling->lineNum);
                    hasError = 1;
                }
                ret.type = exp1.type;
                ret.dims = removeOneDim(exp1.dims);
                return ret;
            }

            break;
        default:
            ret.type = -2;
            return ret;
    }
}

static void checkFuncDecl(symNode *node)
{   
    if (node->type == 0) {
        funcInfo *info = (funcInfo *)node->info;

        if ((info->flag & 2) == 0) {
            printf("Error type 18 at Line %d: Incomplete definition of function \"%s\".\n", info->lineNum, info->name);
            hasError = 1;
        }
    }
    if (node->left)
        checkFuncDecl(node->left);
    if (node->right)
        checkFuncDecl(node->right);
}

static int checkTypeConsist(int type1, int type2)
{
    int ret, i;

    if (type1 == type2)
        return 1;
    if (type1 < 2 || type2 < 2)
        return 0;

    return matchVarList(((structDefInfo *)type1)->region, ((structDefInfo *)type2)->region);
}