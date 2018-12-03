#include "packet.h"
#include "config.h"
#include "http.h"

typedef void (*func)(char *, Packet *, int index);
static size_t _atoi(Packet *packet, int index);
static void content_length(char *buf, Packet *packet, int index);
static void connection_state(char *buf, Packet *packet, int index);
func func_array[] = {content_length, connection_state, NULL};

#define PACKET2REALBUF(packet) ((char *)((packet)->buf[(packet)->now_use].buf))
#define PACKET2BUF(packet) ((packet)->buf[packet->now_use])

static size_t _atoi(Packet *packet, int index){
    size_t ret = 0;
    while(PACKET2REALBUF(packet)[index] != '\r'){
        ret = ret * 10 + PACKET2REALBUF(packet)[index] - 48;
        index++;
        if(index == PACKET2BUF(packet).cap){
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
    if(index == PACKET2BUF(packet).cap){
        index = 0;
    }
    if(!strcmp(buf, "Content-Length")){
        while(PACKET2REALBUF(packet)[index] == ' ') {
            index++;
            if(index == PACKET2BUF(packet).cap)
                index = 0;
        }
        packet->info.length = _atoi(packet, index);
        #ifdef DEBUG
        printf("%lu\n", packet->info.length);
        #endif
    }
}

static void connection_state(char *buf, Packet *packet, int index){
    if(index == PACKET2BUF(packet).cap){
        index = 0;
    }
    if(!strcmp(buf, "Connection")){
        while(PACKET2REALBUF(packet)[index] == ' ') {
            index++;
            if(index == PACKET2BUF(packet).cap)
                index = 0;
        }
        if(!cmp(PACKET2REALBUF(packet), "close", 5)){
            packet->connection_state = CLOSE;
        }
    }
}