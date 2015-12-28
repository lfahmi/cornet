#include "cornet.h"

#define UDP_SEND(asent, asocket, acn_buffer, asockaddr_in) \
    asent = sendto(asocket, acn_buffer->originalb, acn_buffer->originallength, 0, (const struct sockaddr *)&asockaddr_in, sizeof(struct sockaddr))

char _s_connect[] = "CONN";
char _s_sendTo[] = "TRGT";
char _s_thePing[] = "PING";
char _s_myId[] = "MYID";
char _s_nodePrefix[] = "client_";

cn_list *nodes;

typedef struct
{
    char cmd[4];
    char data[40];
} packStruct;

int handler(void *arg)
{
    cn_udpPacket *pack = arg;
    char fromaddr[CN_IP4STRLEN];
    fromaddr[0] = '\0';
    cn_inaddrToStr((char *)&fromaddr, pack->from.sin_addr.s_addr);
    printf("Received data from %s:%d\n", fromaddr, ntohs(pack->from.sin_port));
    printf("data:%s\n", pack->buffer->b);
    if(cn_bitIndexOf(pack->buffer->b, pack->buffer->length, _s_connect, sizeof(_s_connect) - 1) == 0)
    {
        cn_buffer *resbuf = cn_createBuffer(1, sizeof(packStruct));
        packStruct *resp = (void *)resbuf->originalb;
        resbuf->originallength = sizeof(packStruct);

        // Send Node ID
        cn_bitcp(resp->cmd, _s_myId, sizeof(_s_myId));
        cn_bitcp(resp->data, _s_nodePrefix, sizeof(_s_nodePrefix));
        time_t t = time(NULL);
        srand(t);
        sprintf(resp->data + sizeof(_s_nodePrefix) - 1, "%u", rand());
        UDP_SEND(int n, pack->listener->sock, resbuf, pack->from);
        printf("Send id %s as id for %s\n", resp->data, fromaddr);

        // Prepare SendTo command for nodes
        cn_bitzero(resp, sizeof(packStruct));
        cn_bitcp(resp->cmd, _s_sendTo, sizeof(_s_sendTo));
        cn_bitcp(resp->data, fromaddr, strlen(fromaddr));
        int ipstrlen = strlen(resp->data) - 1;
        sprintf(resp->data + ipstrlen + 1, ":%u|", ntohs(pack->from.sin_port));

        // Send SendTo command to nodes
        cn_buffer *buff2 = cn_createBuffer(1, sizeof(packStruct));
        packStruct *buff2b = (void *)buff2->originalb;
        buff2->originallength = sizeof(packStruct);
        struct sockaddr_in *addrs;
        CN_LIST_FOREACH(addrs, nodes)
        {
            UDP_SEND(n, pack->listener->sock, resbuf, addrs);

            // Prepare SendTo command for nodes
            cn_bitzero(buff2b, sizeof(packStruct));
            cn_bitcp(buff2b->cmd, _s_sendTo, sizeof(_s_sendTo));
            cn_inaddrToStr(buff2b->data ,addrs->sin_addr.s_addr);
            ipstrlen = strlen(buff2b->data) - 1;
            sprintf(buff2b->data + ipstrlen + 1, ":%u|", ntohs(addrs->sin_port));

            UDP_SEND(n, pack->listener->sock, buff2, pack->from);
        }

        // prepare Sendto command for server
        cn_bitzero(resp, sizeof(packStruct));
        cn_bitcp(resp->cmd, _s_sendTo, sizeof(_s_sendTo));
        cn_inaddrToStr(resp->data ,pack->listener->addr.sin_addr.s_addr);
        ipstrlen = strlen(resp->data) - 1;
        sprintf(resp->data + ipstrlen + 1, ":%d|", ntohs(pack->listener->addr.sin_port));
        UDP_SEND(n, pack->listener->sock, resbuf, pack->from);

        cn_listAppend(nodes, &pack->from);
    }
    else if(cn_bitIndexOf(pack->buffer->b, pack->buffer->length, _s_thePing, sizeof(_s_thePing) - 1) == 0)
    {
        printf("Got ping from %s\n", fromaddr);
    }
    cn_desBuffer(pack->buffer);
    free(pack);
    return 0;
}

int main(int argc, char *argv[])
{
    nodes = cn_makeList("nodes", sizeof(struct sockaddr_in), 10, false);

    printf("Starting UDP Operations Server\n");
    if (argc < 2) {
      printf("ERROR, no port provided\n");
      exit(0);
    }
    int port;
    char *ipaddress;
    if(argc == 2)
    {
        port = atoi(argv[1]);
        ipaddress = "0.0.0.0";
    }
    else if(argc == 3)
    {
        port = atoi(argv[2]);
        ipaddress = argv[1];
    }
    cn_sock udpsock;
    cn_startUDPListener(&udpsock, ipaddress, port, 1024, handler);
    while(1)
    {
        cn_sleep(1000);
    }
    return 0;
}
