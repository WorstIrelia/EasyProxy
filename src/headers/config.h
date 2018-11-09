#ifndef _CONFIG_H_
#define _CONFIG_H_

#define BUFSIZE 4096
#define _CODE 0x32

typedef enum proxy_type{
    CLIENT2SERVER,
    PROXY2PROXY,
    CLIENT2PROXY,
    PROXY2SERVER
}Proxy_type;


#endif