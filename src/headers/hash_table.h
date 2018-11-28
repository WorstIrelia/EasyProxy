#ifndef _HASH_TABLE_
#define _HASH_TABLE_
#include "list.h"
#include <stdio.h>
#include <stdlib.h>




typedef struct hash_table{
    unsigned int cap;
    unsigned int size;
    unsigned int key_n;
    unsigned int value_n;
    List *list;
    unsigned int (*fun_ptr)(void *, int, unsigned int);
    int (*cmp_key)(void *lsh, void *rsh);
}Hash_table;

typedef struct pair Pair;
typedef struct hash_table_node Hash_table_node;

void hash_table_init(Hash_table *hash,\
                    unsigned int (*fun_ptr)(void *, int, unsigned int), \
                    int (*cmp_key)(void *lsh, void *rsh), \
                    unsigned int key_n, \
                    unsigned int value_n);

                    
void hash_table_destory(Hash_table *hash);

unsigned int  base_hash(void *key, int n, unsigned int mod);

void insert(Hash_table *hash, void *key, void *value);

const void *lookup(Hash_table *hash, void *key);

void del(Hash_table *hash, void *key);

void change(Hash_table *hash, void *key, void *value);


#endif