#include "connection_pool.h"
#define _IP_PORT_PTR(x) ((_Ip_port*)x)
#define INIT if(!init_flag){hash_table_init(&_ipport2list, base_hash, _ip_port_cmp, sizeof(_Ip_port), sizeof(List));init_flag = 1;}
static int init_flag = 0;
static Hash_table _ipport2list;
static int _ip_port_cmp(void *lsh, void *rsh);
typedef struct _ip_port{
    unsigned int ip;
    unsigned short port;
}_Ip_port;
typedef struct _list_node{
    Listnode tag;
    int fd;
}_List_node;

// #ifdef _LOG
// static void _pri(Packet *packet, int type){
//     int i = packet->buf[type].l;
//     while(i != packet->buf[type].r){
//         printf(RED"%c"COLOR_END, packet->buf[type].buf[i]);
//         BUF_ADD_INDEX(&packet->buf[type], i);
//     }
// }
// #endif

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




void insert_pool(unsigned int ip, short port, int fd){
    INIT;
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
    tmp_ptr->fd = fd;
    list_push_back(list, (Listnode *)tmp_ptr);
    assert(get_ipport_ptr(fd) == NULL);
    _Ip_port * ipport_ptr = (_Ip_port*)malloc(sizeof(_Ip_port));
    assert(ipport_ptr);
    memcpy(ipport_ptr, &tmp_ipport, sizeof(tmp_ipport));
    set_ipport_ptr(fd, ipport_ptr);
    return ;
}
int get_pool(unsigned int ip, unsigned short port){
    INIT;
    _Ip_port tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.ip = ip;
    tmp.port = port;
    _List_node *ptr = NULL;
    int serverfd;
    while((ptr = _get_fd_from_ip_port(&tmp))){
        LOG("im get from pool\n");
        serverfd = ptr->fd;
        assert(get_fd_type(ptr->fd) & IN_POOL);
        void *ipport_ptr = get_ipport_ptr(ptr->fd);
        free(ipport_ptr);
        del_ipport_ptr(ptr->fd);
        free(ptr);
        if(test_connection(serverfd)){
            break;
        }
        COLOR_LOG(YELLOW, "%d fd is closed!\n", serverfd);
        close(serverfd);
    }
    if(ptr == NULL) return -1;
    return serverfd;
}
void del_pool(int fd){
    INIT;
    _List_node* ptr = get_ipport_ptr(fd);
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
};