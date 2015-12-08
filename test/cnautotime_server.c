/* Creates a datagram server.  The port
   number is passed as an argument.  This
   server runs forever */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "cornet.h"
#include <time.h>

#define MEDSTRLEN 102

char _s_getTimeRequest[] = "CNOPERATION_GET_UTC_TIME";

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int handler(void *arg)
{
    cn_udpPacket *pack = arg;
    char fromaddr[CN_IP4STRLEN];
    fromaddr[0] = '\0';
    cn_inaddrToStr((char *)&fromaddr, pack->from.sin_addr.s_addr);
    printf("Received data from %s:%d\n", fromaddr, pack->from.sin_port);
    printf("data:%s\n", pack->buffer->b);
    if(cn_bitIndexOf(pack->buffer->b, pack->buffer->length, _s_getTimeRequest, sizeof(_s_getTimeRequest) - 1) == 0)
    {
        printf("sending utc time\n");
        time_t t = time(NULL);
        sendto(pack->listener->sock , &t, sizeof(time_t), 0,(struct sockaddr *)&pack->from, pack->fromlen);
    }
    cn_desBuffer(pack->buffer);
    free(pack);
    return 0;
}

int main(int argc, char *argv[])
{
    char *kokowa = malloc(100);
    cn_inaddrToStr(kokowa, cn_strToInaddr("255.255.255.255"));
    printf("%s\n", kokowa);
   if (argc < 2) {
      printf("ERROR, no port provided\n");
      exit(0);
   }
   cn_sock udpsock;
   cn_startUDPListener(&udpsock, "127.0.0.1", atoi(argv[1]), 1024, handler);
   while(1)
   {
        cn_sleep(1000);
   }
   return 0;
 }

