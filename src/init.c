#include "fd_manager.h"
#include "config.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


static char *path = "../conf/proxy.conf";
Proxy_type proxy_type;
unsigned short listenport = 0;
char *listenip = NULL;
unsigned short proxyport = 0;
char *proxyip = NULL;
static int match(char *str, char *aim){
    int len = strlen(aim);
    for(int i = 0; i < len; i++){
        if(str[i] != aim[i]){
            return 0;
        }
    }
    return 1;
}
static void get_port(char *str){
    if(match(str, "proxyport")){
        char *ptr = str + strlen("proxyport") + 1;
        proxyport = atoi(ptr);
    }
    else if(match(str, "port")){
        char *ptr = str + strlen("port") + 1;
        listenport = atoi(ptr);
    }
}
static void get_ip(char *str){
    if(match(str, "proxyip")){
        char *ptr = str + strlen("proxyip") + 1;
        proxyip = malloc(16);
        char *tmpip = proxyip;
        while(*ptr != '\n' && *ptr != 0){
            *tmpip++ = *ptr++;
        }
        *tmpip = 0;
    }
    else if(match(str, "ip")){  
        char *ptr = str + strlen("ip") + 1;
        listenip = malloc(16);
        char *tmpip = listenip;
        while(*ptr != '\n' && *ptr != 0){
            *tmpip++ = *ptr++;
        }
        *tmpip = 0;
    }
}
static void get_type(char *str){
    if(match(str, "proxytype")){
        char *ptr = str + strlen("proxytype") + 1;
        if(match(ptr, "proxy2proxy")){
            proxy_type = PROXY2PROXY;
        }
        else if(match(ptr, "proxy2server")){
            proxy_type = PROXY2SERVER;
        }
        else if(match(ptr, "client2proxy")){
            proxy_type = CLIENT2PROXY;
        }
        else if(match(ptr, "client2server")){
            proxy_type = CLIENT2SERVER;
        }
    }
}

void (*func[])(char *) = {get_type, get_ip, get_port};







int init(int argc, char *argv[]){
    // char buf[BUFSIZE];
    char str[BUFSIZE];
    struct sigaction act, oldact;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NODEFER;
    sigaction(SIGPIPE, &act, &oldact);
    fd_manager_init();

    if(argc > 2){
        return -1;
    }
    if(argc == 2){
        path = argv[1];
    }
    FILE* file = fopen(path, "r");
    // int fd = open(path, O_RDWR);
    assert(file);
    while(fgets(str, sizeof(str) - 1, file)!= NULL){
        if(str[0] == '#') continue;
        printf("%s", str);
        for(int i = 0; i < sizeof(func)/sizeof(void *); i++){
            func[i](str);
        }
    }
    printf("%d %s %d %s %d\n",proxy_type, listenip, listenport, proxyip, proxyport);
    fclose(file);
    return 0;
}
