#include "tree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

Node *newNode(char *type, int lineNum)
{
	Node *ret = malloc(sizeof(Node));

	strcpy(ret->type, type);
	ret->lineNum = lineNum;
	ret->child = ret->sibling = NULL;
	ret->childNum = 0;

	return ret;
}

void printTree(Node *tree, int level)
{
	int i;

	for (i = 0; i < 2 * level; i++)
		putchar(' ');
	printf("%s", tree->type);
	if (tree->child != NULL)
	{
		printf(" (%d)\n", tree->lineNum);
		printTree(tree->child, level + 1);
	}
	else
	{
		if (!strcmp(tree->type, "ID"))
			printf(": %s\n", tree->data.id);
		else if (!strcmp(tree->type, "TYPE"))
			printf(": %s\n", tree->data.type);
		else if (!strcmp(tree->type, "INT"))
			printf(": %d\n", tree->data.intValue);
		else if (!strcmp(tree->type, "FLOAT"))
			printf(": %f\n", tree->data.floatValue);
		else
			printf("\n");
	}

	if (tree->sibling != NULL)
		printTree(tree->sibling, level);
}

void addChild(Node *father, int n, ...)
{
	int i;
	va_list varList;
	Node *child;

	father->childNum += n;
	va_start(varList, n);
	for (i = 0; i < n; i++)
	{
		child = va_arg(varList, Node *);

		if (father->child == NULL)
			father->child = child;
		else
		{
			Node *tail = father->child;

			while (tail->sibling != NULL)
				tail = tail->sibling;

			tail->sibling = child;
		}
	}
	va_end(varList);
}

void freeTree(Node *tree)
{
	Node *child = tree->child;
	Node *sibling = tree->sibling;

	if(!strcmp(tree->type, "ID"))
		free(tree->data.id);
	if (!strcmp(tree->type, "TYPE"))
		free(tree->data.type);
	free(tree);
	if (child != NULL)
		freeTree(child);
	if (sibling != NULL)
		freeTree(sibling);
}