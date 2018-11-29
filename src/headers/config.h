#ifndef _CONFIG_H_
#define _CONFIG_H_

#define BUFSIZE 4096
#define _CODE 0x32
#define _LOG
#define DEBUG
// #define NOHTTPS
// #define ONEREQUEST


#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define YELLOW "\e[1;33m"
#define BLUE "\e[1;34m"
#define PURPLE "\e[1;35m"
#define CYAN "\e[1;36m"
#define GRAY "\e[1;37m"
#define COLOR_END "\e[0m"



#ifdef _LOG
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define COLOR_LOG(COLOR, fmt, ...) printf(COLOR""fmt""COLOR_END, ##__VA_ARGS__) 
#else
#define LOG(fmt, ...) 
#define COLOR_LOG(COLOR, fmt, ...)
#endif



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