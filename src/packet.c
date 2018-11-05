#include "packet.h"


static char buf[BUFSIZE];

static int copy_and_deal_packet(int fd, Packet *packet, char *buf, int n);
static Packet *get_packet(int fd, int flag);
static Packet *get_packet(int fd, int flag){
    Packet *packet = (Packet*)get_packet_ptr(fd);
    if(packet == NULL){
        packet = (Packet *)malloc(sizeof(Packet));
        assert(packet);
        if(flag == 1)
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

void packet_init(Packet *packet, Client_server_flag flag){
    packet->buf = (char*)malloc(BUFSIZE);
    assert(packet->buf);
    packet->cap = BUFSIZE;
    packet->l = packet->r = 0;
    packet->size = 0;
    packet->state = 0;
    packet->buf_type = REQUEST_HEAD;
    packet->client_server_flag = flag;
    packet->com_flag = NOT_COMPLETE;
    // packet->info.chunked_size = 0;
    info_init(&packet->info);
}


int read_packet(int fd, int epollfd){
    int n = read(fd, buf, sizeof(buf));
    
    if(n <= 0)
        return n;
    Packet *packet = get_packet(fd, get_fd_type(fd));//
    if(copy_and_deal_packet(fd, packet, buf, n) < 0){
        return -1;
    }
    return n;
}
static int _send(int fd, Packet *packet){
    int n;
    if(packet->r < packet->l){
        n = write(fd, packet->buf + packet->l, packet->cap - packet->l);
        if(n < 0){
            return n;
        }
        packet->l += n;
        packet->size -= n;
        if(packet->l == packet->cap){
            // assert(packet->l == packet->cap);
            packet->l = 0;
            if(packet->size){
                int tmp = _send(fd, packet);
                if(tmp < 0){
                    return tmp;
                }
                n += tmp;
            }
            
        }
        else{
            
            // epoll_add;
        }
    }
    else{
        n = write(fd, packet->buf + packet->l, packet->r - packet->l);
        if(n < 0){
            return n;
        }
        packet->l += n;
        packet->size -= n;
        if(packet->l != packet->r){
            //epoll_add
        }
    }
    return n;
}
int send_packet(int fd, int epollfd){
    //printf("%d %d\n", fd, get_fd(fd));

    Packet *packet = get_packet(fd, get_fd_type(fd));
    int n;
    if(packet->buf_type != DATA_BODY){
        return 0;
    }
    if(get_fd(fd) == -1 && get_fd_type(fd) == -1){
        assert(packet->info.ip != 0);
        assert(packet->info.port != 0);
        int serverfd = connection_create(fd, packet->info.ip, packet->info.port);
        if(serverfd < 0){
            return serverfd;
        }
        epoll_add(epollfd, serverfd, EPOLLIN | EPOLLERR);
    }
    n = _send(get_fd(fd), packet);
    
    if(packet->size == 0 && packet->com_flag == COMPLETE && packet->client_server_flag == SERVER){
        int client_fd = get_fd(fd);
        Packet *client_packet = get_packet(client_fd, get_fd_type(client_fd));
        printf("client_fd = %d, server_fd = %d",client_fd, fd);
        struct in_addr addr;
        memcpy(&addr, &client_packet->info.ip, sizeof(addr));
        printf(" ip == %s\n",inet_ntoa(addr));
        connection_release(fd, client_packet->info.ip, client_packet->info.port);
    }
    if(packet->size == 0 && packet->com_flag == COMPLETE){
        packet->buf_type = REQUEST_HEAD;
        packet->state = 0;
        packet->com_flag = NOT_COMPLETE;
        packet->info.length = -1;
        packet->info.chunked_size = 0;
    }
    
    // 这里递归可能删除两次 小心怎么处理比较好

    return n;

}