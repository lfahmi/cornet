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
    cn_ip4tos((char *)&fromaddr, pack->from.sin_addr.s_addr);
    printf("Received data from %s:%d\n", fromaddr, pack->from.sin_port);
    printf("data:%s\n", pack->buffer->workb);
    sendto(pack->listener->sock ,"Got your message\n",17, 0,(struct sockaddr *)&pack->from, pack->fromlen);
}

int main(int argc, char *argv[])
{
   in_addr_t ip4ni = inet_addr("127.0.2.1");
   uint32_t ip4ni2 = cn_stoip4("127.0.3.1");
   cn_ip4 *myip = cn_createip4(malloc(sizeof(cn_ip4)) ,&ip4ni2);
   printf("Regulated Cornet %s\n", myip->stringIp);
   struct in_addr *theaddr = malloc(sizeof(struct in_addr));
   theaddr->s_addr = ip4ni;
   printf("regulated %s\n", inet_ntoa(*theaddr));
   int sock, length, n;
   socklen_t fromlen;
   struct sockaddr_in server, from;
   char buf[1024];
   */

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

