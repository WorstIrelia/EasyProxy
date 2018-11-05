#include "list.h"
void list_init(List *list){
    list->head = list->tail = NULL;
}

void list_push_back(List *list, Listnode *node){
    if(list->tail == NULL){
        list->head = list->tail = node;
        node->prev = node->next = NULL;
        return;
    }
    list->tail->next = node;
    node->prev = list->tail;
    node->next = NULL;
    list->tail = node;
}

void list_push_front(List *list, Listnode *node){
    if(list->head == NULL){
        list->head = list->tail = node;
        node->prev = node->next = NULL;
        return;
    }
    list->head->prev = node;
    node->next = list->head;
    node->prev = NULL;
    list->head = list->head->prev;
}

Listnode* list_pop_back(List *list){
    assert(list->tail != NULL);
    Listnode *res = list->tail;
    list->tail = list->tail->prev;
    if(list->tail){
        list->tail->next = NULL;
    }
    else list->head = NULL;
    return res;
}

Listnode* list_pop_front(List *list){
    assert(list->head != NULL);
    Listnode *res = list->head;
    list->head = list->head->next;
    if(list->head){
        list->head->prev = NULL;
    }
    else list->tail = NULL;
    return res;
    
}

void list_erase(List *list, Listnode *ptr){
    if(list->head == ptr){
        list_pop_front(list);
    }
    else if(list->tail == ptr){
        list_pop_back(list);
    }
    else{
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
    }
}

void list_pri(List *list, void (*func)(Listnode *)){
    Listnode *ptr = list->head;
    while(ptr){
        func(ptr);
        ptr = ptr->next;
    }
    return;
}