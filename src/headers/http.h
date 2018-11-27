#ifndef _HTTP_H_
#define _HTTP_H_


typedef enum head_body{
    REQUEST_HEAD,
    REQUEST_BODY,
    DATA_BODY,
    HTTPS
}Head_body;

typedef enum complete{
    COMPLETE,
    NOT_COMPLETE
}Complete;

typedef enum connection{
    KEEP_ALIVE,
    CLOSE
}Connection;


#endif