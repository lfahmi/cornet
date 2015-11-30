#ifndef __CORNET_H__
#define __CORNET_H__    1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cornet/cnthread.h"

#define CN_SIZEOF_IP4STRLEN
#define CN_IP4STRLEN 16

typedef int (*cn_sockPacketHandler)(void *packet);

typedef struct
{
    char initialized;
    char node1;
    char node2;
    char node3;
    char node4;
    char stringIp[16];
    unsigned int intIp;
} cn_ip4;

typedef struct
{
    char initialized;
    unsigned int fd;
    pthread_t listenerThreadId;
    cn_ip4 ip4;
    int sock;
    struct sockaddr_in addr;
    cn_sockPacketHandler handler;
} cn_sock;

typedef struct
{
    char *b;
    char *workb;
    uint16_t perSize;
    uint16_t length;
    uint16_t capacity;
} cn_buffer;

typedef struct
{
    cn_sock *listener;
    struct sockaddr_in from;
    socklen_t fromlen;
    cn_buffer *buffer;
} cn_udpPacket;

/*
Create IPv4 binary type from string.
return 0 if failed.
*/
extern unsigned int cn_stoip4(char *__s);

extern char *cn_ip4tos(char CN_SIZEOF_IP4STRLEN *__dest, unsigned int __s);

extern cn_ip4 *cn_createip4(cn_ip4 *target, unsigned int *a);

extern cn_buffer *cn_createBuffer(uint16_t perSize, uint16_t length);

extern void cn_desBuffer(cn_buffer *target);


//MAIN.C

extern int cn_startUDPListener(cn_sock *fd, char *__ip4, uint16_t port, uint32_t bufferSize, cn_sockPacketHandler handler);

#endif
