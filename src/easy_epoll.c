#include "easy_epoll.h"

int epoll_add(int epollfd, int fd, int events){
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = events;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0){
        return -1;
    }
    return 0;
}

int epoll_del(int epollfd, int fd, int events){
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = events;    
    if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) < 0){
        return -1;
    }
    return 0;
}

int epoll_mod(int epollfd, int fd, int events){
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = events;
    if(epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) < 0){
        return -1;
    }
    return 0;
}
