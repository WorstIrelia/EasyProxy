#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdio.h>
#include <stdlib.h>
#include "fd_manager.h"
#include "config.h"
#include "easy_epoll.h"

typedef enum head_body{
    REQUEST_HEAD,
    REQUEST_BODY,
    DATA_BODY,
    HTTPS
}Head_body;

typedef enum complete{
    COMPLETE,
    NOT_COMPLETE
}Complete;

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
    char *buf;
    size_t cap;
    size_t size;
    int l,r;
    int state;
    Fd_type client_server_flag;
    Head_body buf_type;
    Info info;
    Complete com_flag;
}Packet;

#include "auto_match.h"

int read_packet(int fd, int epollfd);
int send_packet(int fd, int epollfd);
void packet_init(Packet *packet, Fd_type flag);
void packet_destory(void *packet);
void info_init(Info *ptr);

#endif