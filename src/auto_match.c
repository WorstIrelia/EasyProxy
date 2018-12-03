#include "auto_match.h"


#define PACKET2REALBUF(packet) ((char *)((packet)->buf[(packet)->now_use].buf))
#define PACKET2BUF(packet) ((packet)->buf[packet->now_use])
static char buf[BUFSIZE];
extern Proxy_type proxy_type;
extern char *proxyip;
extern unsigned short proxyport;
static void do_copy(Packet *packet, char *buf, int n);
static int _auto_match_chunk(Packet *packet, char *buf, int n);
static int _auto_match_data_body(Packet *packet, char *buf, int n);
static Packet_type get_type(char *buf);
static void _get_ip_and_port(char **ptr, Packet *);
static int _analyise_head(Packet *packet);
// static void match(char *buf, Packet *packet, int index, void (*func)(char *,Packet *, int));
static int _analyise_body(Packet *packet);
static int _auto_match_request_body(Packet *packet, char *buf, int n);
static int _auto_match_request_head(Packet *packet, char *buf, int n);
typedef void (*func)(char *, Packet *, int index);
extern func func_array[3];


int auto_match(Packet *packet, char *buf, int n);


#ifdef DEBUG
static void _pri(Packet *packet, int type){
    int i = packet->buf[type].l;
    while(i != packet->buf[type].r){
        printf(RED"%c"COLOR_END, packet->buf[type].buf[i]);
        BUF_ADD_INDEX(&packet->buf[type], i);
    }
}
#endif


static void do_copy(Packet *packet, char *buf, int n){
    buf_copy(&PACKET2BUF(packet), buf, n);
}


static int _auto_match_chunk(Packet *packet, char *buf, int n){
    for(int i = 0; i < n; i++){
        if(packet->state == 2 && packet->info.chunked_size != 0){
            int left = n - i;
            if(left >= packet->info.chunked_size){
                i += packet->info.chunked_size;
                packet->info.chunked_size = 0;
                packet->state = 0;
                i--;
            }
            else{
                packet->info.chunked_size -= left;
                break;
            }
        }
        if(packet->state == 0 && ((buf[i] <= '9' && buf[i] >= '0' )|| (buf[i] >= 'A' && buf[i] <= 'F') || (buf[i] >= 'a' && buf[i] <= 'f'))){
            packet->info.chunked_size = packet->info.chunked_size * 16;
            if(buf[i] <= '9'){
                packet->info.chunked_size += buf[i] - '0';
            }
            else if(buf[i] <= 'f' && buf[i] >= 'a'){
                packet->info.chunked_size += buf[i] - 'a' + 10;
            }
            else{
                packet->info.chunked_size += buf[i] - 'A' + 10;
            }
        }
        else if(packet->state == 0 && buf[i] == '\r'){
            packet->state = 1;
        }
        else if(packet->state == 1 && buf[i] == '\n'){
            if(packet->info.chunked_size != 0)
                packet->info.chunked_size += 2;
            packet->state = 2;
        }
        else if(packet->state == 2 && buf[i] == '\r' && packet->info.chunked_size == 0){
            packet->state = 3;
        }
        else if(packet->state == 3 && buf[i] == '\n' && packet->info.chunked_size == 0){
            packet->com_flag = COMPLETE;
            assert(i == n - 1);
            packet->state = 0;
        }
    }
    return 0;

}

static int _auto_match_data_body(Packet *packet, char *buf, int n){
    if(packet->packet_kind == REQUEST || packet->packet_kind == INIT){
        if(packet->info.packet_type == CONNECT){
            packet->buf_type = HTTPS;
            if((proxy_type == CLIENT2SERVER || proxy_type == PROXY2SERVER)){
                PACKET2BUF(packet).size = 0;
                PACKET2BUF(packet).r = PACKET2BUF(packet).l;
                assert(n == 0);
            }
            return 0;
        }
        if(packet->info.packet_type == GET){
            packet->com_flag = COMPLETE;
            return 0;
        }
    }
    
    if(packet->info.length == 0){
        if(n != 0){
            COLOR_LOG(RED, "n == %d\n", n);
            for(int i = 0; i < n; i++){
                COLOR_LOG(RED, "%02X ", (unsigned char)buf[i]);
            }
            puts("");
        }
        assert(n >= 0);//why?? 目前看到iqiyi qipashuo里 会多发一个字节， 很奇怪。。\r\n\r\n\n
        packet->com_flag = COMPLETE;
        return 0;
    }
    if(n == 0){
        return 0;
    }
    assert(packet->buf_type == DATA_BODY);
    if(packet->info.length != -1){
        do_copy(packet, buf, n);
        packet->info.length -= n;
        if(packet->info.length == 0){
            packet->com_flag = COMPLETE;
        }
    }
    else{
        if(_auto_match_chunk(packet, buf, n) < 0){
            return -1;
        }
        do_copy(packet, buf, n);
    }
    return 0;
}


static Packet_type get_type(char *buf){
    if(!strcmp(buf, "GET"))
        return GET;
    if(!strcmp(buf, "POST"))
        return POST;
    if(!strcmp(buf, "CONNECT"))
        return CONNECT;
    return UNKNOW;
}

static void _get_ip_and_port(char **ptr, Packet *packet){
    char *tmp = *ptr;
    // http://
    if(packet->info.packet_type != CONNECT) tmp += 7;
    if(tmp > PACKET2REALBUF(packet)+ PACKET2BUF(packet).cap){
        tmp = tmp - PACKET2BUF(packet).cap;
    }
    char buf[BUFSIZE];
    int index = 0;
    while(*tmp != '/' && *tmp != ':') {
        buf[index++] = *tmp++;
        if(tmp == PACKET2REALBUF(packet)+ PACKET2BUF(packet).cap){
            tmp = PACKET2REALBUF(packet);
        }
    }
    buf[index] = 0;
    packet->info.ip = hostname2ip(buf);
    if(*tmp == '/' || *tmp == ' '){
        packet->info.port = htons(80);
    }
    else{
        assert(*tmp == ':');
        index = 0;
        tmp++;
        while(*tmp != '/' && *tmp != ' ') {
            buf[index++] = *tmp++;
            if(tmp == PACKET2REALBUF(packet)+ PACKET2BUF(packet).cap){
                tmp = PACKET2REALBUF(packet);
            }
        }
        buf[index] = 0;
        packet->info.port = htons(atoi(buf));
    }
    *ptr = tmp ;
    return;
}

static int _analyise_head(Packet *packet){
    assert(packet->buf_type == REQUEST_HEAD);
    if(packet->packet_kind == RESPONSE)
        return 0;

    #ifdef ONEREQUEST
    static int flag = 0;
    #endif
    char type_buf[64];
    int index = 0;
    int i = PACKET2BUF(packet).l;
    while(index < 64 && PACKET2REALBUF(packet)[i] != ' '){
        type_buf[index++] = PACKET2REALBUF(packet)[i++];
        if(i == PACKET2BUF(packet).cap)
            i = 0;
    }
    i++;
    if(i == PACKET2BUF(packet).cap){
        i = 0;
    }
    if(index == 64) return -1;
    type_buf[index] = 0;
    packet->info.packet_type = get_type(type_buf);
    #ifdef NOHTTPS
    if(packet->info.packet_type == CONNECT){
        return -1;
    }
    #endif
    #ifdef ONEREQUEST
    if(flag){
        return -1;
    }
    flag = 1;
    #endif
    if(packet->info.packet_type == UNKNOW){
        printf("packet->packet_type == ");
        puts(buf);
        return -1;
    }
    if(proxy_type == CLIENT2PROXY || proxy_type == PROXY2PROXY){
        packet->info.ip = inet_addr(proxyip);
        packet->info.port = htons(proxyport);
        return 0;
    }

    char *ptr = PACKET2REALBUF(packet)+ i;
    _get_ip_and_port(&ptr, packet);
    if(packet->info.ip == 0 || packet->info.port == 0)
        return -1;
    char *lsh = PACKET2REALBUF(packet)+ i;
    while(ptr != PACKET2REALBUF(packet)+ PACKET2BUF(packet).r){
        *lsh++ = *ptr++;
        if(ptr == PACKET2REALBUF(packet)+ PACKET2BUF(packet).cap){
            ptr = PACKET2REALBUF(packet);
        }
        if(lsh == PACKET2REALBUF(packet)+ PACKET2BUF(packet).cap){
            lsh = PACKET2REALBUF(packet);
        }
    }
    PACKET2BUF(packet).r = lsh - PACKET2REALBUF(packet);
    PACKET2BUF(packet).size = PACKET2BUF(packet).r < PACKET2BUF(packet).l? PACKET2BUF(packet).r + PACKET2BUF(packet).cap - PACKET2BUF(packet).l:PACKET2BUF(packet).r - PACKET2BUF(packet).l;
    return 0;
}

static int _analyise_body(Packet *packet){
    assert(packet->buf_type == REQUEST_BODY);
    int i = PACKET2BUF(packet).l;
    while(i != PACKET2BUF(packet).r && PACKET2REALBUF(packet)[i] != '\n') {
        i++;
        if(i == PACKET2BUF(packet).cap){
            i = 0;
        }
    }
    if(i == PACKET2BUF(packet).r){
        return -1;
    }
    for(; i != PACKET2BUF(packet).r; ){
        i++;
        if(i == PACKET2BUF(packet).cap) i = 0;
        if(PACKET2REALBUF(packet)[i] == '\r' && PACKET2REALBUF(packet)[i + 1 == PACKET2BUF(packet).cap? 0 : i + 1] == '\n'){
            break;
        }
        int index = 0;
        while(i != PACKET2BUF(packet).r && PACKET2REALBUF(packet)[i] != ':'){
            buf[index++] = PACKET2REALBUF(packet)[i++];
            if(i == PACKET2BUF(packet).cap) i = 0;
        }
        buf[index] = 0;
        for(int j = 0; func_array[j]; j++){
            func_array[j](buf, packet, ++i);
            if(i == PACKET2BUF(packet).cap){
                i = 0;
            }
        }
        while(i != PACKET2BUF(packet).r && PACKET2REALBUF(packet)[i] != '\n'){
            i++;
            if(i == PACKET2BUF(packet).cap) i = 0;
        }
        
    }
    return 0;


}
static int _auto_match_request_body(Packet *packet, char *buf, int n){
    assert(packet->buf_type == REQUEST_BODY);
    for(int i = 0; i < n; i++){
        if(buf[i] == '\r' && (packet->state == 0 || packet->state == 2))
            packet->state ++;
        else if(buf[i] == '\r')
            packet->state = 1;
        else if(buf[i] == '\n' && (packet->state == 1 || packet->state == 3))
            packet->state ++;
        else
            packet->state = 0;
        if(packet->state == 4){
            i++;
            do_copy(packet, buf, i);
            #ifdef DEBUG
            COLOR_LOG(RED, "_auto_match_request_body\n");
            _pri(packet, packet->now_use);
            #endif
            if(_analyise_body(packet) < 0)
                return -1;
            packet->buf_type = DATA_BODY;
            packet->state = 0;
            _auto_match_data_body(packet, buf + i, n - i);
            return 0;
        }
    }
    do_copy(packet, buf, n);
    return 0;
}

static int _auto_match_request_head(Packet *packet, char *buf, int n){
    assert(packet->buf_type == REQUEST_HEAD);
    for(int i = 0; i < n; i++){
        if(packet->state == 0 && buf[i] == '\r')
            packet->state = 1;
        else if(packet->state == 1 && buf[i] == '\r')
            packet->state = 1;
        else if(packet->state == 1 && buf[i] == '\n')
            packet->state = 2;
        else{
            packet->state = 0;
        }
        if(packet->state == 2){
            i++;
            do_copy(packet, buf, i);
            #ifdef DEBUG
            COLOR_LOG(RED, "_auto_match_request_head\n");
            _pri(packet, packet->now_use);
            #endif
            if(_analyise_head(packet) < 0)
                return -1;
            packet->buf_type = REQUEST_BODY;
            _auto_match_request_body(packet, buf + i, n - i);
            return 0;
        }
    }
    do_copy(packet, buf, n);
    return 0;
}

static int _auto_match_https(Packet *packet, char *buf, int n){
    do_copy(packet, buf, n);
    return n;
}

int auto_match(Packet *packet, char *buf, int n){
    switch (packet->buf_type){
        case REQUEST_HEAD:
            if(_auto_match_request_head(packet, buf, n) < 0){
                return -1;
            }
            break;
        case REQUEST_BODY:
            if(_auto_match_request_body(packet, buf, n) < 0){
                return -1;
            }
            break;
        case DATA_BODY:
            if(_auto_match_data_body(packet, buf, n) < 0){
                return -1;
            }
            break;
        case HTTPS:
            if(_auto_match_https(packet, buf, n) < 0){
                return -1;
            }
            break;
    }
    return 0;
}
