#include "buf.h"

void buf_init(Buf *buf){
    buf->buf = (char*)malloc(BUFSIZE);
    assert(buf->buf);
    buf->l = buf->r = 0;
    buf->size = 0;
    buf->cap = BUFSIZE;
}
void buf_destory(Buf *buf){
    free(buf->buf);
}

void buf_extend(Buf *buf){
    char *tmp = (char *)malloc(buf->cap << 1);
    assert(tmp);
    int l = 0;
    for(;buf->l != buf->r;){
        tmp[l++] = buf->buf[buf->l];
        BUF_ADD_INDEX(buf, buf->l);
    }
    free(buf->buf);
    buf->buf = tmp;
    buf->cap = buf->cap << 1;
    buf->l = 0;
    buf->r = l;
}

void buf_copy(Buf *dest_buf, char *src_buf, size_t size){
    int left = dest_buf->cap - dest_buf->size - 1;
    while(left < size){
        buf_extend(dest_buf);
        left = dest_buf->cap - dest_buf->size - 1;
    }
    int l = 0;
    while(size--){
        dest_buf->buf[dest_buf->r] = src_buf[l++];
        BUF_ADD_INDEX(dest_buf, dest_buf->r);
        dest_buf->size++;
    }
}

void buf_clear(Buf *buf){
    buf->size = buf->l = buf->r = 0;
}