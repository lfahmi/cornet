#include "cornet/cnbitop.h"

int cn_bitcp(char *dest, char* src, int len)
{
    if(dest == NULL || src == NULL) {return -1;}
    int i, nexti = 0;
    if(len >= 8)
    {
        int len8 = len / 8;
        uint64_t *lsrc = (uint64_t *)src, *ldest = (uint64_t *)dest;
        for(i = 0; i < len8; i++)
        {
            ldest[i] = lsrc[i];
        }
        nexti = len8 * 8;
    }
    if(nexti < len)
    {
        for(i = nexti; i < len; i++)
        {
            dest[i] = src[i];
        }
    }
    return 0;
}

bool cn_bitcompare(char *objA, char* objB, int len)
{
    if(objA == NULL || objB == NULL) {return -1;}
    int i, nexti = 0;
    if(len >= 8)
    {
        int len8 = len / 8;
        uint64_t *lsrc = (uint64_t *)objB, *ldest = (uint64_t *)objA;
        for(i = 0; i < len8; i++)
        {
            if(ldest[i] != lsrc[i]){return false;}
        }
        nexti = len8 * 8;
    }
    if(nexti < len)
    {
        for(i = nexti; i < len; i++)
        {
            if(objA[i] != objB[i]){return false;}
        }
    }
    return true;
}
