#ifndef _NETLIB_H_
#define _NETLIB_H_

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

int hostname2ip(char * hostname);
int connect2server(unsigned int ip, short port);
#endif