#ifndef _NETLIB_H_
#define _NETLIB_H_

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int hostname2ip(char * hostname);
int connect2server(unsigned int ip, unsigned short port);
int listen_port(unsigned short port);
#endif