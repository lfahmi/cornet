#include "cornet/cornet.h"

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

int cn_inaddrToStr(char *__dest, in_addr_t ip4addrt)
{
    if(__dest == NULL){return -1;}
    cn_ip4 tmp;
    cn_createip4(&tmp, &ip4addrt);
    strcpy(__dest, tmp.stringIp);
    return 0;
}

int cn_createip4(cn_ip4 *target, in_addr_t *ip4addrt)
{
    if(target == NULL){return -1;}
    cn_ip4 *result = target;
    result->node1 = (char)(0xFF&*ip4addrt);
    result->node2 = (char)((0xFF00&*ip4addrt)>>8);
    result->node3 = (char)((0xFF0000&*ip4addrt)>>16);
    result->node4 = (char)((0xFF000000&*ip4addrt)>>24);
    result->intIp = *ip4addrt;
    sprintf(result->stringIp, "%u.%u.%u.%u", result->node1, result->node2, result->node3, result->node4);
    result->initialized = 1;
    return 0;
}

cn_buffer *cn_createBuffer(uint16_t perSize, uint16_t length)
{
    cn_buffer *result = malloc(sizeof(cn_buffer));
    result->b = malloc(length * perSize);
    result->workb = result->b;
    result->perSize = perSize;
    result->length = 0;
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
