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
#include "packet.h"
#include "fd_manager.h"
#include "easy_epoll.h"
static struct epoll_event events[BUFSIZE];


static int Accept(int fd, int epollfd){
    // printf("%d\n", fd);
    int retfd = accept(fd, NULL, NULL);
    if(retfd < 0){
        return retfd;
    }
    epoll_add(epollfd, retfd, EPOLLIN | EPOLLERR);
    assert(get_fd_type(retfd) == -1);
    set_fd_type(retfd, CLIENT | IN_EPOLL);
    assert(get_fd_type(retfd) != -1);
    return 0;
}

static void init(){
    fd_manager_init();
    signal(SIGPIPE, SIG_IGN);
    puts("init down");
}
int main(int argc, char *argv[]){
    init();
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 1024);

    int epollfd = epoll_create(BUFSIZE);
    assert(epollfd >= 0);
    epoll_add(epollfd, fd, EPOLLIN);

    for(;;){
        int num = epoll_wait(epollfd, events, BUFSIZE, -1);
        for(int i = 0; i < num; i++){
            int tmpfd = events[i].data.fd;
            // printf("poll fd = %d\n", tmpfd);
            // printf("events = %d\n", events[i].events);
            int n;
            if(tmpfd == fd){
                if(Accept(tmpfd, epollfd) < 0){
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