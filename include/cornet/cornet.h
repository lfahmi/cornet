#ifndef __CORNET_H__
#define __CORNET_H__    1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cornet/cntype.h"
#include "cornet/cnthread.h"

#define CN_SIZEOF_IP4STRLEN
#define cn_sockaddrSTRLEN 16

// forward declaration
struct cn_connection;
struct cn_buffer;

/** packet handler */
typedef int (*cn_socketPacketHandler)(struct cn_connection *conn, struct cn_buffer *buffer);

typedef int (*cn_socketConnectionEvent)(struct cn_connection *conn);

typedef int (*cn_socketAddressEvent)(struct sockaddr_in *addr, int EVENT_STATUS_CODE);

// ENUMS

enum CN_SOCK_TRY_CONN_FAIL{SID = 1, NORESPONSE = 2}

/* buffer.c */

struct cn_buffer
{
    unsigned char *b;
    unsigned char *originalb;
    uint16_t perSize;
    uint16_t length;
    uint16_t originallength;
    uint16_t capacity;
    pthread_mutex_t key;
};
typedef struct cn_buffer cn_buffer;

/* addrmap.c */

struct cn_addrmap
{
    const char *refname;
    cn_dictionary4b **dics;
};
typedef struct cn_addrmap cn_addrmap;

/* util.c */

struct cn_sockaddr
{
    unsigned char node1;
    unsigned char node2;
    unsigned char node3;
    unsigned char node4;
    unsigned int intIp;
    char stringIp[16];
    in_port_t port;
    struct sockaddr_in sockaddr_in;
};
typedef struct cn_sockaddr cn_sockaddr;

/* main.c */

/**
    * Cornet Listener object structure definition
    * optimized network protocol for lightweight and speed
    */
struct cn_socket
{
    pthread_t listenerThreadId;
    cn_sockaddr addr;
    int sock;
    cn_socketPacketHandler handler;
    cn_socketConnectionEvent onConnect;
    cn_socketConnectionEvent onDisconnect;
    cn_threadpool *worker;
    cn_list *conns;
    cn_addrmap *addrmap;
    cn_list *tryConnect;
};
typedef struct cn_socket cn_socket;

/* util.c */

/// type id for connection object
extern cn_type_id_t cn_connection_type_id;

/**
    * connection object structure definition
    * this object is used in packet handler
    * and send operation
    */
struct cn_connection
{
    cn_type t;
    const char *refname;
    uint16_t connSessionID;
    cn_socket *listener;
    cn_sockaddr remoteaddr;
    uint32_t pingref;
    int pingStatus;
    cn_socketPacketHandler handler;
};
typedef struct cn_connection cn_connection;

/*
struct cn_cornetPacket
{
    cn_connection *conn;
    cn_buffer *buffer;
};
typedef struct cn_cornetPacket cn_cornetPacket;
*/

/* END OF DEFINITIONS */

/* buffer.c */

extern cn_buffer *cn_createBuffer(uint16_t perSize, uint16_t length);

extern void cn_desBuffer(cn_buffer *target);

/* addrmap.c */

extern cn_addrmap *cn_makeAddrmap(const char *refname);

extern int cn_addrmapSet(cn_addrmap *addrmap, cn_sockaddr *ip, void *object);

extern void *cn_addrmapGet(cn_addrmap *taddrmap, cn_sockaddr *addr);

extern int cn_addrmapRemove(cn_addrmap *taddrmap, cn_sockaddr *addr);

/* sockaddr utils.c */

/*
Create IPv4 binary type from string.
return 0 if failed.
*/
extern in_addr_t cn_strToInaddr(char *ip4Str);

extern int cn_sockaddrFrom_sockaddr_in(cn_sockaddr *target, struct sockaddr_in *addrt);

extern int cn_makeSockAddr(cn_sockaddr *target, char *ip4, uint16_t port);

/* CONNECTION utils.c */

extern cn_connection *cn_makeConnection(const char *refname, cn_socket *listener, cn_sockaddr remoteaddr);

extern int cn_desConnection(cn_connection *conn);


/* main.c */

extern int cn_startUDPListener(cn_socket *fd, char *__ip4, uint16_t *port, uint32_t bufferSize, cn_socketPacketHandler handler);

extern int cn_connSend(cn_connection *conn, cn_buffer *buffer);

// client.c

#endif
