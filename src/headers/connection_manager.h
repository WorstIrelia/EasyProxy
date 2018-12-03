#ifndef _CONNCECTION_MANAGER_H_
#define _CONNCECTION_MANAGER_H_

#include "fd_manager.h"
#include "packet.h"
#include "config.h"
#include "netlib.h"
#include "connection_pool.h"
int connection_create(int fd);
int connection_release(int serverfd);
void connection_close(int fd, int epollfd);
void server_close(int fd, int epollfd);

#endif