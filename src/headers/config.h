#ifndef _CONFIG_H_
#define _CONFIG_H_

#define BUFSIZE 4096
#define _CODE 0x32

// #define DEBUG
#ifdef DEBUG
#define TIME_BEGIN      struct timeval start,end;\
                        gettimeofday(&start, NULL);
#else
#define TIME_BEGIN
#endif

#ifdef DEBUG
#define TIME_END(X)     gettimeofday(&end, NULL);\
                        printf(X" %f\n", (end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000);

#else
#define TIME_END(X) 
#endif

typedef enum proxy_type{
    CLIENT2SERVER,
    PROXY2PROXY,
    CLIENT2PROXY,
    PROXY2SERVER
}Proxy_type;


#endif