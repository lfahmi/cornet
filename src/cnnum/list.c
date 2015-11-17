#include "cornet/cnnum.h"

cn_list *cn_makeList(uint16_t perSize, uint32_t firstMaxCnt, bool appendTheAddress)
{
    if(perSize == 0 || firstMaxCnt == 0) {return NULL;}
    cn_list *tlist = malloc(sizeof(cn_list));
    if(tlist == NULL) {return NULL;}
    tlist->perSize = perSize;
    tlist->blen = firstMaxCnt * perSize;
    tlist->maxCnt = firstMaxCnt;
    tlist->b = malloc(sizeof(*tlist->b));
    *tlist->b = malloc(tlist->blen);
    tlist->appendTheAddress = appendTheAddress;
    if(*tlist->b == NULL) {free(tlist);return NULL;}
    tlist->cnt = 0;
    if(pthread_mutex_init(&tlist->key, NULL) != 0)
    {
        free(*tlist->b);
        free(tlist->b);
        free(tlist);
        return NULL;
    }
    return tlist;
}

int cn_desList(cn_list *tlist)
{
    if(tlist == NULL) { return -1; }
    pthread_mutex_lock(&tlist->key);
    free(*tlist->b);
    free(tlist->b);
    pthread_mutex_unlock(&tlist->key);
    pthread_mutex_destroy(&tlist->key);
    if(tlist != NULL){free(tlist);}
    #ifdef CN_DEBUG_MODE
    cn_log("desList finished for %d\n", (int)tlist);
    #endif // CN_DEBUG_MODE
    return 0;
}


int cn_listAppend(cn_list *tlist, void *item)
{
    if(tlist == NULL || item == NULL)
    {
        return -1;
    }
    if(tlist->cnt >= tlist->maxCnt)
    {
        if(cn_listExpand(tlist, tlist->cnt) < 0) {return -1;}
    }
    pthread_mutex_lock(&tlist->key);
    int n = 0;
    if(tlist->appendTheAddress == 1)
    {
        void **tmpPtr = malloc(sizeof(void *));
        if(tmpPtr == NULL){goto unlock_reterr;}
        *tmpPtr = item;
        n = cn_bitcp((*tlist->b + (tlist->cnt * tlist->perSize)), (char *)  tmpPtr, tlist->perSize);
        free(tmpPtr);
    }
    else
    {
        n = cn_bitcp((*tlist->b + (tlist->cnt * tlist->perSize)), item, tlist->perSize);
    }
    if(n < 0)
    {
        goto unlock_reterr;
    }
    tlist->cnt++;
    pthread_mutex_unlock(&tlist->key);
    return 0;
    unlock_reterr: ;
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

void *cn_listGet(cn_list *tlist, uint32_t indx)
{
    if(tlist == NULL)
    {
        return NULL;
    }
    if(tlist->cnt < indx) {return NULL;}
    return (*tlist->b + (indx * tlist->perSize));
}

int cn_listSet(cn_list *tlist, uint32_t indx, void * item)
{
    if(tlist == NULL || item == NULL)
    {
        return -1;
    }
    if(tlist->cnt < indx) {return -1;}
    pthread_mutex_lock(&tlist->key);
    int n = cn_bitcp((*tlist->b + (indx * tlist->perSize)), item, tlist->perSize);
    pthread_mutex_unlock(&tlist->key);
    if(n < 0)
    {
        return -1;
    }
    return 0;
}

int cn_listIndexOf(cn_list *tlist, void *item, uint16_t CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
{
    // return false if parameters NULL
    if(tlist == NULL || item == NULL){return -1;}
    // Start searching through list
    pthread_mutex_lock(&tlist->key);
    void **items = (void **)*tlist->b;
    int i;
    if(tlist->appendTheAddress)
    {
        for(i = 0; i < tlist->cnt; i++)
        {
            #ifdef CN_DEBUG_MODE
            cn_log("cn_listIndexOf %d = %d item %d count %d\n", i, (int)*(items + i), (int)item, tlist->cnt);
            #endif // CN_DEBUG_MODE
            if(items[i] == item)
            {
                #ifdef CN_DEBUG_MODE
                cn_log("cn_listIndexOf finished at index %d\n", i);
                #endif // CN_DEBUG_MODE
                pthread_mutex_unlock(&tlist->key);
                return i;
            }
        }
    }
    else
    {
        int limit = tlist->blen - (tlist->perSize * cnt);
        void *listPtr = *tlist->b;
        for(i = 0; i < limit; i++)
        {
            if(cn_bitcompare((listPtr + i), item, (tlist->perSize * cnt)))
            {
                pthread_mutex_unlock(&tlist->key);
                return i;
            }
        }
    }
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

bool cn_listContains(cn_list *tlist, void *item, uint16_t CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
{
    int result = cn_listIndexOf(tlist, item, cnt);
    if(result >= 0){return true;}
    else{return false;}
}

int cn_listSplice(cn_list *tlist, uint32_t startIndx, uint16_t len, cn_syncFuncPTR itemDestructor)
{
    uint32_t i = 0;
    int n = 0;
    i += startIndx;
    i += len;
    pthread_mutex_lock(&tlist->key);

    //AUTO FREE FEATURE
    void **ptrsToFree;
    if(tlist->appendTheAddress)
    {
        if(itemDestructor != NULL)
        {
            ptrsToFree = malloc(sizeof(void *) * len);
            int freeIndex;
            for(freeIndex = 0; freeIndex < len; freeIndex++)
            {
                ptrsToFree[freeIndex] = cn_listGet(tlist, (startIndx + freeIndex));
            }
        }
    }

    n = cn_bitcp((*tlist->b + (startIndx * tlist->perSize)), (*tlist->b + (i * tlist->perSize)), (tlist->cnt - i) * tlist->perSize);
    if(n < 0)
    {
        goto unlock_reterr;
    }

    //AUTO FREE FEATURE
    if(tlist->appendTheAddress == true)
    {
        if(itemDestructor != NULL)
        {
            int freeIndex;
            for(freeIndex = 0; freeIndex < len; freeIndex++)
            {
                itemDestructor(ptrsToFree[freeIndex]);
            }
            free(ptrsToFree);
        }
    }

    tlist->cnt -= len;
    pthread_mutex_unlock(&tlist->key);
    return 0;
    unlock_reterr: ;
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

int cn_listRemoveAt(cn_list *tlist, uint32_t indx)
{
    return cn_listSplice(tlist, indx, 1, NULL);
}

int cn_listExpand(cn_list *tlist, uint32_t cnt)
{
    if(tlist == NULL) {return -1;}
    if(cnt == 0) {return 0;}
    uint32_t newblen = tlist->blen + (cnt * tlist->perSize);
    char *newb = malloc(newblen);
    if(newb == NULL) {return -1;}
    int n = 0;
    pthread_mutex_lock(&tlist->key);
    n = cn_bitcp(newb, *tlist->b, newblen);
    if(n < 0) {free(newb); goto unlock_reterr;}
    tlist->maxCnt += cnt;
    tlist->blen = newblen;
    free(*tlist->b);
    *tlist->b = newb;
    pthread_mutex_unlock(&tlist->key);
    return 0;
    unlock_reterr: ;
    pthread_mutex_unlock(&tlist->key);
    return -1;
}


int cn_listEmpty(cn_list *tlist, cn_syncFuncPTR itemDestructor)
{
    return cn_listSplice(tlist, 0, tlist->cnt, itemDestructor);
}

int cn_listInsertAt(cn_list *tlist, uint32_t indx, void *items, uint16_t cnt)
{
    if(tlist == NULL || items == NULL) {return -1;}
    if(cnt == 0){return 0;}
    int needExpand = 0;
    uint32_t tmpMaxCnt;
    uint32_t tmpblen;
    char *tmpb;
    if((float)(tlist->cnt + cnt) >= (float)tlist->maxCnt - ((float)tlist->maxCnt * 0.3f))
    {
        needExpand = 1;
        if((float)(tlist->maxCnt*2) <= ((float)tlist->cnt + (float)cnt - ((float)tlist->cnt * 0.3f)))
        {
            tmpMaxCnt = (tlist->cnt + cnt) * 2;
        }
        else
        {
            tmpMaxCnt = (tlist->maxCnt * 2);
        }
        tmpblen = (tmpMaxCnt * tlist->perSize);
        tmpb = malloc(tmpblen);
        if(tmpb == NULL){return -1;}
    }
    else
    {
        tmpb = *tlist->b;
    }
    int n = 0;
    pthread_mutex_lock(&tlist->key);
    n = cn_bitcp(tmpb, *tlist->b, (indx * tlist->perSize));
    if(n < 0) {goto unlock_reterr;}
    if(indx != (tlist->cnt - 1) && tlist->cnt > 0)
    {
        uint32_t tmpblen2 = ((tlist->cnt - indx) * tlist->perSize);
        char tmpb2[tmpblen2];
        if(tmpb2 == NULL) {goto unlock_reterr;}
        n = cn_bitcp(tmpb2, (*tlist->b + (indx * tlist->perSize)), tmpblen2);
        if(n < 0) {goto unlock_reterr;}
        n = cn_bitcp((tmpb + ((indx + cnt) * tlist->perSize)), tmpb2, tmpblen2);
        if(n < 0) {goto unlock_reterr;}
    }
    n = cn_bitcp((tmpb + (indx * tlist->perSize)), items, (cnt * tlist->perSize));
    if(n < 0) {goto unlock_reterr;}
    if(needExpand == 1)
    {
        free(*tlist->b);
        tlist->maxCnt = tmpMaxCnt;
        tlist->blen = tmpblen;
    }
    *tlist->b = tmpb;
    tlist->cnt += cnt;
    pthread_mutex_unlock(&tlist->key);
    return 0;
    unlock_reterr: ;
    pthread_mutex_unlock(&tlist->key);
    return -1;
}
