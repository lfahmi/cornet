#include "cornet/cnnum.h"

/**
    * Construct list object
    * @param perSize : sizeof item type that will be stored.
    * @param firstMaxCnt : first size of array to store items.
    * @param appendTheAddress : set true if you want to store pointer.
    */
cn_list *cn_makeList(uint16_t perSize, uint32_t firstMaxCnt, bool appendTheAddress)
{
    // Defensive code.
    if(perSize == 0 || firstMaxCnt == 0) {return NULL;}

    // Allocate list structure, defensively.
    cn_list *tlist = malloc(sizeof(cn_list));
    if(tlist == NULL) {return NULL;}

    // set structure default value.
    tlist->perSize = perSize;
    tlist->blen = firstMaxCnt * perSize;
    tlist->maxCnt = firstMaxCnt;
    tlist->appendTheAddress = appendTheAddress;
    tlist->cnt = 0;

    // Allocate list buffer pointer holder, defensively.
    tlist->b = malloc(sizeof(*tlist->b));
    if(tlist->b == NULL)
    {
        free(tlist);
        return NULL;
    }

    // Allocate list buffer, defensively.
    *tlist->b = malloc(tlist->blen);
    if(*tlist->b == NULL)
    {
        free(tlist->b);
        free(tlist);
        return NULL;
    }

    // Initialize list key, defensively.
    if(pthread_mutex_init(&tlist->key, NULL) != 0)
    {
        free(*tlist->b);
        free(tlist->b);
        free(tlist);
        return NULL;
    }

    // execution made it to finish fine.
    return tlist;
}

/**
    * Destruct list object
    * @param tlist : target
    */
int cn_desList(cn_list *tlist)
{
    // Defensive code.
    if(tlist == NULL) { return -1; }

    // lock the list
    pthread_mutex_lock(&tlist->key);

    // dealloc list buffer
    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNNUM_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d] dealloc attempt next.", __FILE__, __func__, __LINE__);
    #endif // CN_DEBUG_MODE
    free(*tlist->b);

    // dealloc list buffer pointer holder
    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNNUM_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d] dealloc attempt next.", __FILE__, __func__, __LINE__);
    #endif // CN_DEBUG_MODE
    free(tlist->b);

    // unlock list
    pthread_mutex_unlock(&tlist->key);

    // destroy lock key
    pthread_mutex_destroy(&tlist->key);

    // finnaly dealloc list structure
    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNNUM_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d] dealloc attempt next.", __FILE__, __func__, __LINE__);
    #endif // CN_DEBUG_MODE
    free(tlist);

    // finish fine
    return 0;
}

/**
    * Append an item to the list
    * @param tlist : target list.
    * @param item : the item to add.
    * return 0 for fine otherwise -1
    */
int cn_listAppend(cn_list *tlist, void *item)
{
    // Defensive code.
    if(tlist == NULL || item == NULL){return -1;}

    // declare execution status
    int n = 0;
    if(tlist->cnt >= tlist->maxCnt)
    {
        /*  seems like number of item inside the list buffer
            is at maximum. let's expand the list buffer. */
        n = cn_listExpand(tlist, tlist->cnt);
        if(n != 0) {return n;}
    }

    // now lock the list
    pthread_mutex_lock(&tlist->key);
    if(tlist->appendTheAddress == 1)
    {
        // seems like this is a list of pointer we must hold the address
        // on buffer to be copied to the list.
        void **tmpPtr = malloc(sizeof(void *));
        if(tmpPtr == NULL){goto unlock_reterr;}

        // now set buffer to be copied by item address
        *tmpPtr = item;

        // finnaly copy it to the list buffer. defensively.
        n = cn_bitcp((*tlist->b + (tlist->cnt * tlist->perSize)), (char *)  tmpPtr, tlist->perSize);

        // dealloc address holder
        free(tmpPtr);
    }
    else
    {
        // seems like this is not a list of pointer. just copy the content on item address. defensively.
        n = cn_bitcp((*tlist->b + (tlist->cnt * tlist->perSize)), item, tlist->perSize);
    }
    // defensive code. check last copy operation.
    if(n < 0){goto unlock_reterr;}

    // finnaly we can increase list count and unlock the list
    tlist->cnt++;
    pthread_mutex_unlock(&tlist->key);

    // finish fine.
    return 0;

    // error handling
    unlock_reterr:

    // unlcok list then return failure
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

/**
    * Get an item from list.
    * @param tlist : target list.
    * @param indx : item to be retreived index from the list.
    * return NULL if fail.
    */
void *cn_listGet(cn_list *tlist, uint32_t indx)
{
    // Defensive code.
    if(tlist == NULL){return NULL;}
    if(tlist->cnt < indx) {return NULL;}

    // return the item address
    return (*tlist->b + (indx * tlist->perSize));
}

/**
    * Set an item value from list by parameter item.
    * @param tlist : target list.
    * @param indx : target item index.
    * @param item : new item value for item at index.
    * return 0 for fine.
    */
int cn_listSet(cn_list *tlist, uint32_t indx, void * item)
{
    // Defensive code.
    if(tlist == NULL || item == NULL){return -1;}
    if(tlist->cnt < indx) {return -1;}

    // Declare status code
    int n = 0;

    // lock the list
    pthread_mutex_lock(&tlist->key);
    if(tlist->appendTheAddress)
    {
        // seems like this is a pointer list. lets copy its address to list
        // create address holder for bitcp, pointer of pointer
        void **tmpPtr = malloc(sizeof(void *));
        if(tmpPtr == NULL){goto reterr;}
        *tmpPtr = item;
        n = cn_bitcp((*tlist->b + (indx * tlist->perSize)), (char *)tmpPtr, tlist->perSize);

        // dealloc address holder
        free(tmpPtr);
    }
    else
    {
        // copy item to list buffer
        n = cn_bitcp((*tlist->b + (indx * tlist->perSize)), item, tlist->perSize);
    }
    // unlock the list
    pthread_mutex_unlock(&tlist->key);

    // return status from bitcp
    return n;

    reterr:

    // unlock list then return failure
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

/**
    * Get the index of item that equal to parameter item
    * @param tlist : target list.
    * @param item : item to look for.
    * @param cnt : count of ordered item, only used for non append address list.
    * return -1 for failure.
    */
int cn_listIndexOf(cn_list *tlist, void *item, uint16_t CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
{
    // Defemsive code.
    if(tlist == NULL || item == NULL){return -1;}

    // lock the list
    pthread_mutex_lock(&tlist->key);
    int i;
    if(tlist->appendTheAddress)
    {
        // seems like this is pointer list
        // retrieve list buffer as list of pointer
        void **items = (void **)*tlist->b;

        // now look for the matching address in the list
        for(i = 0; i < tlist->cnt; i++)
        {
            if(items[i] == item)
            {
                // we got the index. unlock list and return index.
                pthread_mutex_unlock(&tlist->key);
                return i;
            }
        }
    }
    else
    {
        // retrieve the loop limit and list buffer
        int limit = tlist->blen - (tlist->perSize * cnt);
        void *listPtr = *tlist->b;

        // start looking for the item, increment by sizeof item
        for(i = 0; i < limit; i+=tlist->perSize)
        {
            // compare items in list with parameter item
            if(cn_bitcompare((listPtr + i), item, (tlist->perSize * cnt)) == true)
            {
                // we got the mathcing items index from the list.
                // unlock the list and return the index.
                pthread_mutex_unlock(&tlist->key);
                return i;
            }
        }
    }
    // seems like we couldn't find matching item in list
    // unlock list and return failure.
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

/**
    * Determine wether a list contain an item.
    * @param tlist : target list.
    * @param item : item to look for.
    * @param cnt : count of ordered item, only used for non append address list.
    */
bool cn_listContains(cn_list *tlist, void *item, uint16_t CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
{
    // if item is found at any valid index, return true.
    if(cn_listIndexOf(tlist, item, cnt) >= 0){return true;}
    else{return false;}
}

/**
    * Remove items from list at startIndex and merge items after and before it.
    * @param tlist : target list.
    * @param startIndx : start index of removal.
    * @param remlen : how much item to be removed.
    * @param itemDestructor : destructor for the item, set NULL for dont destruct.
    */
int cn_listSplice(cn_list *tlist, uint32_t startIndx, uint16_t remlen, cn_syncFuncPTR itemDestructor)
{
    // declare work index
    uint32_t i = 0;

    // declare status code
    int n = 0;

    // add index by removal start index
    i += startIndx;

    // add index by length of items to be removed
    i += remlen;

    // lock the list
    pthread_mutex_lock(&tlist->key);

    //AUTO FREE FEATURE
    void **itemToDestruct;
    if(itemDestructor != NULL)
    {
        if(tlist->appendTheAddress)
        {
            // because we need a pointer array.
            // rather than using single pointer of pointer
            // alocate enough pointer array.
            itemToDestruct = malloc(sizeof(void *) * remlen);
            int loopi;
            for(loopi = 0; loopi < remlen; loopi++)
            {
                // catch the item addresses
                itemToDestruct[loopi] = cn_listGet(tlist, (startIndx + loopi));
            }
        }
    }

    if(i >= tlist->cnt)
    {
        // seems like there is items at index higher than what we will remove.
        // we need to move them to the right index
        n = cn_bitcp((*tlist->b + (startIndx * tlist->perSize)), (*tlist->b + (i * tlist->perSize)), (tlist->cnt - i) * tlist->perSize);
        if(n < 0)
        {
            goto unlock_reterr;
        }
    }
    //AUTO FREE FEATURE
    if(itemDestructor != NULL)
    {
        if(tlist->appendTheAddress == true)
        {
            int loopi;
            for(loopi = 0; loopi < remlen; loopi++)
            {
                // finnaly destruct each removed items
                itemDestructor(itemToDestruct[loopi]);
            }
        }
    }

    // decrease the list count by removed items count
    tlist->cnt -= remlen;

    // unlock list
    pthread_mutex_unlock(&tlist->key);

    // return fine
    return 0;

    // error handler
    unlock_reterr:

    // unlock list then return failure
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

/**
    * Remove item from list at index.
    * @param tlist : target list.
    * @param indx : index of item to be removed.
    */
int cn_listRemoveAt(cn_list *tlist, uint32_t indx)
{
    // real work in list splice
    return cn_listSplice(tlist, indx, 1, NULL);
}

/**
    * Expand the list buffer capacity (maxCnt property)
    * @param tlist : target list.
    * @param cnt : how much capacity will expand.
    */
int cn_listExpand(cn_list *tlist, uint32_t cnt)
{
    // Defensive code.
    if(tlist == NULL) {return -1;}
    if(cnt == 0) {return 0;}

    // declare new capacity of list buffer.
    uint32_t newblen = tlist->blen + (cnt * tlist->perSize);

    // Allocate new list buffer defensively.
    char *newb = malloc(newblen);
    if(newb == NULL) {return -1;}

    // Declare status code.
    int n = 0;

    // lock the list
    pthread_mutex_lock(&tlist->key);

    // copy list buffer items to newly expanded buffer, defensively.
    n = cn_bitcp(newb, *tlist->b, newblen);
    if(n < 0) {free(newb); goto unlock_reterr;}

    // finally update the list buffer capacity and buffer lenght
    tlist->maxCnt += cnt;
    tlist->blen = newblen;

    // deallocate the old list buffer
    free(*tlist->b);

    // finnaly set newly expanded buffer as list buffer.
    *tlist->b = newb;

    // unlock the list
    pthread_mutex_unlock(&tlist->key);

    // return fine
    return 0;

    // error handler
    unlock_reterr:

    // unlock the list then return failure.
    pthread_mutex_unlock(&tlist->key);
    return -1;
}

/**
    * Remove all items in list.
    * @param tlist : target list.
    * @param itemDestructor : destructor for the item, set NULL for dont destruct.
    */
int cn_listEmpty(cn_list *tlist, cn_syncFuncPTR itemDestructor)
{
    // real work in list splice
    return cn_listSplice(tlist, 0, tlist->cnt, itemDestructor);
}

/**
    * Insert items to the list at index.
    * @param tlist : target list.
    * @param indx : position where to insert the items.
    * @param items : item objects array.
    * @param cnt : how much item is goint to be added.
    */
int cn_listInsertAt(cn_list *tlist, uint32_t indx, void *items, uint16_t cnt)
{
    // Defensive code.
    if(tlist == NULL || items == NULL) {return -1;}
    if(cnt == 0){return 0;}

    // declare indication for expansion is required
    bool needExpand = false;

    // temporary list capacity, list buffer lenght, list buffer
    uint32_t tmpMaxCnt;
    uint32_t tmpblen;
    char *tmpb;
    if((float)(tlist->cnt + cnt) >= (float)tlist->maxCnt - ((float)tlist->maxCnt * 0.3f))
    {
        // seems like if we add the items to the list, there is less than 30% list capacity left.
        // we need to expand the list capacity first.
        needExpand = true;
        if((float)(tlist->maxCnt*2) <= ((float)tlist->cnt + (float)cnt - ((float)tlist->cnt * 0.3f)))
        {
            // seems like the item that will be added is larger than the items in list.
            // expand the list the double of addition of both of them. set tno list capacity.
            tmpMaxCnt = (tlist->cnt + cnt) * 2;
        }
        else
        {
            // items is smaller than the items in list. expand the list to double. set tno list capacity.
            tmpMaxCnt = (tlist->maxCnt * 2);
        }
        // set tmp list buffer length by multiply per item size with tmp list capacity
        tmpblen = (tmpMaxCnt * tlist->perSize);

        // allocate tmp list buffer by tmp list buffer lenght, defensinvely.
        tmpb = malloc(tmpblen);
        if(tmpb == NULL){return -1;}
    }
    else
    {
        // seems like there's no need for expansion, set tmp list buffer by list buffer
        tmpb = *tlist->b;
    }
    // declare status code
    int n = 0;

    // go inside the list and lock it.
    pthread_mutex_lock(&tlist->key);
    if(needExpand == true && indx > 0)
    {
        // seems like tmp list buffer is new buffer, copy items before insert index, defensively.
        n = cn_bitcp(tmpb, *tlist->b, (indx * tlist->perSize));
        if(n < 0) {goto unlock_reterr;}

    }
    if(indx != (tlist->cnt - 1) && tlist->cnt > 0)
    {
        /*  so we are inserting items in the middle of the list
            ( not after last index and not the first item for the list )
            we need to move the items at insert index to
            the new place so we can insert our items. */
        // take the bufferlength of the items at insert index to the list end.
        uint32_t tmpblen2 = ((tlist->cnt - indx) * tlist->perSize);

        // tmp copy buffer the lenght of items to move
        char tmpb2[tmpblen2];

        // copy items to move to tmp copy buffer, defensively.
        n = cn_bitcp(tmpb2, (*tlist->b + (indx * tlist->perSize)), tmpblen2);
        if(n < 0) {goto unlock_reterr;}

        // copy items to move to the right position on list buffer from copy buffer, defensively.
        n = cn_bitcp((tmpb + ((indx + cnt) * tlist->perSize)), tmpb2, tmpblen2);
        if(n < 0) {goto unlock_reterr;}
    }
    // finally after all the preparation, we insert our items to the list, defensively.
    n = cn_bitcp((tmpb + (indx * tlist->perSize)), items, (cnt * tlist->perSize));
    if(n < 0) {goto unlock_reterr;}
    if(needExpand == true)
    {
        /*  because the list capacity was expanded here in the function
            we need to deallocate the old list buffer and update the list
            capacity and buffer length. */
        free(*tlist->b);
        tlist->maxCnt = tmpMaxCnt;
        tlist->blen = tmpblen;
    }
    // set list buffer by tmp list buffer (buffer we work on)
    *tlist->b = tmpb;

    // add the list items count by the items count we add.
    tlist->cnt += cnt;

    // get out from the list left it unlocked
    pthread_mutex_unlock(&tlist->key);

    // return fine.
    return 0;

    // error handler, unlock the list, return failure.
    unlock_reterr:
    pthread_mutex_unlock(&tlist->key);
    return -1;
}
