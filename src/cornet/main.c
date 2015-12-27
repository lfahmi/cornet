#include "cornet/cornet.h"

typedef struct
{
    cn_socket *fd;
    uint32_t bufferSize;
    pthread_t threadId;
} cn_privListnerArg;

struct cn_udpPacket
{
    cn_socket *listener;
    struct sockaddr_in from;
    socklen_t fromlen;
    cn_buffer *buffer;
};
typedef struct cn_udpPacket cn_udpPacket;

struct cn_tryConnectObj
{
    cn_socket *listener;
    struct sockaddr_in addr;
    uint16_t connSessionID;
    cn_socketConnectionEvent onConnected;
    cn_socketAddressEvent onFailed;
};
typedef struct cn_tryConnectObj cn_tryConnectObj;

#define ILock(key) pthread_mutex_lock(key)
#define IUnlock(key) pthread_mutex_unlock(key)

#define PBOF 170
#define PEOF 85
#define PCORNET_PTYPE_CONNECT 1
#define PCORNET_PTYPE_DATA 2
#define PCORNET_PTYPE_PING 3
#define PCORNET_PTYPE_DISCONNECT 4

#define PCORNET_PTYPE_CONNECT_LENGTH 7

#define CORNET_PROTOCOL_VERSION 2

#define PCORNET_PACKET_BOF 0
#define PCORNET_PACKET_VER 1
#define PCORNET_PACKET_TYPE 3
#define PCORNET_PACKET_DATA 4
#define PCORNET_PACKET_EOF 1
#define PCORNET_PACKET_MINLEN 5

#define UDP_SEND(asocket, acn_buffer, asockaddr_in) \
    sendto(asocket, acn_buffer->originalb, acn_buffer->originallength, 0, (const struct sockaddr *)asockaddr_in, sizeof(struct sockaddr))

#define UDP_NSEND(asent, asocket, acn_buffer, asockaddr_in) \
    asent = UDP_SEND(asocket, acn_buffer, asockaddr_in)

static void packetError(cn_udpPacket *pack)
{

}

static void *cornet_protocol(void *arg)
{
    cn_udpPacket *pack = arg;
    cn_buffer *buffobj = pack->buffer;
    unsigned char *buffer = buffobj->b;
    if(buffobj->length >= PCORNET_PACKET_MINLEN && buffer[PCORNET_PACKET_BOF] == PBOF &&
        *((unsigned short *)(buffer + PCORNET_PACKET_VER)) ==  CORNET_PROTOCOL_VERSION &&
        buffer[pack->buffer->length - PCORNET_PACKET_EOF] == PEOF)
    {
        cn_sockaddr addr;
        cn_sockaddrFrom_sockaddr_in(&addr, &pack->from);
        cn_socket *listener = pack->listener;
        cn_connection *conn = cn_addrmapGet(listener->addrmap, &addr);

        switch(buffer[PCORNET_PACKET_TYPE])
        {
            case PCORNET_PTYPE_DATA:
                if(conn == NULL)
                {
                    packetError(pack);
                }
                else
                {
                    buffobj->b += PCORNET_PACKET_DATA;
                    buffobj->length -= PCORNET_PACKET_MINLEN;
                    if(conn->handler(conn, pack->buffer) < 0)
                    {
                        packetError(pack);
                    }
                }
                break;
            case PCORNET_PTYPE_PING:
                if(conn == NULL)
                {
                    packetError(pack);
                }
                else
                {
                    conn->pingref = *((uint32_t *)buffer + PCORNET_PACKET_DATA);
                }
                break;
            case PCORNET_PTYPE_CONNECT:;
                // Parse received data
                uint16_t connectionSessionID;
                connectionSessionID = *((uint16_t *)buffer + PCORNET_PACKET_DATA);

                /*  if there is already connection with the same address
                    and connection Session ID is same, it's likely that our
                    our confirmation packet was not sent, resent it and quit the switch.
                    but if the Session ID is not match, call disconnect callback and
                    continue operation */
                // HAS FINAL POINT !EP436
                if(conn != NULL)
                {
                    if(conn->connSessionID == connectionSessionID)
                    {
                        // Send confirmation and break the packet switch
                        UDP_SEND(listener->sock, pack->buffer, &pack->from);
                        packetError(pack);
                        break; // !EP436
                    }
                    else
                    {
                        listener->onDisconnect(conn);
                    }
                }

                // Determine wether addr is in try connect
                // if it is, copy and remove it from try connect
                bool inTryConn = false;
                cn_list *lTryConn = listener->tryConnect;
                ILock(&lTryConn->key);
                cn_tryConnectObj tryconn;
                {
                    cn_tryConnectObj *item;
                    CN_LIST_FOREACH(item, lTryConn)
                    {
                        if(item->addr.sin_addr.s_addr == addr.sockaddr_in.sin_addr.s_addr &&
                            item->addr.sin_port == addr.sockaddr_in.sin_port)
                        {
                            inTryConn = true;
                            tryconn = *item;
                            cn_listRemoveAtULOCK(lTryConn, (((void *)item - *lTryConn->b) / lTryConn->perSize));
                            break;
                        }
                    }
                }
                IUnlock(&lTryConn->key);

                /*  we tried to connect to this address but we didnt get the
                    confirmation for our connection session id.
                    we will treat this as an error and quit the switch. */
                // HAS FINAL POINT !EP987
                if(inTryConn)
                {
                    if(tryconn.connSessionID != connectionSessionID)
                    {
                        tryconn.onFailed(&pack->from, CN_SOCK_TRY_CONN_FAIL_SID);
                        packetError(pack);
                        break; // !EP987
                    }
                }
                else
                {
                    UDP_SEND(listener->sock, pack->buffer, &pack->from);
                }

                /*  make new connection object and add it to address map and conn list */
                cn_connection *newconn = cn_makeConnection("cn_socket.connection", listener, addr);
                newconn->connSessionID = *((uint16_t *)buffer + PCORNET_PACKET_DATA);
                cn_listAppend(listener->conns, newconn);
                cn_addrmapSet(listener->addrmap, &addr, newconn);
                cn_connSend(newconn, pack->buffer);

                /*  if we are the proposer then call try connect success callback
                    else call listener new connection callback */
                if(inTryConn)
                {if(tryconn.onConnected != NULL){tryconn.onConnected(newconn);}}
                else
                {if(listener->onConnect != NULL){listener->onConnect(newconn);}}
                break;
            case PCORNET_PTYPE_DISCONNECT:
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
        cn_thplEnq(worker, cn_makeAction("cn_socket.listner.packetProccessing", cornet_protocol, pack, NULL));
    }
    return NULL;
}

int cn_startUDPListener(cn_socket *fd, char *ip4Str, uint16_t *port, uint32_t bufferSize, cn_socketPacketHandler handler)
{
    // Defensive code
    if(fd == NULL) {return -1;}
    // reset target
    bzero(fd, sizeof(fd));

    // Init Socket
    fd->sock = socket(AF_INET, SOCK_DGRAM, 0);
    uint16_t theport;
    if(port == NULL)
    {
        srand((uint32_t)time(NULL));
        theport = rand() % 50000;
    }
    else
    {
        theport = *port;
    }
    if(cn_makeSockAddr(&fd->addr, ip4Str, theport) < 0){return -1;}
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
    fd->worker = cn_makeThpl("cn_socket.worker", workerCount);

    // Init addrmap
    fd->addrmap = cn_makeAddrmap("cn_socket.addrmap");

    // Init tryConnect
    fd->tryConnect = cn_makeList("cn_socket.tryConnect", sizeof(cn_tryConnectObj), 10, false);

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
    UDP_NSEND(int n, conn->listener->sock, buffer, &conn->remoteaddr.sockaddr_in);
    if(n == buffer->originallength){return 0;}
    return -1;
}

static void *checkConnectAttemptStatus(void *arg)
{
    // check wether tryConnect contained in cn_sock.tryConnect
    // If not exist, quit
    cn_tryConnectObj tryconn = *((cn_tryConnectObj *)arg);
    bool inTryConn = false;
    cn_list *lTryConn = tryconn.listener->tryConnect;
    ILock(&lTryConn->key);
    {
        cn_tryConnectObj *item;
        CN_LIST_FOREACH(item, lTryConn)
        {
            if(item->addr.sin_addr.s_addr == tryconn.addr.sin_addr.s_addr &&
                item->addr.sin_port == tryconn.addr.sin_port)
            {
                inTryConn = true;
                tryconn = *item;
                cn_listRemoveAtULOCK(lTryConn, (((void *)item - *lTryConn->b) / lTryConn->perSize));
                break;
            }
        }
    }
    IUnlock(&lTryConn->key);
    if(!inTryConn){return NULL;}

    // try connect is inside cn_sock.tryConnect
    // check wether addr is already in address map of cn_sock
    // if it was not inside, then call failure callback.
    cn_sockaddr cnsockaddr;
    cn_sockaddrFrom_sockaddr_in(&cnsockaddr, &tryconn.addr);
    if(cn_addrmapGet(tryconn.listener->addrmap, &cnsockaddr) == NULL)
    {tryconn.onFailed(&tryconn.addr, CN_SOCK_TRY_CONN_FAIL_NORESPONSE);}
    return NULL;
}

int cn_socketConnect(cn_socket *sock, char * ip4, int port, cn_socketConnectionEvent onSuccess, cn_socketAddressEvent onFailed)
{
    /*  Create tryConnectobj and add to socket.tryConnect */
    cn_tryConnectObj *tryconn = malloc(sizeof(cn_tryConnectObj));
    tryconn->listener = sock;
    tryconn->addr.sin_addr.s_addr = cn_strToInaddr(ip4);
    tryconn->addr.sin_port = htons(port);
    tryconn->onConnected = onSuccess;
    tryconn->onFailed = onFailed;
    cn_listAppend(sock->tryConnect, tryconn);

    /* add delayed action to scheduler for checking try connect attempt status. */
    cn_doDelayedAction(cn_makeAction("checkConnectAttemptStatus", checkConnectAttemptStatus, tryconn, cn_cnnumDefaultItemDestructor), 30);

    /* send PCORNET_CONNECT packet to remote addr */
    cn_buffer *buff = cn_createBuffer(1, PCORNET_PTYPE_CONNECT_LENGTH);
    unsigned char *buffer = buff->originalb;
    buffer[PCORNET_PACKET_BOF] = PBOF;
    buffer[PCORNET_PACKET_VER] = CORNET_PROTOCOL_VERSION;
    buffer[PCORNET_PACKET_TYPE] = PCORNET_PTYPE_CONNECT;
    srand(time(NULL));
    uint16_t *connRef = (void *)buffer + PCORNET_PACKET_DATA;
    *connRef = (uint16_t)rand();
    buffer[PCORNET_PTYPE_CONNECT_LENGTH - PCORNET_PACKET_EOF] = PEOF;
    UDP_NSEND(int n, sock->sock, buff, &tryconn->addr);
    if(n != buff->originallength){return -1;}
    return 0;
}
