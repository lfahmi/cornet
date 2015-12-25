#include "cornet/cornet.h"

typedef struct
{
    cn_cornetsv *fd;
    uint32_t bufferSize;
    pthread_t threadId;
} cn_privListnerArg;

struct cn_udpPacket
{
    cn_cornetsv *listener;
    struct sockaddr_in from;
    socklen_t fromlen;
    cn_buffer *buffer;
};
typedef struct cn_udpPacket cn_udpPacket;

#define PBOF 170
#define PEOF 85
#define PCORNET_CONNECT 1
#define PCORNET_DATA 2
#define PCORNET_PING 3
#define PCORNET_DISCONNECT 4

#define UDP_SEND(asent, asocket, acn_buffer, asockaddr_in) \
    asent = sendto(asocket, acn_buffer->originalb, acn_buffer->originallength, 0, (const struct sockaddr *)&asockaddr_in, sizeof(struct sockaddr))

static void packetError(cn_udpPacket *pack)
{

}

static void *cornet_protocol(void *arg)
{
    cn_udpPacket *pack = arg;
    unsigned char *buffer = pack->buffer->b;
    pack->buffer->b += 2;
    pack->buffer->length -= 3;
    if(buffer[0] == PBOF && buffer[pack->buffer->length - 1] == PEOF)
    {
        cn_sockaddr addr;
        cn_sockaddrFrom_sockaddr_in(&addr, &pack->from);
        cn_cornetsv *listener = pack->listener;
        cn_connection *conn = cn_addrmapGet(listener->addrmap, &addr);
        conn++;
        switch(buffer[1])
        {
            case PCORNET_DATA:
                if(conn == NULL)
                {
                    packetError(pack);
                }
                else
                {
                    if(conn->handler(conn, pack->buffer) < 0)
                    {
                        packetError(pack);
                    }
                }
                break;
            case PCORNET_PING:
                if(conn == NULL)
                {
                    packetError(pack);
                }
                else
                {
                    conn->pingref = *((unsigned long *)buffer + 2);
                }
                break;
            case PCORNET_CONNECT:
                if(conn != NULL)
                {
                    packetError(pack);
                }
                else
                {
                    cn_connection *newconn = cn_makeConnection("cn_cornetsv.connection", listener, addr);
                    cn_listAppend(listener->conns, newconn);
                    cn_addrmapSet(listener->addrmap, &addr, newconn);
                    cn_connSend(newconn, pack->buffer);
                }
                break;
            case PCORNET_DISCONNECT:
                if(conn == NULL)
                {
                    packetError(pack);
                }
                else
                {
                    cn_connSend(conn, pack->buffer);
                    cn_addrmapRemove(listener->addrmap, &addr);
                    cn_desConnection(conn);
                }
                break;
            default:
                packetError(pack);
                break;
        }
    }
    else
    {
        packetError(pack);
    }
    cn_desBuffer(pack->buffer);
    free(pack);
    return NULL;
}

static void *cn_privlistener(void *arg)
{
    cn_privListnerArg *targ = arg;
    uint16_t bufferSize = targ->bufferSize;
    //cn_cornetsvPacketHandler handler = targ->fd->handler;
    int listenfd = targ->fd->sock;
    cn_threadpool *worker = targ->fd->worker;
    while(1)
    {
        cn_udpPacket *pack = malloc(sizeof(cn_udpPacket));
        pack->listener = targ->fd;
        pack->fromlen = sizeof(struct sockaddr_in);
        pack->buffer = cn_createBuffer(1, bufferSize);
        uint16_t *len = &pack->buffer->originallength;
        *len = recvfrom(listenfd, pack->buffer->b, bufferSize, 0, (struct sockaddr *)&pack->from, &pack->fromlen);
        pack->buffer->length = pack->buffer->originallength;
        cn_thplEnq(worker, cn_makeAction("cn_cornetsv.listner.packetProccessing", cornet_protocol, pack, NULL));
    }
    return NULL;
}

int cn_startUDPListener(cn_cornetsv *fd, char *ip4Str, uint16_t port, uint32_t bufferSize, cn_cornetsvPacketHandler handler)
{
    // Defensive code
    if(fd == NULL) {return -1;}
    // reset target
    bzero(fd, sizeof(fd));

    // Init Socket
    fd->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(cn_makeSockAddr(&fd->addr, ip4Str, port) < 0){return -1;}
    if (bind(fd->sock,(struct sockaddr *)&fd->addr, sizeof(fd->addr))<0){return -1;}
    fd->handler = handler;

    // Init worker
    int workerCount = 1;
    FILE *fp = popen("cat /proc/cpuinfo | grep processor | wc -l", "r");
    if(fp != NULL)
    {
        char tmpstr[10];
        if(fgets(tmpstr, sizeof(tmpstr), fp) != NULL)
        {
            workerCount = atoi(tmpstr);
        }
        pclose(fp);
    }
    fd->worker = cn_makeThpl("cn_cornetsv.worker", workerCount);

    // Init addrmap
    fd->addrmap = cn_makeAddrmap("cn_cornetsv.addrmap");

    // Create new thread for listener frame
    cn_privListnerArg *arg = malloc(sizeof(cn_privListnerArg));
    arg->bufferSize = bufferSize;
    arg->fd = fd;
    arg->threadId = fd->listenerThreadId;
    pthread_create(&fd->listenerThreadId, 0, cn_privlistener, (void *)arg);
    return 0;
}

int cn_connSend(cn_connection *conn, cn_buffer *buffer)
{
    UDP_SEND(int n, conn->listener->sock, buffer, conn->remoteaddr.sockaddr_in);
    //int n = sendto(conn->listener->sock, buffer->originalb, buffer->originallength, 0, (const struct sockaddr *)&conn->remoteaddr.sockaddr_in, sizeof(struct sockaddr));
    if(n == buffer->originallength){return 0;}
    return -1;
}
