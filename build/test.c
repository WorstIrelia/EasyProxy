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
#include "../src/headers/__packet.h"
#include "../src/headers/config.h"
#include "../src/headers/fd_manager.h"
int save_packet_head(int fd, Packet *packet, Packet *body);
int main(){
    fd_manager_init();
    // assert(1 == 0);
    // printf("????\n");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 10);
    // printf("ok\n");
    int testfd = accept(fd, NULL, NULL);
    // printf("ok\n");
    while(read_packet(testfd, 0) > 0);
    // printf("\n");
    // printf("fd_manager_begin\n");
    
    // printf("fd_manager_end\n");
    // int serverfd = create_connection(testfd, head.packet_head.ip, head.packet_head.port);
    // assert(serverfd >= 0);
    // printf("socked_fd = %d\n", serverfd);
    
    // char buf[BUFSIZE];
    // char *ptr = buf;
    // int cnt = 2;
    // for(char **tmp = head.packet_head.lines; *tmp != NULL; tmp++){
    //     char *ptr_tmp = *tmp;
    //     while(*ptr_tmp) {
    //         *ptr++ = *ptr_tmp++;
    //         cnt++;
    //     }
        
    // }
    // *ptr++ = '\r';
    // *ptr++ = '\n';
    // send(serverfd, buf, cnt, 0);
    // int fssd = open("mark", O_RDWR);
    // while(1){
    //     int n = recv(serverfd, buf, sizeof(buf), 0);
    //     write(fssd, buf, n);
    //     send(testfd, buf, n, 0);
    // }


}