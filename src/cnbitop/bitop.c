#include "cornet/cnbitop.h"

int cn_bitcp(void *dest, void* src, int len)
{
    if(dest == NULL || src == NULL || len < 1) {return -1;}
    int nextIndex = 0;
    if(len >= 8)
    {
        uint64_t *tsrc = src, *tdest = dest;
        while(len >= 8)
        {
            *tdest = *tsrc;
            tdest++;
            tsrc++;
            len -= 8;
            nextIndex += 8;
        }
    }
    if(len >= 4)
    {
        uint32_t *tsrc = src + nextIndex, *tdest = dest + nextIndex;
        while(len >= 4)
        {
            *tdest = *tsrc;
            tdest++;
            tsrc++;
            len -= 4;
            nextIndex += 4;
        }
    }
    if(len >= 1)
    {
        char *tsrc = src + nextIndex, *tdest = dest + nextIndex;
        while(len >= 1)
        {
            *tdest = *tsrc;
            tdest++;
            tsrc++;
            len -= 1;
            nextIndex += 1;
        }
    }
    return 0;
}

bool cn_bitcompare(void *objA, void* objB, int len)
{
    if(objA == NULL || objA == NULL || len < 1) {return -1;}
    if(*(char *)objA != *(char *)objB){return false;}
    int nextIndex = 0;
    if(len >= 8)
    {
        uint64_t *tobjA = objA, *tobjB = objB;
        while(len >= 8)
        {
            if(*tobjB != *tobjA){return false;}
            tobjB++;
            tobjA++;
            len -= 8;
            nextIndex += 8;
        }
    }
    if(len >= 4)
    {
        uint32_t *tobjA = objA + nextIndex, *tobjB = objB + nextIndex;
        while(len >= 4)
        {
            if(*tobjB != *tobjA){return false;}
            tobjB++;
            tobjA++;
            len -= 4;
            nextIndex += 4;
        }
    }
    if(len >= 1)
    {
        char *tobjA = objA + nextIndex, *tobjB = objB + nextIndex;
        while(len >= 1)
        {
            if(*tobjB != *tobjA){return false;}
            tobjB++;
            tobjA++;
            len -= 1;
            nextIndex += 1;
        }
    }
    return true;
}

int cn_bitzero(void *target, int len)
{
    if(target == NULL || len < 1) {return -1;}
    int nextIndex = 0;
    if(len >= 8)
    {
        uint64_t *ttarget = target;
        while(len >= 8)
        {
            *ttarget = 0;
            ttarget++;
            len -= 8;
            nextIndex += 8;
        }
    }
    if(len >= 4)
    {
        uint32_t *ttarget = target + nextIndex;
        while(len >= 4)
        {
            *ttarget = 0;
            ttarget++;
            len -= 4;
            nextIndex += 4;
        }
    }
    if(len >= 1)
    {
        char *ttarget = target + nextIndex;
        while(len >= 1)
        {
            *ttarget = 0;
            ttarget++;
            len -= 1;
            nextIndex += 1;
        }
    }
    return 0;
}

int cn_bitIndexOf(void *target, int tlen, void *item, int ilen)
{
    if(target == NULL || item == NULL || tlen < 1 || ilen < 1 || tlen < ilen)
    {return -2;}
    int i, limit = tlen - ilen;
    target--;
    for(i = 0; i <= limit; i++)
    {
        if(*(char *)++target == *(char *)item)
        {
            if(cn_bitcompare(target, item, ilen))
            {return i;}
        }
    }
    return -1;
}

int cn_bitStrLen(char *str)
{
    int i = 1;
    while(*str != 0)
    {
        str++;
        i++;
    }
    return i;
}
