#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdio.h>
#include <stdlib.h>
#include "fd_manager.h"
#include "config.h"
#include "easy_epoll.h"
#include "http.h"
#include "buf.h"

// typedef enum client_server_flag{
//     CLIENT,
//     SERVER
// }Client_server_flag;

typedef enum packet_type{
    UNKNOW,
    GET,
    POST,
    CONNECT
}Packet_type;



typedef struct info{
    unsigned int ip;
    unsigned short port;
    Packet_type packet_type;
    size_t length;
    size_t chunked_size;
}Info;//先实现 在考虑层次


typedef struct packet{
    Buf buf[2];
    int state;
    int refcnt;
    Info info;
    Head_body buf_type;
    Complete com_flag;
    Connection connection_state;//no use
    Packet_kind packet_kind;
    int now_use;
}Packet;

#include "auto_match.h"
Packet* packet_init();
void packet_destory(void *ptr, int type);
void packet_reinit(Packet *packet);
void packet_request(Packet *packet);
void packet_response(Packet *packet);
#endif