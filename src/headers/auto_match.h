#ifndef _AUTO_MATCH_H_
#define _AUTO_MATCH_H_
#include "packet.h"
#include "netlib.h"
#include "http.h"
#include <stdlib.h>
#include <stdio.h>


int auto_match(Packet *packet, char *buf, int n);


#endif