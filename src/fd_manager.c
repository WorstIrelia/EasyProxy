#include "fd_manager.h"

#define _IP_PORT_PTR(x) ((_Ip_port*)x)
#define SERVER 1
static Hash_table _fd2fd;
static Hash_table _ipport2list;
static Hash_table _fd2packet_ptr;
static Hash_table _fd2ipport_ptr;
static Hash_table _fd2type;


typedef struct _ip_port{
    unsigned int ip;
    unsigned short port;
}_Ip_port;
typedef struct _list_node{
    Listnode tag;
    int fd;
}_List_node;


static _List_node* _get_fd_from_ip_port(_Ip_port *ip_port);
static int _ip_port_cmp(void *lsh, void *rsh);
static int _fd_cmp(void *lsh, void *rsh);
static _List_node* _get_fd_from_ip_port(_Ip_port *ip_port);
static void _server_destory(int fd, int epoll_fd);
static void _client_destory(int fd, int epoll_fd);
static void _single_close(int fd, int epoll_fd);
static void _connection_close(int client_fd, int server_fd, int epoll_fd);

static int _fd_cmp(void *lsh, void *rsh){
    return *(int*)lsh == *(int*)rsh;
}
static int _ip_port_cmp(void *lsh, void *rsh){
    return _IP_PORT_PTR(lsh)->ip == _IP_PORT_PTR(rsh)->ip \
        && _IP_PORT_PTR(lsh)->port == _IP_PORT_PTR(rsh)->port;

}

static _List_node* _get_fd_from_ip_port(_Ip_port *ip_port){
    List *tmp = (List *)lookup(&_ipport2list, ip_port);
    if(tmp == NULL || tmp->head == NULL)
        return NULL;
    // if(tmp->head != NULL)
    Listnode *fd = list_pop_front(tmp);
    return (_List_node*) fd;
    
}

static void _client_destory(int fd, int epollfd){
    assert((get_fd_type(fd) & 0x1) == 0);
    void *ptr = get_packet_ptr(fd);
    if(ptr) {
        del_packet_ptr(fd);  //_fd2packet
        packet_destory(ptr);
        free(ptr);
    }
    if((get_fd_type(fd) & 0x2) == IN_EPOLL){
        assert(!epoll_del(epollfd, fd, EPOLLIN | EPOLLERR));
    }
    del_fd_type(fd);
    assert(fd >= 0);
    close(fd);
}


static void _single_close(int fd, int epoll_fd){
    assert(get_fd_type(fd) != -1);
    int type = get_fd_type(fd) & 0x1;
    if(type == SERVER){
        _server_destory(fd, epoll_fd);
    }
    else{
        _client_destory(fd, epoll_fd);
    }
}
static void _connection_close(int client_fd, int server_fd, int epoll_fd){
    del(&_fd2fd, &client_fd);
    del(&_fd2fd, &server_fd);
    assert(get_ipport_ptr(server_fd) == NULL);
    _single_close(client_fd, epoll_fd);
    _single_close(server_fd, epoll_fd);
}


static void _server_destory(int fd, int epollfd){
    
    void *ptr = get_packet_ptr(fd);
    if(ptr) {
        del_packet_ptr(fd);  //_fd2packet
        packet_destory(ptr);
        free(ptr);//?????
    }
    ptr = get_ipport_ptr(fd);
    if(ptr){
        List *list = (List*)lookup(&_ipport2list, ptr);
        Listnode* list_node_ptr = list->head;
        while(list_node_ptr){
            if(((_List_node*)list_node_ptr)->fd == fd){
                list_erase(list, list_node_ptr);
                break;
            }
            list_node_ptr = list_node_ptr->next;
        }
        assert(list_node_ptr);
        free(list_node_ptr);//del _ipport2list
        del_ipport_ptr(fd);
        free(ptr);
    }
    if((get_fd_type(fd) & 0x2) == IN_EPOLL){
        assert(!epoll_del(epollfd, fd, EPOLLIN | EPOLLERR));
    }
    del_fd_type(fd);
    assert(fd >= 0);
    close(fd); //del fd;
    
}

void fd_manager_init(){
    hash_table_init(&_fd2fd, base_hash, _fd_cmp, sizeof(int), sizeof(int));
    hash_table_init(&_fd2type, base_hash, _fd_cmp, sizeof(int), sizeof(int));
    hash_table_init(&_ipport2list, base_hash, _ip_port_cmp, sizeof(_Ip_port), sizeof(List));
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
int get_fd_type(int fd){
    void *ptr = (void *)lookup(&_fd2type, &fd);
    if(ptr == NULL){
        return -1;
    }
    return *(int*)ptr;

}
void set_fd_type(int fd, int type){
    int on = type;
    // assert(get_fd_type(fd) == -1);
    change(&_fd2type, &fd, &on);
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





void connection_close(int fd, int epoll_fd){
    assert(fd >= 0);
    if(get_fd_type(fd) == -1){
        return ;
    }
    int match_fd = get_fd(fd);
    
    if(match_fd == -1){
        _single_close(fd, epoll_fd);
    }
    else{
        if((get_fd_type(fd) & 0x1)== SERVER){
            int tmp = match_fd;
            match_fd = fd;
            fd = tmp;
            // match_fd ^= fd ^= match_fd ^= fd;//swap
        }
        _connection_close(fd, match_fd, epoll_fd);
    }
}


int connection_create(int fd, unsigned int ip, unsigned short port){
    _Ip_port tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.ip = ip;
    tmp.port = port;
    _List_node *ptr = _get_fd_from_ip_port(&tmp);
    int serverfd;
    if(ptr == NULL){
        serverfd = connect2server(ip, port);
        if(serverfd < 0){
            return serverfd;
        }
    }
    else{
        serverfd = ptr->fd;
        void *ipport_ptr = get_ipport_ptr(ptr->fd);
        free(ipport_ptr);
        del_ipport_ptr(ptr->fd);
        free(ptr);
    }    
    assert(lookup(&_fd2fd, &fd) == NULL);
    assert(lookup(&_fd2fd, &serverfd) == NULL);
    insert(&_fd2fd, &fd, &serverfd);
    insert(&_fd2fd, &serverfd, &fd);
    return serverfd;
}

int connection_release(int serverfd, unsigned int ip, unsigned short port){
    int fd = get_fd(serverfd);
    assert(fd >= 0 && get_fd(fd) == serverfd);
    del(&_fd2fd, &serverfd);
    del(&_fd2fd, &fd);
    _Ip_port tmp_ipport;
    memset(&tmp_ipport, 0, sizeof(tmp_ipport));
    tmp_ipport.ip = ip;
    tmp_ipport.port = port;
    List *list = (List*) lookup(&_ipport2list, &tmp_ipport);
    if(!list){
        List tmp_list;
        list_init(&tmp_list);
        insert(&_ipport2list, &tmp_ipport, &tmp_list);
        list = (List*) lookup(&_ipport2list, &tmp_ipport);
    }
    assert(list);
    _List_node *tmp_ptr = (_List_node*)malloc(sizeof(_List_node));
    assert(tmp_ptr);
    tmp_ptr->fd = serverfd;
    list_push_back(list, (Listnode *)tmp_ptr);
    assert(get_ipport_ptr(serverfd) == NULL);
    _Ip_port * ipport_ptr = (_Ip_port*)malloc(sizeof(_Ip_port));
    assert(ipport_ptr);
    memcpy(ipport_ptr, &tmp_ipport, sizeof(tmp_ipport));
    insert(&_fd2ipport_ptr, &serverfd, &ipport_ptr);
    del_fd_type(serverfd);
    return 0;
}



