#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define N 3

typedef struct node_tag {
    int puzzle[N][N];
    int x, y;
    int depth;			
    struct node_tag *next;	
    struct node_tag *parent;
    struct node_tag *children[4];
    int num_children;
} node;

node * create_node(int puzzle[N][N], int x, int y, int newX, int newY, int depth, node *parent) 
{
    node * p = (node*)malloc(sizeof(node));
    assert(p != NULL);
    memcpy(p->puzzle, puzzle, sizeof(p->puzzle));
    p->puzzle[x][y] = p->puzzle[newX][newY];
    p->puzzle[newX][newY] = 0;
    p->x = newX;
    p->y = newY;
    p->depth = depth;
    p->parent = parent;
    p->next = NULL;
    p->num_children = 0; 
    for (int i = 0; i < 4; i++) { 
        p->children[i] = NULL; 
        }
    return p;			
}

//add newnode the last of the linked list determined by *head and *tail
//note **head, **tail
void add_last(node **head, node **tail, node *newnode)
{
	if((*head) == NULL)
	{
		(*head) = (*tail) = newnode;
	}
	else
	{
		(*tail)->next = newnode;
		(*tail) = newnode;
	}
}

//remove the first node from the list
//note **head

node * remove_first(node **head, node **tail) 
{
	node *p;

	p = (*head);
	if((*head)!=NULL) (*head) = (*head)->next;
	if((*tail) == p) (*tail) = NULL; 
	return p;
}


//add_first() should add to the beginning of a linked list
//note the type: 'node **head'
//note that it does not return a value 
void add_first(node **head, node *newnode)
{
    if((*head) == NULL)
    {
        (*head) = newnode;
    }
    else
    {
        newnode->next = (*head);
        (*head) = newnode;
    }
}

void free_all(node **head, node **tail)
{
        while((*head)!=NULL) free(remove_first(head, tail));
}


node * remove_first_2(node **head) 
{
    node *first_node = (*head);
    if ((*head) != NULL){
        (*head) = (*head)->next;
    }
    return first_node;
}


void remove_all(node **head)
{
    node *current_node;
    while((*head) != NULL){
        current_node = remove_first_2(head);
        if (current_node != NULL){
            free(current_node);
        }
    }
}