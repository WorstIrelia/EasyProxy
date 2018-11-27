#include "packet.h"
#include "config.h"
#include "http.h"

typedef void (*func)(char *, Packet *, int index);
static size_t _atoi(Packet *packet, int index);
static void content_length(char *buf, Packet *packet, int index);
static void connection_state(char *buf, Packet *packet, int index);
func func_array[] = {content_length, connection_state, NULL};

static size_t _atoi(Packet *packet, int index){
    size_t ret = 0;
    while(packet->buf[index] != '\r'){
        ret = ret * 10 + packet->buf[index] - 48;
        index++;
        if(index == packet->cap){
            index = 0;
        }
    }
    return ret;
}
static int cmp(char *lsh, char *rsh, int num){
    int flag = 0;
    while(num--){
        flag+= *(lsh) - *(rsh);
        lsh++;
        rsh++;
        if(flag != 0) return flag;
    }
    return flag;
}
static void content_length(char *buf, Packet *packet, int index){
    if(index == packet->cap){
        index = 0;
    }
    if(!strcmp(buf, "Content-Length")){
        while(packet->buf[index] == ' ') {
            index++;
            if(index == packet->cap)
                index = 0;
        }
        packet->info.length = _atoi(packet, index);
        #ifdef DEBUG
        printf("%lu\n", packet->info.length);
        #endif
    }
}

static void connection_state(char *buf, Packet *packet, int index){
    if(index == packet->cap){
        index = 0;
    }
    if(!strcmp(buf, "Connection")){
        while(packet->buf[index] == ' ') {
            index++;
            if(index == packet->cap)
                index = 0;
        }
        if(!cmp(&packet->buf[index], "close", 5)){
            packet->connection_state = CLOSE;
        }
        #ifdef DEBUG
        printf("%lu\n", packet->info.length);
        #endif
    }
}