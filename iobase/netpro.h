#ifndef _NET_PRO_H
#define _NET_PRO_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

typedef struct sockaddr SA;

//#define bool char
#define true 1
#define false 0

#define MAXLINE 1024
#define MAXBUF 8092

#endif
