#ifndef _LIST_H_
#define _LIST_H_

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
typedef struct list_node{
    struct list_node * prev;
    struct list_node * next;
}Listnode;

typedef struct list{
    Listnode *head;
    Listnode *tail;
}List;


void list_init(List *list);

void list_push_back(List *list, Listnode *node);

void list_push_front(List *list, Listnode *node);

Listnode* list_pop_back(List *list);

Listnode* list_pop_front(List *list);

void list_erase(List *list, Listnode *ptr);

void list_pri(List *list, void (*func)(Listnode *));

#endif