#include "midCode.h"
#include <stdio.h>
#include <string.h>

static int sArgIndex;
static void printHead();
static int getFreeSArg();
static void processValueSt(valueSt *st);
static int getOffsetOfVar(varInfo *var);
static void printValueSt(valueSt *st);

static funcCode *prtFunc;

void printTargetCode()
{
    int i, j;
    tripleCode *prtCode;
    int paramSize;
    int midTarget;

    printHead();
    for (i = 0; i < allCodes.pos; i++) {
        prtFunc = allCodes.data[i];

        if (!strcmp(prtFunc->info->name, "main"))
            printf("main:\n");
        else
            printf("f%s:\n", prtFunc->info->name);
        if (prtFunc->space)
            printf("addi $sp, $sp, -%d\n", prtFunc->space);     //alloc space for local var

        prtCode = prtFunc->code->next;
        paramSize = 0;
        while (prtCode != prtFunc->code) {
            switch(prtCode->op) {
                case 1:
                    printf("label%d:\n", prtCode->arg1.value);
                    break;
                case 2:
                    processValueSt(&prtCode->arg1);
                    if (prtCode->target < 10 && prtCode->arg1.isImm == 1)
                        printf("li $t%d, %d\n", prtCode->target, prtCode->arg1.value);
                    else if (prtCode->target > 10 && prtCode->arg1.isImm == 1) {
                        int sarg = getFreeSArg();

                        printf("li $s%d, %d\n", sarg, prtCode->arg1.value);
                        printf("sw $s%d, %d($sp)\n", sarg, getOffsetOfVar((varInfo *)prtCode->target));
                    } else if (prtCode->target < 10) {
                        printf("move $t%d, ", prtCode->target);
                        printValueSt(&prtCode->arg1);
                        putchar('\n');
                    } else {
                        printf("sw ");
                        printValueSt(&prtCode->arg1);
                        printf(", %d($sp)\n", getOffsetOfVar((varInfo *)prtCode->target));
                    }
                    break;
                case 3:
                case 4:
                    processValueSt(&prtCode->arg1);
                    processValueSt(&prtCode->arg2);
                    if (prtCode->arg1.isImm == 1 && prtCode->op == 4) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg1.value);
                        prtCode->arg1.isImm = 6;
                        prtCode->arg1.value = midTarget;
                    }
                    if (prtCode->op == 3)
                        printf("add ");
                    else
                        printf("sub ");
                    if (prtCode->target < 10)
                        printf("$t%d, ", prtCode->target);
                    else {
                        midTarget = getFreeSArg();
                        printf("$s%d, ", midTarget);
                    }
                    printValueSt(&prtCode->arg1);
                    printf(", ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    if (prtCode->target > 10)
                        printf("sw $s%d, %d($sp)\n", midTarget, getOffsetOfVar((varInfo *)prtCode->target));
                    break;
                case 5:
                    processValueSt(&prtCode->arg1);
                    processValueSt(&prtCode->arg2);
                    if (prtCode->arg2.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg2.value);
                        prtCode->arg2.value = midTarget;
                        prtCode->arg2.isImm = 6;
                    }
                    printf("mul ");
                    if (prtCode->target < 10)
                        printf("$t%d, ", prtCode->target);
                    else {
                        midTarget = getFreeSArg();
                        printf("$s%d, ", midTarget);
                    }
                    printValueSt(&prtCode->arg1);
                    printf(", ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    if (prtCode->target > 10)
                        printf("sw $s%d, %d($sp)\n", midTarget, getOffsetOfVar((varInfo *)prtCode->target));
                    break;
                case 6:
                    processValueSt(&prtCode->arg1);
                    processValueSt(&prtCode->arg2);
                    if (prtCode->arg1.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg1.value);
                        prtCode->arg1.value = midTarget;
                        prtCode->arg1.isImm = 6;
                    }

                    if (prtCode->arg2.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg2.value);
                        prtCode->arg2.value = midTarget;
                        prtCode->arg2.isImm = 6;
                    }
                    printf("div ");
                    printValueSt(&prtCode->arg1);
                    printf(", ");
                    printValueSt(&prtCode->arg2);
                    putchar('\n');
                    if (prtCode->target < 10)
                        printf("mflo $t%d\n", prtCode->target);
                    else {
                        midTarget = getFreeSArg();
                        printf("mflo $s%d\n", midTarget);
                        printf("sw $s%d, %d($sp)\n", midTarget, getOffsetOfVar((varInfo *)prtCode->target));
                    }
                    break;
                case 7:
                    printf("add $t%d, $sp, %d\n", prtCode->target, getOffsetOfVar((varInfo *)prtCode->arg1.value));
                    break;
                case 8:
                    if (prtCode->target > 10) {
                        midTarget = getFreeSArg();
                        printf("lw $s%d, 0($t%d)\n", midTarget, prtCode->arg1.value);
                        printf("sw $s%d, %d($sp)\n", midTarget, getOffsetOfVar((varInfo *)prtCode->target));
                    } else
                        printf("lw $t%d, 0($t%d)\n", prtCode->target, prtCode->arg1.value);
                    break;
                case 9:
                    if (prtCode->arg1.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg1.value);
                        printf("sw $s%d, 0($t%d)\n", midTarget, prtCode->target);
                    } else {
                        processValueSt(&prtCode->arg1);
                        printf("sw ");
                        printValueSt(&prtCode->arg1);
                        printf(", 0($t%d)\n", prtCode->target);
                    }
                    break;
                case 10:
                    printf("j label%d\n", prtCode->arg1.value);
                    break;
                case 12:
                    processValueSt(&prtCode->arg1);
                    if (prtCode->arg1.isImm == 1)
                        printf("li $v0, %d\n", prtCode->arg1.value);
                    else {
                        printf("move $v0, ");
                        printValueSt(&prtCode->arg1);
                        putchar('\n');
                    }
                    if (prtFunc->space)
                        printf("add $sp, $sp, %d\n", prtFunc->space);
                    printf("jr $ra\n");
                    break;
                case 13:
                    processValueSt(&prtCode->arg1);
                    if (prtCode->arg1.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg1.value);
                        prtCode->arg1.value = midTarget;
                        prtCode->arg1.isImm = 6;
                    }
                    printf("sw ");
                    printValueSt(&prtCode->arg1);
                    printf(", %d($sp)\n", -paramSize - 4);
                    paramSize += 4;
                    break;
                case 14:
                    printf("move $a3, $ra\n");
                    if (paramSize)
                        printf("sub $sp, $sp, %d\n", paramSize);
                    printf("jal f%s\n", ((funcInfo *)(prtCode->arg1.value))->name);
                    if (paramSize)
                        printf("add $sp, $sp, %d\n", paramSize);
                    printf("move $ra, $a3\n");
                    paramSize = 0;
                    if (prtCode->target < 10)
                        printf("move $t%d, $v0\n", prtCode->target);
                    else
                        printf("sw $v0, %d($sp)\n", getOffsetOfVar((varInfo *)prtCode->target));
                    break;
                case 15:
                    printf("move $a3, $ra\n");
                    printf("jal read\n");
                    printf("move $ra, $a3\n");
                    if (prtCode->target < 10)
                        printf("move $t%d, $v0\n", prtCode->target);
                    else
                        printf("sw $v0, %d($sp)\n", getOffsetOfVar((varInfo *)prtCode->target));
                    break;
                case 16:
                    printf("move $a3, $ra\n");
                    if (prtCode->arg1.isImm == 0) {
                        if (prtCode->arg1.value < 10)
                            printf("move $a0, $t%d\n", prtCode->arg1.value);
                        else
                            printf("lw $a0, %d($sp)\n", getOffsetOfVar((varInfo *)prtCode->arg1.value));
                    } else if (prtCode->arg1.isImm == 1)
                        printf("li $a0, %d\n", prtCode->arg1.value);
                    else
                        printf("lw $a0, 0($t%d)\n", prtCode->arg1.value);
                    printf("jal write\n");
                    printf("move $ra, $a3\n");
                    break;
                default:
                    processValueSt(&prtCode->arg1);
                    if (prtCode->arg1.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg1.value);
                        prtCode->arg1.isImm = 6;
                        prtCode->arg1.value = midTarget;
                    }
                    processValueSt(&prtCode->arg2);
                    if (prtCode->arg2.isImm == 1) {
                        midTarget = getFreeSArg();
                        printf("li $s%d, %d\n", midTarget, prtCode->arg2.value);
                        prtCode->arg2.isImm = 6;
                        prtCode->arg2.value = midTarget;
                    }
                    switch(prtCode->op - 11) {
                        case 10:
                            printf("bgt ");
                            break;
                        case 20:
                            printf("blt ");
                            break;
                        case 30:
                            printf("bge ");
                            break;
                        case 40:
                            printf("ble ");
                            break;
                        case 50:
                            printf("beq ");
                            break;
                        default:
                            printf("bne ");
                    }
                    printValueSt(&prtCode->arg1);
                    printf(", ");
                    printValueSt(&prtCode->arg2);
                    printf(", label%d\n", prtCode->target);
            }
        prtCode = prtCode->next;
        }
        putchar('\n');
    }
}

static void printHead()
{
    printf(".data\n");
    printf("_prompt: .asciiz \"Enter an integer:\"\n");
    printf("_ret: .asciiz \"\\n\"\n" );
    printf(".globl main\n");
    printf(".text\n");
    printf("read:\n");
    printf("\tli $v0, 4\n");
    printf("\tla $a0, _prompt\n");
    printf("\tsyscall\n");
    printf("\tli $v0, 5\n");
    printf("\tsyscall\n");
    printf("\tjr $ra\n\n");
    
    printf("write:\n");
    printf("\tli $v0, 1\n");
    printf("\tsyscall\n");
    printf("\tli $v0, 4\n");
    printf("\tla $a0, _ret\n");
    printf("\tsyscall\n");
    printf("\tmove $v0, $0\n");
    printf("\tjr $ra\n\n");
}

static void processValueSt(valueSt *st)
{
    int offset;
    varInfo *var = (varInfo *)(st->value);
    int sarg;

    if (st->isImm == 1)
        return;

    if (!st->isImm && isTempVar(st))
        return;

    sarg = getFreeSArg();
    if (st->isImm == 2 && isTempVar(st)) {
        printf("lw $%d, 0($t%d)\n", sarg, st->value);
        st->isImm = 6;
        st->value = sarg;
        return;
    }

    offset = getOffsetOfVar(var);
    if (!st->isImm || st->isImm == 2)
        printf("lw $s%d, %d($sp)\n", sarg, offset);
    else
        printf("addi sarg, $sp, %d\n", offset);
    st->isImm = 6;
    st->value = sarg;
}

static int getFreeSArg()
{
    sArgIndex += 1;
    sArgIndex %= 8;

    return sArgIndex;
}

static void printValueSt(valueSt *st)
{
    switch(st->isImm) {
        case 0:
            printf("$t");
            break;
        case 6:
            printf("$s");
    }
    printf("%d", st->value);
}

static int getOffsetOfVar(varInfo *var)
{
    int offset;

    offset = var->offset;
    if (isParam(prtFunc->paramList, var))       //param of function
        offset += prtFunc->space;
    return offset;
}