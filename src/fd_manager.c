#include "fd_manager.h"


static Hash_table _fd2fd;
static Hash_table _fd2packet_ptr;
static Hash_table _fd2ipport_ptr;
static Hash_table _fd2type;


// typedef struct _ip_port{
//     unsigned int ip;
//     unsigned short port;
// }_Ip_port;
// typedef struct _list_node{
//     Listnode tag;
//     int fd;
// }_List_node;


extern void packet_destory(void *);
// static _List_node* _get_fd_from_ip_port(_Ip_port *ip_port);

static int _fd_cmp(void *lsh, void *rsh);
// static _List_node* _get_fd_from_ip_port(_Ip_port *ip_port);

static int _fd_cmp(void *lsh, void *rsh){
    return *(int*)lsh == *(int*)rsh;
}




void fd_manager_init(){
    hash_table_init(&_fd2fd, base_hash, _fd_cmp, sizeof(int), sizeof(int));
    hash_table_init(&_fd2type, base_hash, _fd_cmp, sizeof(int), sizeof(int));
    //
    hash_table_init(&_fd2packet_ptr, base_hash, _fd_cmp, sizeof(int), sizeof(void *));
    hash_table_init(&_fd2ipport_ptr, base_hash, _fd_cmp, sizeof(int), sizeof(void *));
}


int get_fd(int fd){
    void *ptr = (void *)lookup(&_fd2fd, &fd);
    if(ptr == NULL)
        return -1;
    else 
        return *(int*)ptr;
}
void set_fd(int key_fd, int value_fd){
    change(&_fd2fd, &key_fd, &value_fd);
}
void del_fd(int fd){
    assert(get_fd(fd) >= 0);
    del(&_fd2fd, &fd);
}




int get_fd_type(int fd){
    void *ptr = (void *)lookup(&_fd2type, &fd);
    if(ptr == NULL){
        return -1;
    }
    return *(int*)ptr;

}
void set_fd_type(int fd, int type){
    // assert(get_fd_type(fd) == -1);
    change(&_fd2type, &fd, &type);
}
void del_fd_type(int fd){
    assert(get_fd_type(fd) >= 0);
    del(&_fd2type, &fd);
}



void *get_packet_ptr(int fd){
    void *ptr = (void *)lookup(&_fd2packet_ptr, &fd);
    if(ptr){
        return (void *)*(size_t *)ptr;
    }
    return ptr;
}
void set_packet_ptr(int fd, void *ptr){
    assert(get_packet_ptr(fd) == NULL);
    insert(&_fd2packet_ptr, &fd, &ptr);
}
void del_packet_ptr(int fd){
    assert(get_packet_ptr(fd)!= NULL);
    del(&_fd2packet_ptr, &fd);
}




void *get_ipport_ptr(int fd){
    void *ptr = (void *)lookup(&_fd2ipport_ptr, &fd);
    if(ptr){
        return (void *)*(size_t *)ptr;
    }
    return ptr;
}
void set_ipport_ptr(int fd, void *ptr){
    assert(get_ipport_ptr(fd) == NULL);
    insert(&_fd2ipport_ptr, &fd, &ptr);
}
void del_ipport_ptr(int fd){
    assert(get_ipport_ptr(fd)!= NULL);
    del(&_fd2ipport_ptr, &fd);
}
