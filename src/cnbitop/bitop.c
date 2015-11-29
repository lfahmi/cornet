#include "cornet/cnbitop.h"

int cn_bitcp(void *dest, void* src, int len)
{
    if(dest == NULL || src == NULL) {return -1;}
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

bool cn_bitcompare(char *objA, char* objB, int len)
{
    if(objA == NULL || objA == NULL) {return -1;}
    if(*objA != *objB){return false;}
    void *ptrA = objA, *ptrB = objB;
    int nextIndex = 0;
    if(len >= 8)
    {
        uint64_t *tptrA = ptrA, *tptrB = ptrB;
        while(len >= 8)
        {
            if(*tptrB != *tptrA){return false;}
            tptrB++;
            tptrA++;
            len -= 8;
            nextIndex += 8;
        }
    }
    if(len >= 4)
    {
        uint32_t *tptrA = ptrA + nextIndex, *tptrB = ptrB + nextIndex;
        while(len >= 4)
        {
            if(*tptrB != *tptrA){return false;}
            tptrB++;
            tptrA++;
            len -= 4;
            nextIndex += 4;
        }
    }
    if(len >= 1)
    {
        char *tptrA = ptrA + nextIndex, *tptrB = ptrB + nextIndex;
        while(len >= 1)
        {
            if(*tptrB != *tptrA){return false;}
            tptrB++;
            tptrA++;
            len -= 1;
            nextIndex += 1;
        }
    }
    return true;
}
