#include "cornet/cornet.h"

typedef struct
{
    cn_sock *fd;
    uint32_t bufferSize;
    pthread_t threadId;
} cn_privListnerArg;

static void *cn_privlistener(void *arg)
{
    cn_privListnerArg *targ = arg;
    uint16_t bufferSize = targ->bufferSize;
    cn_sockPacketHandler handler = targ->fd->handler;
    int listenfd = targ->fd->sock;
    while(1)
    {
        cn_udpPacket *pack = malloc(sizeof(cn_udpPacket));
        pack->listener = targ->fd;
        pack->fromlen = sizeof(struct sockaddr_in);
        pack->buffer = cn_createBuffer(1, bufferSize);
        uint16_t *len = &pack->buffer->length;
        *len = recvfrom(listenfd, pack->buffer->b, bufferSize, 0, (struct sockaddr *)&pack->from, &pack->fromlen);
        handler(pack);
    }
    return NULL;
}

int cn_startUDPListener(cn_sock *fd, char *ip4Str, uint16_t port, uint32_t bufferSize, cn_sockPacketHandler handler)
{
    if(fd == NULL) {return -1;}
    bzero(fd, sizeof(fd));
    fd->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(cn_bitcompare(ip4Str, "0.0.0.0", 7))
    {
        fd->addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        fd->addr.sin_addr.s_addr = cn_strToInaddr(ip4Str);
    }
    if(fd->addr.sin_addr.s_addr == 0) {return -1;}
    fd->addr.sin_family = AF_INET;
    fd->addr.sin_port = htons(port);
    if (bind(fd->sock,(struct sockaddr *)&fd->addr, sizeof(fd->addr))<0)
    {
        return -1;
    }
    fd->handler = handler;
    cn_privListnerArg *arg = malloc(sizeof(cn_privListnerArg));
    arg->bufferSize = bufferSize;
    arg->fd = fd;
    arg->threadId = fd->listenerThreadId;
    pthread_create(&fd->listenerThreadId, 0, cn_privlistener, (void *)arg);
    return 0;
}
