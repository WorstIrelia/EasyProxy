#include "hash_table.h"

typedef struct pair{
    void *key;
    void *value;
}Pair;

typedef struct hash_table_node{
    Listnode node;
    Pair key_value;
}Hash_table_node;

static int prime[] = {3,7,17,37,79,163,331,673,1361,2729,5471,10949,21911,43853,87719,175447,350899,701819};

static Hash_table_node* hash_table_node_init(void *key,unsigned int key_n, void *value, unsigned int value_n);
static void hash_table_node_destory(Hash_table_node *ptr);
static void _insert(Hash_table *hash, void *ptr);
static Hash_table_node *_lookup(Hash_table *hash, void *key);
static void rehash(Hash_table *hash);

void hash_table_init(Hash_table *hash,unsigned int (*fun_ptr)(void *, int, unsigned int), int (*cmp_key)(void *lsh, void *rsh), unsigned int key_n, unsigned int value_n){
    hash->cap = prime[0];
    hash->size = 0;
    hash->key_n = key_n;
    hash->value_n = value_n;
    hash->list = (List*)malloc(hash->cap * sizeof(List));
    assert(hash->list);
    for(int i = 0; i < hash->cap; i++){
        list_init(hash->list + i);
    }
    hash->fun_ptr = fun_ptr;
    hash->cmp_key = cmp_key;
}

void hash_table_destory(Hash_table *hash){
    for(int i = 0; i < hash->cap; i++){
        Listnode *tmp = hash->list[i].head;
        while(tmp){
            Hash_table_node* _tmp = (Hash_table_node*)tmp;
            tmp = tmp->next;
            hash_table_node_destory(_tmp);
            
        }
    }
    free(hash->list);
}


static Hash_table_node* hash_table_node_init(void *key, unsigned int key_n, void *value, unsigned int value_n){
    Hash_table_node* ptr = (Hash_table_node*)malloc(sizeof(Hash_table_node));
    assert(ptr);
    ptr->key_value.key = malloc(key_n);
    ptr->key_value.value = malloc(value_n);
    assert(ptr->key_value.key);
    assert(ptr->key_value.value);
    memcpy(ptr->key_value.key, key, key_n);
    memcpy(ptr->key_value.value, value, value_n);
    
    return ptr;
}

static void hash_table_node_destory(Hash_table_node *ptr){
    free(ptr->key_value.key);
    free(ptr->key_value.value);
    free(ptr);
}

static void rehash(Hash_table *hash){
    unsigned int cnt = sizeof(prime) / sizeof(prime[0]);
    unsigned int new_cap;
    if(hash->cap == prime[cnt - 1]) return ;
    for(int i = 0; i < cnt; i++){
        if(prime[i] == hash->cap){
            new_cap = prime[i + 1];
            break;
        }
    }
    List *list = (List*)malloc(sizeof(List) * new_cap);
    assert(list);
    for(int i = 0; i < new_cap; i++){
        list_init(list + i);
    }
    for(int i = 0; i < hash->cap; i++){
        List *ptr = hash->list + i;
        while(ptr->head){
            Hash_table_node *tmp = (Hash_table_node *)(list_pop_front(ptr));
            unsigned int index = hash->fun_ptr(tmp->key_value.key, hash->key_n, new_cap);
            list_push_back(list + index, (Listnode*)tmp);
        }
    }
    free(hash->list);
    hash->list = list;
    hash->cap = new_cap;
}

static void _insert(Hash_table *hash, void *ptr){
    Hash_table_node *tmp = (Hash_table_node*)ptr;
    hash->size++;
    if(hash->size > hash->cap){
        rehash(hash);
    }
    int index = hash->fun_ptr(tmp->key_value.key, hash->key_n, hash->cap);
    List *list_ptr = hash->list + index;
    list_push_back(list_ptr, (Listnode*)ptr);
}


static Hash_table_node *_lookup(Hash_table *hash, void *key){
    int index = hash->fun_ptr(key, hash->key_n, hash->cap);
    List *list = hash->list + index;
    Listnode *tmp = list->head;
    while(tmp){
        if(hash->cmp_key(((Hash_table_node*)tmp)->key_value.key, key))
            return (Hash_table_node*)tmp;
        tmp = tmp->next;
    }
    // puts("");
    return NULL;
}





unsigned int  base_hash(void *key, int n, unsigned int mod){
    char *ptr = (char *)key;
    unsigned int res = 0;
    unsigned int x = 0;
    for(int i = 0; i < n; i++, ptr++){
        res = (res << 4) + *ptr ;
        if((x = res & 0xf0000000)){
            res ^= (res >> 24);
            res &= ~x;
        }
    }
    return res % mod;   
}
void hash_table_pri(Hash_table *hash, void (*pri)(void *key, void *value)){
    for(int i = 0; i < hash->cap; i++){
        Listnode *ptr = hash->list[i].head;
        Hash_table_node *tmp = (Hash_table_node *)ptr;
        while(ptr){
            pri(tmp->key_value.key, tmp->key_value.value);
            ptr = ptr->next;
        }
    }
}

void insert(Hash_table *hash, void *key, void *value){
    if(_lookup(hash, key)) {
        return ;
    }
    _insert(hash, hash_table_node_init(key, hash->key_n, value, hash->value_n));
}
const void *lookup(Hash_table *hash, void *key){
    Hash_table_node * ptr= _lookup(hash, key);
    if(ptr == NULL) return ptr;
    return (const void *)ptr->key_value.value;

}
void del(Hash_table *hash, void *key){
    Hash_table_node *ptr = _lookup(hash, key);
    if(ptr == NULL){
        return ;
    }
    hash->size--;
    int index = hash->fun_ptr(key, hash->key_n, hash->cap);
    list_erase(hash->list + index, (Listnode*) ptr);
    hash_table_node_destory(ptr);
}
void change(Hash_table *hash, void *key, void *value){
    Hash_table_node *ptr = _lookup(hash, key);
    if(ptr == NULL){
        insert(hash, key, value);
        return;
    }
    else{
        void *tmp = malloc(hash->value_n);
        memcpy(tmp, value, hash->value_n);
        free(ptr->key_value.value);
        ptr->key_value.value = tmp;
    }
}