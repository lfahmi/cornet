#include "cornet/cornet.h"

/* SOCKADDR */

in_addr_t cn_strToInaddr(char *ip4Str)
{
    in_addr_t result = 0;
    char spt = '.';
    char len = strlen(ip4Str);
    if(len > 16){return 0;}
    char i;
    char tmp[4];
    char tmpi = 0;
    char nodeAt = 0;
    for(i = 0; i < len; i++)
    {
        if(tmpi > 3){ return 0; }
        char cur = *(ip4Str + i);
        if(cur == spt)
        {
            if(nodeAt > 4){return 0;}
            *(tmp + tmpi) = '\0';
            int ipNode = atoi(tmp);
            result |= ipNode<<(nodeAt * 8);
            nodeAt++;
            tmpi = 0;
        }
        else if(i == (len - 1))
        {
            if(tmpi > 2){return 0;}
            *(tmp + tmpi) = cur;
            *(tmp + (tmpi + 1)) = '\0';
            int ipNode = atoi(tmp);
            result |= ipNode<<(nodeAt * 8);
            nodeAt++;
            tmpi = 0;
        }
        else
        {
            *(tmp + tmpi) = cur;
            tmpi++;
        }
    }
    return result;
}

int cn_sockaddrFrom_sockaddr_in(cn_sockaddr *target, struct sockaddr_in *addrt)
{
    if(target == NULL){return -1;}
    cn_sockaddr *result = target;
    in_addr_t *ip4addrt = &addrt->sin_addr.s_addr;
    result->node1 = (char)(0xFF&*ip4addrt);
    result->node2 = (char)((0xFF00&*ip4addrt)>>8);
    result->node3 = (char)((0xFF0000&*ip4addrt)>>16);
    result->node4 = (char)((0xFF000000&*ip4addrt)>>24);
    result->intIp = *ip4addrt;
    sprintf(result->stringIp, "%u.%u.%u.%u", result->node1, result->node2, result->node3, result->node4);
    result->port = addrt->sin_port;
    result->sockaddr_in = *addrt;
    return 0;
}

int cn_makeSockAddr(cn_sockaddr *target, char *ip4, uint16_t port)
{
    struct sockaddr_in tmpaddr;
    tmpaddr.sin_family = AF_INET;
    tmpaddr.sin_addr.s_addr = cn_strToInaddr(ip4);
    tmpaddr.sin_port = htons(port);
    return cn_sockaddrFrom_sockaddr_in(target, &tmpaddr);
}

/* BUFFER */

cn_buffer *cn_createBuffer(uint16_t perSize, uint16_t length)
{
    cn_buffer *result = malloc(sizeof(cn_buffer));
    result->originalb = malloc(length * perSize);
    result->b = result->originalb;
    result->perSize = perSize;
    result->length = 0;
    result->originallength = 0;
    result->capacity = length;
    pthread_mutex_init(&result->key, NULL);
    return result;
}

void cn_desBuffer(cn_buffer *target)
{
    pthread_mutex_destroy(&target->key);
    free(target->b);
    free(target);
}

/* CONNECTION */

cn_type_id_t cn_connection_type_id = 0;

cn_connection *cn_makeConnection(const char *refname, cn_socket *listener, cn_sockaddr remoteaddr)
{
    // Defensive code
    if(listener == NULL){return NULL;}

    // Allocate object structure in memory
    cn_connection *result = malloc(sizeof(cn_connection));
    if(result == NULL){return NULL;}

    result->refname = refname;
    result->listener = listener;
    result->remoteaddr = remoteaddr;
    result->handler = listener->handler;

    // if list type has no identifier, request identifier.
    if(cn_connection_type_id < 1){cn_connection_type_id = cn_typeGetNewID();}

    // initialize object type definition
    cn_typeInit(&result->t, cn_connection_type_id);

    return result;
}

int cn_desConnection(cn_connection *conn)
{
    // Defensive code
    if(conn == NULL){return -1;}

    // Destroy object type definition
    cn_typeDestroy(&conn->t);

    // Deallocate object structure from memory
    free(conn);

    return 0;
}
