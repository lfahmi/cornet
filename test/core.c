#include "cornet.h"

int onConnect(cn_connection *conn)
{
    printf("%s:%u connected!\n", conn->remoteaddr.stringIp, ntohs(conn->remoteaddr.port));
    return 0;
}

int onFailedConnect(struct sockaddr_in *addr, int eventStatus)
{
    cn_sockaddr cnaddr;
    cn_sockaddrFrom_sockaddr_in(&cnaddr, addr);
    printf("%s:%u failed to connect!\n", cnaddr.stringIp, ntohs(cnaddr.port));
    return 0;
}

int handler(cn_connection *conn, cn_buffer *buffer)
{
    printf("received from %s:%u data: %s", conn->remoteaddr.stringIp, ntohs(conn->remoteaddr.port), buffer->b);
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        return -1;
    }
    cn_socket listener;
    if(*argv[1] == 's')
    {
        uint16_t port = atoi(argv[3]);
        printf("server mode port %u\n", port);
        cn_startUDPListener(&listener, argv[2], &port, 100, handler);
    }
    else
    {
        printf("client mode\n");
        cn_startUDPListener(&listener, "0.0.0.0", NULL, 100, handler);
        cn_socketConnect(&listener, argv[2], atoi(argv[3]), onConnect, onFailedConnect);
    }
    printf("sleeping\n");
    while(1)
    {
        cn_sleep(1000);
    }
}
