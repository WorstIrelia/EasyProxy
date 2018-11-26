#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/io.h>
#include "packet.h"
#include "fd_manager.h"
#include "easy_epoll.h"
static struct epoll_event events[BUFSIZE];

extern Proxy_type proxy_type;
extern unsigned short listenport;
extern char *listenip;
extern unsigned short proxyport;
extern char *proxyip;
extern int init(int ,char *[]);

static int Accept(int fd, int epollfd){
    int retfd = accept(fd, NULL, NULL);
    if(retfd < 0){
        return retfd;
    }
    epoll_add(epollfd, retfd, EPOLLIN);
    assert(get_fd_type(retfd) == -1);
    set_fd_type(retfd, CLIENT | IN_EPOLL);
    if(proxy_type == PROXY2PROXY || proxy_type == PROXY2SERVER){
        set_fd_type(retfd, get_fd_type(retfd) | CODE);
    }
    assert(get_fd_type(retfd) != -1);
    return 0;
}

int main(int argc, char *argv[]){
    if(init(argc, argv)){
        puts("init error\n");
        return -1;
    }
    int http_fd = Listen(listenip, listenport);
    // for(int i = 0; i < 6; i++){
    //     if(!fork())
    //         break;
    // }
    assert(http_fd >= 0);
    int epollfd = epoll_create(BUFSIZE);
    assert(epollfd >= 0);
    epoll_add(epollfd, http_fd, EPOLLIN);
    
    for(;;){
        int num = epoll_wait(epollfd, events, BUFSIZE, -1);
        for(int i = 0; i < num; i++){
            int tmpfd = events[i].data.fd;
            int n;
            if(tmpfd == http_fd){
                if(Accept(tmpfd, epollfd) < 0){
                    // continue;
                    return -1;
                }
            }
            else {
                if(events[i].events & EPOLLERR){
                    connection_close(tmpfd, epollfd);
                    continue;
                } 
                if(events[i].events & EPOLLIN){
                    n = read_packet(tmpfd, epollfd);
                    if(n <= 0){
                        connection_close(tmpfd, epollfd);
                    }
                }
                if(events[i].events & EPOLLOUT){
                    n = send_packet(tmpfd, epollfd);
                    if(n < 0){
                        connection_close(tmpfd, epollfd);
                    }
                }
                   
            }
            
        }
    }


}