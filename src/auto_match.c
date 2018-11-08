#include "auto_match.h"


static char buf[BUFSIZE];
extern Proxy_type proxy_type;

static void content_length(char *buf, Packet *packet, int index);
static void extend_packet(Packet *packet);
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
static size_t _atoi(Packet *packet, int index);
typedef void (*func)(char *, Packet *, int index);
static func func_array[] = {content_length};


int auto_match(Packet *packet, char *buf, int n);


static void _pri(Packet *packet){
    int i = packet->l;
    while(i != packet ->r){
        putchar(packet->buf[i]);
        i++;
        if(i == packet->cap){
            i = 0;
        }
    }
}

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
        // printf("%lu\n",packet->info.length);
    }
}



static void extend_packet(Packet *packet){
    int tmpcap = packet->cap << 1;
    char *tmpbuf = (char*)malloc(tmpcap);
    char *ptr = tmpbuf;
    assert(tmpbuf);
    while(packet->l != packet->r){
        *ptr++ = packet->buf[packet->l++];
        if(packet->l == packet->cap){
            packet->l = 0;
        }
    }
    
    free(packet->buf);
    packet->buf = tmpbuf;
    packet->l = 0;
    packet->r = packet->size;
    packet->cap = tmpcap;
}

static void do_copy(Packet *packet, char *buf, int n){
    int left = packet->cap - packet->size - 1;
    if(left < n){
        extend_packet(packet);
    }
    while(n--){
        // printf("%d", packet->r);
        // putchar(*buf);
        packet->buf[packet->r++] = *buf++;
        // putchar(packet->buf[packet->r - 1]);
        // puts("");
        packet->size++;
        if(packet->r == packet->cap)
            packet->r = 0;
    }
}


static int _auto_match_chunk(Packet *packet, char *buf, int n){//here
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
    if(packet->info.packet_type == CONNECT){
        packet->buf_type = HTTPS;
        if((proxy_type == CLIENT2SERVER || proxy_type == PROXY2SERVER)){
            packet->size = 0;
            packet->r = packet->l;
            assert(n == 0);
        }
        
        return 0;
    }
    if(packet->info.packet_type == GET){
        packet->com_flag = COMPLETE;
        return 0;
    }
    if(packet->info.length == 0){
        assert(n == 0);
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
    if(tmp > packet->buf + packet->cap){
        tmp = tmp - packet->cap;
    }
    char buf[BUFSIZE];
    int index = 0;
    while(*tmp != '/' && *tmp != ':') {
        buf[index++] = *tmp++;
        if(tmp == packet->buf + packet->cap){
            tmp = packet->buf;
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
            if(tmp == packet->buf + packet->cap){
                tmp = packet->buf;
            }
        }
        buf[index] = 0;
        // puts(buf);
        packet->info.port = htons(atoi(buf));
    }
    *ptr = tmp ;
    return;
}

static int _analyise_head(Packet *packet){
    assert(packet->buf_type == REQUEST_HEAD);
    if(packet->client_server_flag == SERVER)
        return 0;
    char type_buf[64];
    int index = 0;
    int i = packet->l;
    while(index < 64 && packet->buf[i] != ' '){
        type_buf[index++] = packet->buf[i++];
        if(i == packet->cap)
            i = 0;
    }
    i++;
    if(i == packet->cap){
        i = 0;
    }
    if(index == 64) return -1;
    type_buf[index] = 0;
    packet->info.packet_type = get_type(type_buf);
    if(packet->info.packet_type == UNKNOW){
        puts(buf);
        return -1;
    }
    if(proxy_type == CLIENT2PROXY || proxy_type == PROXY2PROXY){
        packet->info.ip = inet_addr(PROXY_IP);
        packet->info.port = htons(PROXY_PORT);
        return 0;
    }

    char *ptr = packet->buf + i;
    _get_ip_and_port(&ptr, packet);
    if(packet->info.ip == 0 || packet->info.port == 0)
        return -1;
    char *lsh = packet->buf + i;
    while(ptr != packet->buf + packet->r){
        *lsh++ = *ptr++;
        if(ptr == packet->buf + packet->cap){
            ptr = packet->buf;
        }
        if(lsh == packet->buf + packet->cap){
            lsh = packet->buf;
        }
    }
    packet->r = lsh - packet->buf;
    packet->size = packet->r < packet->l? packet->r + packet->cap - packet->l:packet->r - packet->l;
    return 0;
}

static int _analyise_body(Packet *packet){
    assert(packet->buf_type == REQUEST_BODY);
    int i = packet->l;
    while(i != packet->r && packet->buf[i] != '\n') {
        i++;
        if(i == packet->cap){
            i = 0;
        }
    }
    if(i == packet->r){
        return -1;
    }
    for(; i != packet->r; ){
        i++;
        if(i == packet->cap) i = 0;
        if(packet->buf[i] == '\r' && packet->buf[i + 1 == packet->cap? 0 : i + 1] == '\n'){
            break;
        }
        int index = 0;
        while(i != packet->r && packet->buf[i] != ':'){
            buf[index++] = packet->buf[i++];
            if(i == packet->cap) i = 0;
        }
        buf[index] = 0;
        for(int j = 0; j < sizeof(func_array)/sizeof(void*); j++){
            func_array[j](buf, packet, ++i);
            if(i == packet->cap){
                i = 0;
            }
        }
        while(i != packet->r && packet->buf[i] != '\n'){
            i++;
            if(i == packet->cap) i = 0;
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
            _pri(packet);
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
    // char *rsbuf = buf;
    // int totn = n;
    // if(packet->info.length == 0){
    //     assert(packet->state < 3);
    //     while(packet->state < 3 && n){
    //         packet->state++;
    //         n--;
    //         buf++;
    //     }
    // }
    // if(packet->state == 3 && n){
    //     packet->info.length = *(unsigned char *)buf;
    //     packet->state ++;
    //     buf++;
    //     n--;
    // }
    // if(packet->state == 4 && n){
    //     packet->info.length = packet->info.length << 8 + *(unsigned char *)buf;
    //     packet->state ++;
    //     buf++;
    //     n--;
    // }
    // if(packet->state == 5){
    //     if(packet->info.length > n){
    //         packet->info.length -= n;
    //         buf += n;
    //     }
    //     else{
    //         buf += packet->info.length;
    //         n -= packet->info.length;
    //         packet->info.length = 0;
    //         packet->state = 0;
    //         packet->com_flag = COMPLETE;
    //         if(n){
    //             _auto_match_https(packet, buf, n);
    //         }
    //     }
    // }
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
