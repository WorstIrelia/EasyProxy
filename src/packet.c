#include "packet.h"


static char buf[BUFSIZE];

static int copy_and_deal_packet(int fd, Packet *packet, char *buf, int n);
static Packet *get_packet(int fd, int flag);
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
    // packet->info.chunked_size = 0;
    info_init(&packet->info);
}
void packet_destory(Packet *packet){
    free(packet->buf);
}


int read_packet(int fd, int epollfd){
    int n = read(fd, buf, sizeof(buf));
    if(n <= 0)
        return n;
    assert(get_fd_type(fd) != -1);
    Packet *packet = get_packet(fd, get_fd_type(fd) & 0x1);//
    if(copy_and_deal_packet(fd, packet, buf, n) < 0){
        return -1;
    }
    if(packet->buf_type == DATA_BODY && get_fd(fd) == -1 && (get_fd_type(fd) & 0x1) == CLIENT){
        assert(packet->info.ip);
        assert(packet->info.port);
        int serverfd = connection_create(fd, packet->info.ip, packet->info.port);
        if(serverfd < 0){
            return -1;
        } 
        // assert(!epoll_add(epollfd, fd, EPOLLOUT | EPOLLERR));
        set_fd_type(serverfd, SERVER);
    }
    int destfd = get_fd(fd);
    if((get_fd_type(destfd) & 0x2) != IN_EPOLL){
        assert(!epoll_add(epollfd, destfd, EPOLLOUT | EPOLLERR));
        set_fd_type(destfd, get_fd_type(destfd) | IN_EPOLL);
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
    }
    else{
        n = write(fd, packet->buf + packet->l, packet->r - packet->l);
        if(n < 0){
            return n;
        }
        packet->l += n;
        packet->size -= n;
    }
    return n;
}

static void _reinit_packet(Packet *packet){
    packet->buf_type = REQUEST_HEAD;
    packet->state = 0;
    packet->com_flag = NOT_COMPLETE;
    packet->info.length = -1;
    packet->info.chunked_size = 0;
}
int send_packet(int fd, int epollfd){
    int srcfd = get_fd(fd);
    assert(srcfd >= 0);
    assert(get_fd_type(srcfd) >= 0);
    Packet *packet = get_packet(srcfd, get_fd_type(srcfd) & 0x1) ;
    int n;
    n = _send(fd, packet);
    if(packet->size == 0 && packet->com_flag == COMPLETE){
        assert(!epoll_del(epollfd, srcfd, EPOLLIN | EPOLLERR));
        assert(!epoll_mod(epollfd, fd, EPOLLIN | EPOLLERR));
        set_fd_type(srcfd, get_fd_type(srcfd) & 0x1);
        _reinit_packet(packet);
        if(packet->client_server_flag == SERVER){
            Packet *clientpacket = get_packet(fd, get_fd_type(fd) & 0x1);
            connection_release(srcfd, clientpacket->info.ip, clientpacket->info.port);
        }
    }
    else if(packet->size == 0){
        assert(epoll_del(epollfd, fd, EPOLLOUT | EPOLLERR) == 0);
        set_fd_type(fd, get_fd_type(fd) & 0x1);
    }
    return n;















    // n = _send(get_fd(fd), packet);
    
    // if(packet->size == 0 && packet->com_flag == COMPLETE && packet->client_server_flag == SERVER){
    //     int client_fd = get_fd(fd);
    //     Packet *client_packet = get_packet(client_fd, get_fd_type(client_fd));
    //     printf("client_fd = %d, server_fd = %d",client_fd, fd);
    //     struct in_addr addr;
    //     memcpy(&addr, &client_packet->info.ip, sizeof(addr));
    //     printf(" ip == %s\n",inet_ntoa(addr));
    //     connection_release(fd, client_packet->info.ip, client_packet->info.port);
    // }
    // if(packet->size == 0 && packet->com_flag == COMPLETE){
        
    // }
  

}