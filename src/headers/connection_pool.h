#ifndef _CONNECTION_POOL_H_
#define _CONNECTION_POOL_H_

#include "hash_table.h"
#include "config.h"
#include "fd_manager.h"
#include "http.h"
#include "easy_epoll.h"

void insert_pool(unsigned int ip, short port, int fd);
int get_pool(unsigned int fd, unsigned short port);
void del_pool(int fd);

#endif