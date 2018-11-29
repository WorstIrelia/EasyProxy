#ifndef _BUF_H_
#define _BUF_H_

#include "config.h"
#include "sys/types.h"
#define BUF_ADD_INDEX(buf, i) i++;if(i == (buf)->cap){i = 0;}

typedef struct buf{
    char *buf;
    int l, r;
    size_t size;
    size_t cap;
}Buf;

void buf_init(Buf *buf);

void buf_destory(Buf *buf);

void buf_extend(Buf *buf);

void buf_copy(Buf *dest_buf, char *src_buf, size_t size);


#endif