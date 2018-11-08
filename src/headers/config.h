#ifndef _CONFIG_H_
#define _CONFIG_H_

#define BUFSIZE 4096
#define LISTENPORT 8888
#define SERVER_IP
#define SERVERPORT
#define _CODE 0x32
#define PROXY_IP "43.224.33.57"
#define PROXY_PORT 2018

typedef enum proxy_type{
    CLIENT2SERVER,
    PROXY2PROXY,
    CLIENT2PROXY,
    PROXY2SERVER
}Proxy_type;


#endif