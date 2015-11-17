#include "cornet/cornet.h"

typedef struct
{
    cn_sock *fd;
    uint32_t bufferSize;
    pthread_t threadId;
} cn_privListnerArg;

int cn_privNotifyPack(cn_sock *listenerFd, cn_netPacket *pack)
{
    pthread_mutex_lock(&(*listenerFd->recvHandlers).key);
    cn_sockHandler *handlers = *(*listenerFd->recvHandlers).b;
    uint32_t *cnt = &(*listenerFd->recvHandlers).cnt;
    int i;
    for(i = 0; i < *cnt; i++)
    {
        handlers[i](listenerFd, pack);
    }
    pthread_mutex_unlock(&(*listenerFd->recvHandlers).key);
    return 0;
}

void *cn_privlistener(void *arg)
{
    cn_privListnerArg *targ = arg;
    while(1)
    {
        cn_netPacket *pack = malloc(sizeof(cn_netPacket));
        pack->buffer = cn_createBuffer(1, targ->bufferSize);
        pack->fromlen = sizeof(struct sockaddr_in);
        char *buff = (*pack->buffer).b;
        uint16_t *len = &(*pack->buffer).len;
        *len = recvfrom((*targ->fd).sock, buff, targ->bufferSize, 0, (struct sockaddr *)&pack->from, &pack->fromlen);
        if(*len > 3)
        {
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
        }
        else
        {
            continue;
        }
    }
}

int cn_startUDPListener(cn_sock *fd, char *__ip4, uint16_t port, uint32_t bufferSize)
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
    fd->recvHandlers = cn_makeList(sizeof(cn_sockHandler), 10);
    cn_privListnerArg *arg = malloc(sizeof(cn_privListnerArg));
    arg->bufferSize = bufferSize;
    arg->fd = fd;
    arg->threadId = fd->listenerThreadId;
    pthread_create(&fd->listenerThreadId, 0, cn_privlistener, (void *)arg);
    return 0;
}

int cn_attachUDPHandler(cn_sock *fd, cn_sockHandler action)
{
    cn_sock *sock= fd;
    return cn_listAppend(sock->recvHandlers, action);
}
