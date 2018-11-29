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
    NOT_IN_EPOLL =  0X00,
    CODE =          0X04,
    NOTCODE =       0X00
}Fd_type;

void fd_manager_init();

int get_fd(int fd);
void set_fd(int key_fd, int value_fd);
void del_fd(int fd);

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
