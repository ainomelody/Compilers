#ifndef _TREE_H

#define _TREE_H

typedef struct Node{
	int lineNum;
	char type[20];
	struct Node *child, *sibling;
    int childNum;
	union{
		char *id;
		char *type;
		int intValue;
		float floatValue;
	}data;
}Node;

Node *newNode(char *type, int lineNum);
void printTree(Node *tree, int level);
void addChild(Node *father, int n, ...);
void freeTree(Node *tree);

typedef Node * YYVALTYPE;
#endif


