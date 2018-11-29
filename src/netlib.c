
#include "netlib.h"
#include "config.h"
#include "hash_table.h"
#include <sys/time.h>


#define MAX_QUEUE 128

static Hash_table str2ip;
static unsigned int func_hash(void *ptr, int n, unsigned int mod){
    char *tmp_ptr = *(char**)ptr;
    unsigned int res = 0;
    while(*tmp_ptr != 0){
        res = (res << 4) + *tmp_ptr;
        tmp_ptr++;
    }
    return res % mod;
}
static int cmp(void *lsh, void *rsh){
    char *_lsh = *(char**)lsh;
    char *_rsh = *(char**)rsh;
    return !strcmp(_lsh, _rsh);
}
int hostname2ip(char * hostname){
    static int flag = 0;
    if(!flag){
        hash_table_init(&str2ip, func_hash, cmp, sizeof(void*), sizeof(int));
        flag = 1;
    }
    void *value = (void *)lookup(&str2ip, &hostname);
    if(value){
        return *(int*)value;
    }
    struct hostent *he;
    struct in_addr **addr_list;
    if((he = gethostbyname(hostname)) == NULL){
        return -1;
    }
    addr_list = (struct in_addr **) he->h_addr_list;
    for(int i = 0; addr_list[i]; i++){
        int ret = inet_addr(inet_ntoa(*addr_list[i]));
        int len = strlen(hostname);
        char *key = (char *)malloc(len + 1);
        strcpy(key, hostname);
        key[len] = 0;
        insert(&str2ip, &key, &ret);
        return ret;
    }
    return 0;
}

int Listen(char *listenip, unsigned short listenport){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        return fd;
    }
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listenport);
    addr.sin_addr.s_addr = inet_addr(listenip);
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        return -1;
    }
    if(listen(fd, MAX_QUEUE) < 0){
        return -1;
    }
    return fd;
}

int test_connection(int fd){
    struct tcp_info info;
    int len = sizeof(info);
    getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
    return info.tcpi_state == TCP_ESTABLISHED;
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