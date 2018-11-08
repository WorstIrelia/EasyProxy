
#include "netlib.h"
int hostname2ip(char * hostname){
    printf("hostname = %s\n", hostname);
    struct hostent *he;
    struct in_addr **addr_list;
    if((he = gethostbyname(hostname)) == NULL){
        return -1;
    }
    addr_list = (struct in_addr **) he->h_addr_list;
    for(int i = 0; addr_list[i]; i++){
        puts(inet_ntoa(*addr_list[i]));
        return inet_addr(inet_ntoa(*addr_list[i]));
    }
    return 0;
}

int listen_port(unsigned short port){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        return fd;
    }
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        return -1;
    }
    if(listen(fd, 1024) < 0){
        return -1;
    }
    return fd;
}

int connect2server(unsigned int ip, unsigned short port){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(fd < 0)
        return fd;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = ip;
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    int ret;
    if((ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr))) < 0){
        if(errno != EINPROGRESS){
            close(fd);
            return -1;
        }
        
    }
    fcntl(fd, F_SETFL, oldOption);
    return fd;
}