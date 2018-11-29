#include "io.h"


static char buf[BUFSIZE];
extern Proxy_type proxy_type;
static const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";

static void _de_code(char *, int );
static Packet *get_packet(int);
static int _do_connect(int ,int , Packet*);
static int _read(int , char *, int);
static int __send(int fd, const char *packet_buf, int n);
static int _send(int fd, Packet *packet);


#ifdef _LOG
static void _pri(Packet *packet){
    int i = packet->l;
    while(i != packet ->r){
        printf(RED"%c"COLOR_END, packet->buf[i]);
        i++;
        if(i == packet->cap){
            i = 0;
        }
    }
}
#endif

static void _de_code(char *buf, int n){
    for(int i = 0; i < n; i++){
        buf[i] ^= CODE;
    }

}

static Packet *get_packet(int fd){
    Packet *packet = (Packet*)get_packet_ptr(fd);
    if(packet == NULL){
        packet = packet_init();
        set_packet_ptr(fd, packet);
        packet->refcnt++;
    }
    return packet;
}

static int _do_connect(int fd, int epollfd, Packet *packet){
    assert(packet->info.ip);
    assert(packet->info.port);
    int server_fd = connection_create(fd);
    if(server_fd < 0){
        return -1;
    } 
    assert(get_fd_type(server_fd) == -1);
    set_fd_type(server_fd, SERVER);
    if(proxy_type == CLIENT2PROXY || proxy_type == PROXY2PROXY){
        set_fd_type(server_fd, get_fd_type(server_fd) | CODE);
    }
    return server_fd;
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

int read_packet(int fd, int epollfd){
    if(get_fd_type(fd) == -1){
        COLOR_LOG(YELLOW, "get_fd_type == -1\n");
        return -1;
    }
    int n = _read(fd, buf, sizeof(buf));
    if(n < 0){
        COLOR_LOG(RED, "read error\n");
        COLOR_LOG(RED, "errno = %d\n", errno);
        #ifdef _LOG
        if(get_fd_type(fd) & SERVER){
            COLOR_LOG(RED, "SERVER\n");
        }
        else{
            COLOR_LOG(RED, "CLIENT\n");
        }
        #endif
        return n;
    }
    Packet *packet = get_packet(fd);
    if(n == 0){
        switch(get_fd_type(fd) & SERVER){
            case CLIENT:
                COLOR_LOG(YELLOW, "read == 0 and client\n");
                #ifdef _LOG
                if(packet->size != 0){
                    COLOR_LOG(BLUE, "size = %lu\n", packet->size);
                    _pri(packet);
                }
                #endif
                return -1;
            case SERVER:
                COLOR_LOG(YELLOW, "read == 0 and server\n");
                if(packet->buf_type != HTTPS && (packet->packet_kind == REQUEST || packet->size == 0)){
                    COLOR_LOG(YELLOW, "http, REQUEST or size == 0\n");
                    return -1;
                }
                assert(get_fd_type(fd) & IN_EPOLL);
                epoll_del(epollfd, fd, EPOLLIN);
                set_fd_type(fd, get_fd_type(fd) & (~IN_EPOLL));
                server_close(fd, epollfd);
                return 0;
        }
    }
    
    if(auto_match(packet, buf, n) < 0){
        return -1;
    }
    assert(packet->size != packet->cap);
    if(packet->packet_kind == INIT){
        if(packet->buf_type == REQUEST_HEAD){
            return n;
        }
        assert(get_fd(fd) == -1);
        int server_fd = _do_connect(fd, epollfd, packet);
        if(server_fd < 0){
            return server_fd;
        }
        assert(get_packet_ptr(server_fd) == NULL);
        set_packet_ptr(server_fd, packet);
        packet->refcnt++;
        packet_request(packet);
        if(packet->buf_type == HTTPS && (proxy_type == CLIENT2SERVER || proxy_type == PROXY2SERVER)){
            int n = __send(fd, response, strlen(response));
            if(n < 0 || n != strlen(response)){
                COLOR_LOG(RED, "https response failed!!!!!!!");
                return -1;
            }
        }
    }
    int destfd = get_fd(fd);
    if(destfd < 0){
        COLOR_LOG(GREEN, "read desfd < 0 %d \n", fd);
        return destfd;
    }
    assert(destfd >= 0);
    if((get_fd_type(destfd) & IN_EPOLL) != IN_EPOLL){
        int ret = epoll_add(epollfd, destfd, EPOLLIN | EPOLLOUT);
        assert(!ret);
        set_fd_type(destfd, get_fd_type(destfd) | IN_EPOLL);
    }
    else{
        int ret = epoll_mod(epollfd, destfd, EPOLLIN | EPOLLOUT);
        assert(!ret);
    }
    return n;
}

int send_packet(int fd, int epollfd){
    if(get_fd_type(fd) < 0){
        COLOR_LOG(YELLOW, "get_fd_type == -1\n");
        return -1;
    }
    Packet *packet = (Packet*)get_packet_ptr(fd);
    if(packet == NULL){
        COLOR_LOG(RED, "packet == null\n");
        return -1;
    }
    int n;
    n = _send(fd, packet);
    if(n < 0){
        COLOR_LOG(RED, "send error\n");
        COLOR_LOG(RED, "errno = %d\n", errno);
        return n;
    }
    if(packet->size){
        return n;
    }
    assert((get_fd_type(fd) & IN_EPOLL) == IN_EPOLL);
    int ret = epoll_mod(epollfd, fd, EPOLLIN);
    assert(!ret);
    if(packet->buf_type != HTTPS && packet->com_flag == COMPLETE){
        if(get_fd(fd) < 0 && packet->packet_kind == RESPONSE){
            assert((get_fd_type(fd) & SERVER) == CLIENT);
            return n;
        }
        else if(get_fd(fd) < 0){ // why such happend 
            COLOR_LOG(GREEN, "get_fd(fd) < 0 %d \n", fd);
            return -1;
        }
        assert(get_fd(fd) >= 0);
        assert(get_fd_type(get_fd(fd)) & IN_EPOLL);
        if(packet->packet_kind == REQUEST){//fd = serverfd
            ret = epoll_mod(epollfd, get_fd(fd), EPOLLIN | EPOLLOUT);//clientfd = read and write
            packet_response(packet);
            assert(!ret);
        }
        else if(packet->packet_kind == RESPONSE){// fd = clientfd
            
            epoll_del(epollfd, get_fd(fd), EPOLLIN);// del serverfd
            assert(get_packet_ptr(fd) == get_packet_ptr(get_fd(fd)));
            connection_release(get_fd(fd));
            packet_reinit(packet);
        }
    }
    return n;
}