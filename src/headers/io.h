#ifndef _IO_H_
#define _IO_H_


#include "packet.h"
#include "fd_manager.h"
#include "config.h"
#include "auto_match.h"
#include "connection_manager.h"


int read_packet(int fd, int epollfd);
int send_packet(int fd, int epoolfd);


#endif