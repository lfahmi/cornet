#include "cornet/cornet.h"

unsigned int cn_stoip4(char *__s)
{
    unsigned int result = 0;
    char spt = '.';
    int len = strlen(__s);
    int i;
    char tmp[4];
    int tmpi = 0;
    int nodeAt = 0;
    for(i = 0; i < len; i++)
    {
        if(tmpi > 3){ return 0; }
        char cur = *(__s + i);
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

char *cn_ip4tos(char *__dest, unsigned int __s)
{
    cn_ip4 tmp;
    cn_ip4 *ip = cn_createip4(&tmp, &__s);
    strcpy(__dest, ip->stringIp);
    return __dest;
}

cn_ip4 *cn_createip4(cn_ip4 *target, unsigned int *a)
{
    cn_ip4 *result = target;
    //if(result == NULL){return (cn_ip4)-1;}
    result->node1 = (char)(0xFF&*a);
    result->node2 = (char)((0xFF00&*a)>>8);
    result->node3 = (char)((0xFF0000&*a)>>16);
    result->node4 = (char)((0xFF000000&*a)>>24);
    result->intIp = *a;
    //if(result.stringIp == NULL){return -1;}
    sprintf(result->stringIp, "%hd.%hd.%hd.%hd", (short)result->node1, (short)result->node2, (short)result->node3, (short)result->node4);
    result->initialized = 1;
    return result;
}

cn_buffer *cn_createBuffer(uint16_t _size, uint16_t len)
{
    cn_buffer *result = malloc(sizeof(cn_buffer));
    result->b = malloc(len * _size);
    result->perSize = _size;
    result->len = len;
    return result;
}

void cn_desBuffer(cn_buffer *target)
{
    free(target->b);
    free(target);
}
