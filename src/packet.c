#include "packet.h"


static char buf[BUFSIZE];
extern Proxy_type proxy_type;

static const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";

static int copy_and_deal_packet(int fd, Packet *packet, char *buf, int n);
static Packet *get_packet(int fd, int flag);
static void _reinit_packet(Packet *packet);
static int __send(int fd, const char *packet_buf, int n);
static int _read(int fd, char* buf, int size);
static void _de_code(char *buf, int n){
    for(int i = 0; i < n; i++){
        buf[i] ^= CODE;
    }

}
static Packet *get_packet(int fd, int flag){
    Packet *packet = (Packet*)get_packet_ptr(fd);
    if(packet == NULL){
        packet = (Packet *)malloc(sizeof(Packet));
        assert(packet);
        if(flag == SERVER)
            packet_init(packet, SERVER);
        else 
            packet_init(packet, CLIENT);
        set_packet_ptr(fd, packet);
    }
    return packet;
}

static int copy_and_deal_packet(int fd, Packet *packet, char *buf, int n){
    if(auto_match(packet, buf, n) < 0){
        return -1;
    }
    return 0;
}



void info_init(Info *ptr){
    ptr->ip = ptr->port = 0;
    ptr->packet_type = UNKNOW;
    ptr->length = -1;
    ptr->chunked_size = 0;
}

void packet_init(Packet *packet, Fd_type flag){
    packet->buf = (char*)malloc(BUFSIZE);
    assert(packet->buf);
    packet->cap = BUFSIZE;
    packet->l = packet->r = 0;
    packet->size = 0;
    packet->state = 0;
    packet->buf_type = REQUEST_HEAD;
    packet->client_server_flag = flag;
    packet->com_flag = NOT_COMPLETE;
    packet->connection_state = KEEP_ALIVE;
    // packet->info.chunked_size = 0;
    info_init(&packet->info);
}
void packet_destory(void *packet){
    free(((Packet *)packet)->buf);
}

static int _do_connect(int fd, int epollfd, Packet *packet){
    assert(packet->info.ip);
    assert(packet->info.port);
    int serverfd = connection_create(fd, packet->info.ip, packet->info.port);
    if(serverfd < 0){
        return -1;
    } 
        // assert(!epoll_add(epollfd, fd, EPOLLOUT | EPOLLERR));
    assert(get_fd_type(serverfd) == -1);
    set_fd_type(serverfd, SERVER);
    if(proxy_type == CLIENT2PROXY || proxy_type == PROXY2PROXY){
        set_fd_type(serverfd, get_fd_type(serverfd) | CODE);
    }
    return serverfd;
}


static int _read(int fd, char* buf, int size){
    int n = read(fd, buf, size);
    if(n <= 0){
        return n;
    }
    if(get_fd_type(fd) & CODE){
        _de_code(buf, n);
    }
    return n;
}

int read_packet(int fd, int epollfd){
    int n = _read(fd, buf, sizeof(buf));
    #ifdef DEBUG
    printf("read = %d\n", n);
    #endif
    if(n < 0)
        return n;
    assert(get_fd_type(fd) != -1);
    Packet *packet = get_packet(fd, get_fd_type(fd) & SERVER);
    if(n == 0 && (get_fd_type(fd) & SERVER) == CLIENT){
        return -1;
    }
    else if(n == 0){
        epoll_del(epollfd, fd, EPOLLIN);
        set_fd_type(fd, get_fd_type(fd) & (~IN_EPOLL));
        return 0;
    }
    if(copy_and_deal_packet(fd, packet, buf, n) < 0){
        return -1;
    }
    assert(packet->size != packet->cap);
    if(packet->buf_type == HTTPS && get_fd(fd) == -1){
        int serverfd = _do_connect(fd, epollfd, packet);
        if(serverfd < 0){
            return -1;
        }
        Packet *serverpacket = get_packet(serverfd, SERVER);
        serverpacket->buf_type = HTTPS;
        if(proxy_type == CLIENT2SERVER || proxy_type == PROXY2SERVER){
            int n = __send(fd, response, strlen(response));
            if(n < 0 || n != strlen(response)){
                return -1;
            }   
        }
        
    }
    if(packet->buf_type == DATA_BODY && get_fd(fd) == -1 && (get_fd_type(fd) & SERVER) == CLIENT){
        int serverfd = _do_connect(fd, epollfd, packet);
        if(serverfd < 0){
            return -1;
        }
    }
    int destfd = get_fd(fd);
    if((get_fd_type(destfd) & IN_EPOLL) != IN_EPOLL){
        if(packet->buf_type == HTTPS){
            assert(!epoll_add(epollfd, destfd, EPOLLIN | EPOLLOUT));
        }
        else{
            assert(!epoll_add(epollfd, destfd, EPOLLOUT));
        }
        set_fd_type(destfd, get_fd_type(destfd) | IN_EPOLL);
    }
    if(packet->buf_type == HTTPS){
        assert(!epoll_mod(epollfd, destfd, EPOLLIN | EPOLLOUT));   
    }
    return n;
}

static int __send(int fd, const char *packet_buf, int n){
    if((get_fd_type(fd) & CODE ) == 0){
        return write(fd, packet_buf, n);
    }
    for(int i = 0; i < (n < BUFSIZE? n : BUFSIZE); i++){
        buf[i] = *packet_buf++;
        buf[i] ^= CODE;
    }
    return write(fd, buf, (n < BUFSIZE? n : BUFSIZE));
}
static int _send(int fd, Packet *packet){
    assert(packet->size != packet->cap);
    int n;
    if(packet->connection_state == CLOSE && packet->com_flag == NOT_COMPLETE){
        return 0;
    }
    if(packet->r < packet->l){
        n = __send(fd, packet->buf + packet->l, packet->cap - packet->l);
        if(n < 0){
            return n;
        }
        packet->l += n;
        packet->size -= n;
        if(packet->l == packet->cap){
            packet->l = 0;
            if(packet->size){
                int tmp = _send(fd, packet);
                if(tmp < 0){
                    return tmp;
                }
                n += tmp;
            }
        }
    }
    else{
        n = __send(fd, packet->buf + packet->l, packet->r - packet->l);
        if(n < 0){
            return n;
        }
        packet->l += n;
        packet->size -= n;
    }
    return n;
}


int send_packet(int fd, int epollfd){
    int srcfd = get_fd(fd);
    if(srcfd < 0){
        return -1;
    }
    assert(srcfd >= 0);
    assert(get_fd_type(srcfd) >= 0);
    Packet *packet = get_packet(srcfd, get_fd_type(srcfd) & 0x1);
    int n;
    #ifdef DEBUG
    printf("now = %lu\n", packet->size);
    #endif
    n = _send(fd, packet);
    #ifdef DEBUG
    
    printf("send = %d\n", n);
    printf("leave = %lu\n", packet->size);
    #endif
    if(packet->buf_type == HTTPS){
        if(packet->size == 0){
            assert(!epoll_mod(epollfd, fd, EPOLLIN));
        }
    }
    else if(packet->size == 0 && packet->com_flag == COMPLETE){
        if(packet->connection_state != CLOSE){
            assert(!epoll_del(epollfd, srcfd, EPOLLIN));
        }
        assert(!epoll_mod(epollfd, fd, EPOLLIN));
        set_fd_type(srcfd, get_fd_type(srcfd) & (~IN_EPOLL));
        if(packet->buf_type != HTTPS && packet->client_server_flag == SERVER){
            Packet *clientpacket = get_packet(fd, get_fd_type(fd) & 0x1);
            if(packet->connection_state == CLOSE){
                server_close(srcfd, epollfd);
            }
            else{
                connection_release(srcfd, clientpacket->info.ip, clientpacket->info.port);
            }
        }
        _reinit_packet(packet);
    }
    else if(packet->size == 0){
        assert(epoll_del(epollfd, fd, EPOLLOUT) == 0);
        set_fd_type(fd, get_fd_type(fd) & (~IN_EPOLL));
    }
    return n;
}

static void _reinit_packet(Packet *packet){
    if(packet->buf_type != HTTPS){
        packet->buf_type = REQUEST_HEAD;
    }
    packet->connection_state = KEEP_ALIVE;
    packet->state = 0;
    packet->com_flag = NOT_COMPLETE;
    packet->info.length = -1;
    packet->info.chunked_size = 0;
}