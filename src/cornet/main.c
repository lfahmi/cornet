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
        if(*len > 3)
        {
            handler(pack);
            /*
            char lastByte = buff[*len - 1];
            if(buff[0] == 127 && lastByte == 127)
            {
                if(buff[1] == 200)
                {
                    cn_privNotifyPack(targ->fd, pack);
                }
                else if(buff[1] == 199)
                {}
                else if(buff[1] == 205)
                {}
                else if(buff[1] == 404)
                {}
            }
            */
        }
        else
        {
            continue;
        }
    }
    return NULL;
}

int cn_startUDPListener(cn_sock *fd, char *__ip4, uint16_t port, uint32_t bufferSize, cn_sockPacketHandler handler)
{
    if(fd == NULL) {return -1;}
    bzero(fd, sizeof(fd));
    fd->sock = socket(AF_INET, SOCK_DGRAM, 0);
    fd->addr.sin_addr.s_addr = cn_stoip4(__ip4);
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
