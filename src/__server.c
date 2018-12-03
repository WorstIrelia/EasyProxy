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
#include "connection_manager.h"
#include "io.h"
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
    LOG("accept fd = %d\n", retfd);
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
        // COLOR_LOG(PURPLE, "deal begin\n");
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
                // if(events[i].events & EPOLLERR){
                //     COLOR_LOG(RED, "connection_close ERROR %d\n", tmpfd);
                //     #ifdef _LOG
                //     int status;
                //     socklen_t slen;
                //     getsockopt(tmpfd, SOL_SOCKET, SO_ERROR, (void *) &status, &slen);
                //     COLOR_LOG(RED, "errno = %d\n", status);
                //     #endif
                //     connection_close(tmpfd, epollfd);
                //     continue;
                // } 
                if(events[i].events & EPOLLIN){
                    // LOG("read fd %d = tmpfd\n", tmpfd);
                    n = read_packet(tmpfd, epollfd);
                    if(n < 0){
                        COLOR_LOG(RED, "connection_close READ %d\n", tmpfd);
                        connection_close(tmpfd, epollfd);
                    }
                }
                if(events[i].events & EPOLLOUT){
                    // LOG("send fd %d = tmpfd\n", tmpfd);
                    n = send_packet(tmpfd, epollfd);
                    if(n < 0){
                        
                        COLOR_LOG(RED, "connection_close WRITE %d\n", tmpfd);
                        connection_close(tmpfd, epollfd);
                    }
                }
                   
            }
            
        }
        // COLOR_LOG(PURPLE, "deal down\n");
    }


}