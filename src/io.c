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
    if(get_fd_type(server_fd) >= 0){
        if(get_fd_type(server_fd) & IN_EPOLL){
            epoll_del(epollfd, server_fd, EPOLLIN);
        }
        del_fd_type(server_fd);
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
    int type = (get_fd_type(fd) & SERVER) ^ 1;
    int n;
    if(packet->buf[type].r < packet->buf[type].l){
        n = __send(fd, packet->buf[type].buf + packet->buf[type].l, packet->buf[type].cap - packet->buf[type].l);
        if(n < 0){
            return n;
        }
        packet->buf[type].l += n;
        packet->buf[type].size -= n;
        if(packet->buf[type].l == packet->buf[type].cap){
            packet->buf[type].l = 0;
            if(packet->buf[type].size){
                int tmp = _send(fd, packet);
                if(tmp < 0){
                    return tmp;
                }
                n += tmp;
            }
        }
    }
    else{
        n = __send(fd, packet->buf[type].buf + packet->buf[type].l, packet->buf[type].r - packet->buf[type].l);
        if(n < 0){
            return n;
        }
        packet->buf[type].l += n;
        packet->buf[type].size -= n;
    }
    return n;
}

int read_packet(int fd, int epollfd){
    if(get_fd_type(fd) == -1){
        COLOR_LOG(YELLOW, "get_fd_type == -1\n");
        return -1;
    }
    int n = _read(fd, buf, sizeof(buf));//modified
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
    packet->now_use = get_fd_type(fd) & SERVER;
    if(n == 0){
        switch(get_fd_type(fd) & SERVER){
            case CLIENT:
                COLOR_LOG(YELLOW, "read == 0 and client\n");
                #ifdef _LOG
                if(packet->buf[SERVER].size != 0 || packet->buf[CLIENT].size != 0){
                    COLOR_LOG(BLUE, "s2c size = %lu c2s size = %lu\n", packet->buf[SERVER].size, packet->buf[CLIENT].size);
                    // _pri(packet, get_fd_type(fd) & SERVER);
                }
                #endif
                return -1;
            case SERVER:
                COLOR_LOG(YELLOW, "read == 0 and server\n");
                if(packet->buf_type == HTTPS){
                    return -1;
                }
                else if(get_fd_type(fd) & IN_POOL){
                    COLOR_LOG(RED, "del_pool!\n");
                    del_pool(fd);
                    set_fd_type(fd, get_fd_type(fd) & ~IN_POOL);
                }
                else if(packet->buf_type != HTTPS && (packet->packet_kind == REQUEST || packet->com_flag == NOT_COMPLETE)){
                    COLOR_LOG(YELLOW, "http, REQUEST or size == 0\n");
                    return -1;
                }
                assert(get_fd_type(fd) & IN_EPOLL);
                server_close(fd, epollfd);
                return 0;
        }
    }
    int destfd = get_fd(fd);
    if(destfd < 0){ // server调用server_close释放掉，但是buf[SERVER]里必没有数据(因为现在在读取)，现在destfd == -1 再次收到请求
        COLOR_LOG(GREEN, "read desfd < 0 %d \n", fd);
        assert(packet->buf[SERVER].size == 0);
        packet_reinit(packet);
    }
    if(auto_match(packet, buf, n) < 0){
        return -1;
    }
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
    destfd = get_fd(fd);
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
    packet->now_use = get_fd_type(fd) & SERVER;
    if(packet == NULL){
        COLOR_LOG(RED, "packet == null\n");
        return -1;
    }
    int n;
    int type = (get_fd_type(fd) & SERVER) ^ 1 ;
    n = _send(fd, packet);
    if(n < 0){
        COLOR_LOG(RED, "send error\n");
        COLOR_LOG(RED, "errno = %d\n", errno);
        return n;
    }
    if(packet->buf[type].size){
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
            
            // epoll_del(epollfd, get_fd(fd), EPOLLIN);// del serverfd // 在链接池里收到了 n == 0
            assert(get_packet_ptr(fd) == get_packet_ptr(get_fd(fd)));
            connection_release(get_fd(fd));
            packet_reinit(packet);
        }
    }
    return n;
}