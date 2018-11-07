#ifndef _FD_MANAGER_H_
#define _FD_MANAGER_H_
#include <sys/epoll.h>
#include <assert.h>
#include "hash_table.h"
#include "netlib.h"

#include <unistd.h>
#include <fcntl.h>
#include "easy_epoll.h"

typedef enum fd_type{
    CLIENT =        0X00,
    SERVER =        0X01,
    IN_EPOLL =      0X02,
    NOT_IN_EPOLL =  0X00
}Fd_type;

void fd_manager_init();

int connection_create(int fd, unsigned int ip, unsigned short port);
//建立一个clientfd -> serverfd的映射(fd2fd)
//如果空闲hash链表里有，那么从里面取得fd，同时删除fd->ipport的映射和ipport->fd的这个链表元素(释放)
//没有，直接建立一个链接
int connection_release(int serverfd, unsigned int ip, unsigned short port);
//释放一个已经结束的serverfd
//删除fd2fd的映射
//插入空闲链表，更新fd->ipport, iport->fd;
void connection_close(int fd, int epoll_fd);
//出错或者结束，释放一条相关的链接或者单边链接

int get_fd(int fd);
int get_fd_type(int fd);



void set_fd_type(int fd, int type);
void del_fd_type(int fd);
void *get_packet_ptr(int fd);
void set_packet_ptr(int fd, void *ptr);
void del_packet_ptr(int fd);

void *get_ipport_ptr(int fd);
void set_ipport_ptr(int fd, void *ptr);
void del_ipport_ptr(int fd);


#endif
