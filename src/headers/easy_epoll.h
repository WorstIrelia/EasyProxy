#ifndef _EASY_EPOLL_H_
#define _EASY_EPOLL_H_
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <string.h>
int epoll_add(int epollfd, int fd, int events);

int epoll_del(int epollfd, int fd, int events);

int epoll_mod(int epollfd, int fd, int events);
#endif