#include "connection_manager.h"

// #define _IP_PORT_PTR(x) ((_Ip_port*)x)




// static Hash_table _ipport2list;
// static int _ip_port_cmp(void *lsh, void *rsh);
static void _single_close(int fd, int epollfd);
static void _client_close(int fd, int epollfd);
static void _server_close(int fd, int epollfd);
// typedef struct _ip_port{
//     unsigned int ip;
//     unsigned short port;
// }_Ip_port;
// typedef struct _list_node{
//     Listnode tag;
//     int fd;
// }_List_node;

// #ifdef _LOG
// static void _pri(Packet *packet, int type){
//     int i = packet->buf[type].l;
//     while(i != packet->buf[type].r){
//         printf(RED"%c"COLOR_END, packet->buf[type].buf[i]);
//         BUF_ADD_INDEX(&packet->buf[type], i);
//     }
// }
// #endif

// static int _ip_port_cmp(void *lsh, void *rsh){
//     return _IP_PORT_PTR(lsh)->ip == _IP_PORT_PTR(rsh)->ip 
//         && _IP_PORT_PTR(lsh)->port == _IP_PORT_PTR(rsh)->port;
// }

// static _List_node* _get_fd_from_ip_port(_Ip_port *ip_port){
//     List *tmp = (List *)lookup(&_ipport2list, ip_port);
//     if(tmp == NULL || tmp->head == NULL)
//         return NULL;
//     // if(tmp->head != NULL)
//     Listnode *fd = list_pop_front(tmp);
//     return (_List_node*) fd;
    
// }


int connection_create(int fd){
    LOG("connection_create\n");
    Packet *packet = (Packet*)get_packet_ptr(fd);
    // _Ip_port tmp;
    // memset(&tmp, 0, sizeof(tmp));
    // tmp.ip = packet->info.ip;
    // tmp.port = packet->info.port;
    // _List_node *ptr = NULL;
    int serverfd = get_pool(packet->info.ip, packet->info.port);
    // while((ptr = _get_fd_from_ip_port(&tmp))){
        
    //     LOG("im get from pool\n");
    //     serverfd = ptr->fd;
    //     void *ipport_ptr = get_ipport_ptr(ptr->fd);
    //     free(ipport_ptr);
    //     del_ipport_ptr(ptr->fd);
    //     free(ptr);
    //     if(test_connection(serverfd)){
    //         del_fd_type(serverfd);
    //         break;
    //     }
    //     COLOR_LOG(YELLOW, "%d fd is closed!\n", serverfd);
    //     close(serverfd);
    // }
    if(serverfd == -1){
        serverfd = connect2server(packet->info.ip, packet->info.port);
        if(serverfd < 0){
            #ifdef _LOG
            struct in_addr addr;
            addr.s_addr = packet->info.ip;
            COLOR_LOG(RED,"dest ip = %s, port = %d\n", inet_ntoa(addr), ntohs(packet->info.port));
            // _pri(packet, get_fd_type(fd) & SERVER);
            #endif
            return serverfd;
        }
    }
    


    LOG("src fd = %d, dest fd = %d\n", fd, serverfd);
    #ifdef _LOG
    struct in_addr addr;
    addr.s_addr = packet->info.ip;
    LOG("dest ip = %s, port = %d\n", inet_ntoa(addr), ntohs(packet->info.port)); 
    #endif
    assert(get_fd(fd) == -1);
    assert(get_fd(serverfd) == -1);
    set_fd(fd, serverfd);
    set_fd(serverfd, fd);
    return serverfd;
}

void server_close(int fd, int epollfd){
    LOG("server_close begin\n");
    int _fd = get_fd(fd);
    if(_fd >= 0){
        assert(get_fd(_fd) == fd);
        del_fd(fd);
        del_fd(_fd);
    }
    _server_close(fd, epollfd);
}
static void _server_close(int fd, int epollfd){
    LOG("server_close %d\n", fd);
    void *ptr = get_packet_ptr(fd);
    if(ptr) {
        del_packet_ptr(fd);  //_fd2packet
        packet_destory(ptr,  SERVER);
    }
    if(get_fd_type(fd) & IN_POOL){
        del_pool(fd);
    }
    if((get_fd_type(fd) & IN_EPOLL) == IN_EPOLL){
        int ret = epoll_del(epollfd, fd, EPOLLIN);
        assert(!ret);
    }
    del_fd_type(fd);
    assert(fd >= 0);
    close(fd); //del fd;
}



static void _single_close(int fd, int epollfd){
    if(get_fd_type(fd) == -1){
        COLOR_LOG(RED, "single_close get_fd_type == -1 %d\n", fd);
        return ;
    }
    if((get_fd_type(fd) & SERVER )== SERVER){
        _server_close(fd, epollfd);
    }
    else{
        _client_close(fd, epollfd);
    }
}


static void _client_close(int fd, int epollfd){
    LOG("client_close %d\n", fd);
    void *ptr = get_packet_ptr(fd);
    if(ptr) {
        del_packet_ptr(fd);  //_fd2packet
        packet_destory(ptr, CLIENT);
    }
    if((get_fd_type(fd) & IN_EPOLL) == IN_EPOLL){
        int ret = epoll_del(epollfd, fd, EPOLLIN);
        assert(!ret);
    }
    del_fd_type(fd);
    assert(fd >= 0);
    close(fd);
}


void connection_close(int fd, int epollfd){
    if(get_fd(fd) >= 0){
        int _fd = get_fd(fd);
        assert(get_fd(_fd) == fd);
        _single_close(get_fd(fd), epollfd);
        del_fd(fd);
        del_fd(_fd);
    }
    assert(fd >= 0);
    _single_close(fd, epollfd);
    
}



int connection_release(int serverfd){
    LOG("connection_release %d\n", serverfd);
    int fd = get_fd(serverfd);
    assert(fd >= 0 && get_fd(fd) == serverfd);
    del_fd(serverfd);
    del_fd(fd);
    Packet *packet = (Packet *)get_packet_ptr(serverfd);
    insert_pool(packet->info.ip, packet->info.port, serverfd);
    // _Ip_port tmp_ipport;
    // assert(packet);
    // memset(&tmp_ipport, 0, sizeof(tmp_ipport));
    // tmp_ipport.ip = packet->info.ip;
    // tmp_ipport.port = packet->info.port;
    // List *list = (List*) lookup(&_ipport2list, &tmp_ipport);
    // if(!list){
    //     List tmp_list;
    //     list_init(&tmp_list);
    //     insert(&_ipport2list, &tmp_ipport, &tmp_list);
    //     list = (List*) lookup(&_ipport2list, &tmp_ipport);
    // }
    // assert(list);
    // _List_node *tmp_ptr = (_List_node*)malloc(sizeof(_List_node));
    // assert(tmp_ptr);
    // tmp_ptr->fd = serverfd;
    // list_push_back(list, (Listnode *)tmp_ptr);
    // assert(get_ipport_ptr(serverfd) == NULL);
    // _Ip_port * ipport_ptr = (_Ip_port*)malloc(sizeof(_Ip_port));
    // assert(ipport_ptr);
    // memcpy(ipport_ptr, &tmp_ipport, sizeof(tmp_ipport));
    // set_ipport_ptr(serverfd, ipport_ptr);
    packet_destory(packet, SERVER);
    del_packet_ptr(serverfd);
    set_fd_type(serverfd, IN_EPOLL | IN_POOL | SERVER);
    // del_fd_type(serverfd);
    return 0;
}



