#ifndef _CONNCECTION_MANAGER_H_
#define _CONNCECTION_MANAGER_H_

#include "fd_manager.h"
#include "packet.h"
#include "config.h"
#include "netlib.h"
int connection_create(int fd);
//建立一个clientfd -> serverfd的映射(fd2fd)
//如果空闲hash链表里有，那么从里面取得fd，同时删除fd->ipport的映射和ipport->fd的这个链表元素(释放)
//没有，直接建立一个链接
int connection_release(int serverfd);
//释放一个已经结束的serverfd
//删除fd2fd的映射
//插入空闲链表，更新fd->ipport, iport->fd;
void connection_close(int fd, int epollfd);
//出错或者结束，释放一条相关的链接或者单边链接
void server_close(int fd, int epollfd);

#endif