#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int connectUdp(int *sock, struct sockaddr_in *server, char *host, int port)
{
    *sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(*sock == 0){return -1;}
    server->sin_family = AF_INET;
    struct hostent *hp;
    hp = gethostbyname(host);
    if (hp==0) return -2;
    bcopy((char *)hp->h_addr, (char *)&server->sin_addr, hp->h_length);
    server->sin_port = htons(port);
    return 0;
}

int main(int argc, char *argv[])
{

    if(argc < 3){return 0;}

    int n, sock;
    struct sockaddr_in server, from;
    n = connectUdp(&sock, &server, argv[1], atoi(argv[2]));
    if(n < 0){printf("error connectudp %d\n", n); return 1;}

    char buffer[1024];
    bzero(buffer, 1024);
    strcpy(buffer, "CNOPERATION_GET_UTC_TIME");

    unsigned int stlen = sizeof(struct sockaddr);
    n = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *)&server, stlen);
    if(n < 0){printf("error send %d\n", n); return 1;}

    printf("requesting UTC time ....\n");

    n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &stlen);
    if(n < 0){printf("error recvfrom %d\n", n); return 1;}

    printf("got respond!\n");

    time_t *t = (void *)buffer;

    struct tm srvt;
    localtime_r(t, &srvt);
    long hour = (srvt.tm_gmtoff / 60) / 60;
    printf("gmt offset %ldh\n", hour);

    char outstr[200];
    strftime(outstr, sizeof(outstr), "%a, %d %b %y %T %z", &srvt);
    printf("%s\n", outstr);
    if(stime(t) < 0){printf("failed to set the time.\n");}

    return 0;
}
