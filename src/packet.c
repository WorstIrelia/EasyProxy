#include "packet.h"


extern Proxy_type proxy_type;



static void _info_init(Info *ptr);



Packet* packet_init(){
    Packet *packet = (Packet *)malloc(sizeof(Packet));
    assert(packet);
    packet->buf = (char*)malloc(BUFSIZE);
    assert(packet->buf);
    packet->cap = BUFSIZE;
    packet->refcnt = 0;
    packet_reinit(packet);
    return packet;
}
void packet_destory(void *ptr){
    Packet* packet = (Packet *)ptr;
    packet->refcnt--;
    if(!packet->refcnt){
        free(((Packet *)packet)->buf);
        free(packet);
    }   
}

void packet_request(Packet *packet){
    packet->packet_kind = REQUEST;
}
void packet_response(Packet *packet){
    packet->packet_kind = RESPONSE;
    if(packet->buf_type != HTTPS){
        packet->buf_type = REQUEST_HEAD;
    }
    packet->connection_state = KEEP_ALIVE;
    packet->state = 0;
    packet->com_flag = NOT_COMPLETE;
    packet->info.length = -1;
    packet->info.chunked_size = 0;
}

void packet_reinit(Packet *packet){
    
    packet->l = packet->r = 0;
    packet->size = 0;
    packet->state = 0;
    packet->buf_type = REQUEST_HEAD;
    packet->packet_kind = INIT;
    packet->com_flag = NOT_COMPLETE;
    packet->connection_state = KEEP_ALIVE;
    _info_init(&packet->info);
}


void _info_init(Info *ptr){
    ptr->ip = ptr->port = 0;
    ptr->packet_type = UNKNOW;
    ptr->length = -1;
    ptr->chunked_size = 0;
}










