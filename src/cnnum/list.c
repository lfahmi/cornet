#include "cornet/cnnum.h"
#if CN_DEBUG_MODE_CNNUM_H_LVL > 0
#define CN_DEBUG_TYPENAME "cn_list"
#endif // CN_DEBUG_MODE_CNTYPE_H_LVL

/// List type id
cn_type_id_t cn_list_type_id = 0;

/**
    * Construct list object
    * @param refname : reference name. (variable name)
    * @param perSize : sizeof item type that will be stored.
    * @param firstMaxCnt : first size of array to store items.
    * @param appendTheAddress : set true if you want to store pointer.
    */
cn_list *cn_makeList(const char *refname, int perSize, int firstMaxCnt, bool appendTheAddress)
{
    // Defensive code.
    if(perSize == 0 || firstMaxCnt == 0) {return NULL;}

    // Allocate list structure, defensively.
    cn_list *tlist = malloc(sizeof(cn_list));
    if(tlist == NULL) {return NULL;}

    // Set reference name
    tlist->refname = refname;

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
    if(cn_mutex_init(&tlist->key, NULL) != 0)
    {
        free(*tlist->b);
        free(tlist->b);
        free(tlist);
        return NULL;
    }

    // if list type has no identifier, request identifier.
    if(cn_list_type_id < 1){cn_list_type_id = cn_typeGetNewID();}

    // initialize object type definition
    cn_typeInit(&tlist->t, cn_list_type_id);

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
    cn_mutex_lock(&tlist->key);

    // Destroy object type definition
    cn_typeDestroy(&tlist->t);

    // dealloc list buffer
    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNNUM_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d][%s:%s] dealloc attempt next.\n", __FILE__, __func__, __LINE__, tlist->refname, CN_DEBUG_TYPENAME);
    #endif // CN_DEBUG_MODE
    free(*tlist->b);

    // dealloc list buffer pointer holder
    free(tlist->b);

    // unlock list
    cn_mutex_unlock(&tlist->key);

    // destroy lock key
    cn_mutex_destroy(&tlist->key);

    // finnaly dealloc list structure
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
    cn_mutex_lock(&tlist->key);
    if(tlist->appendTheAddress == 1)
    {
        // seems like this is a list of pointer we must hold the address
        void **items = (void **)*tlist->b;
        items[tlist->cnt] = item;
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
    cn_mutex_unlock(&tlist->key);

    // finish fine.
    return 0;

    // error handling
    unlock_reterr:

    // unlcok list then return failure
    cn_mutex_unlock(&tlist->key);
    return -1;
}

/**
    * Get an item from list.
    * @param tlist : target list.
    * @param indx : item to be retreived index from the list.
    * return NULL if fail.
    */
void *cn_listGet(cn_list *tlist, int indx)
{
    // Defensive code.
    if(tlist == NULL || indx < 0){return NULL;}
    if(tlist->cnt < indx) {return NULL;}

    if(tlist->appendTheAddress)
    {
        // return the item pointer
        return *((void **)(*tlist->b + (indx * tlist->perSize)));
    }

    // return the item address in list buffer
    return (*tlist->b + (indx * tlist->perSize));
}

/**
    * Set an item value from list by parameter item.
    * @param tlist : target list.
    * @param indx : target item index.
    * @param item : new item value for item at index.
    * return 0 for fine.
    */
int cn_listSet(cn_list *tlist, int indx, void * item)
{
    // Defensive code.
    if(tlist == NULL || item == NULL || indx < 0){return -1;}
    if(tlist->cnt < indx) {return -1;}

    // Declare status code
    int n = 0;

    // lock the list
    cn_mutex_lock(&tlist->key);
    if(tlist->appendTheAddress)
    {
        void **items = *tlist->b;
        items[indx] = item;
    }
    else
    {
        // copy item to list buffer
        n = cn_bitcp((*tlist->b + (indx * tlist->perSize)), item, tlist->perSize);
    }
    // unlock the list
    cn_mutex_unlock(&tlist->key);

    // return status from bitcp
    return n;
}

/**
    * Get the index of item that equal to parameter item
    * @param tlist : target list.
    * @param item : item to look for.
    * @param cnt : count of ordered item, only used for non append address list.
    * return -1 for failure.
    */
static int listIndexOf(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
{
    if(tlist->cnt < 1){return -1;}
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
                // we got the index. return index.
                return i;
            }
        }
    }
    else
    {
        // retrieve the loop limit and list buffer
        // limit is the length of all items in list buffer subtracter by comparison item count (cnt) subtracted by one
        // preventing for accessing the end of list buffer if item to look for is greater than one
        int limit = (tlist->perSize * tlist->cnt) - ((cnt - 1) * tlist->perSize);
        void *listPtr = *tlist->b;

        // start looking for the item, increment by sizeof item
        for(i = 0; i < limit; i+=tlist->perSize)
        {
            // compare items in list with parameter item
            if(cn_bitcompare((listPtr + i), item, (tlist->perSize * cnt)) == true)
            {
                // we got the mathcing items index from the list.
                // return the index.
                return i / tlist->perSize;
            }
        }
    }

    // seems like we couldn't find matching item in list return failure.
    return -1;
}

/** Interface for listIndexOf */
int cn_listIndexOf(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
{
    // Defensive code
    if(tlist == NULL || item == NULL || cnt < 0){return -1;}

    // lock the list
    cn_mutex_lock(&tlist->key);

    // Real function.
    int n =listIndexOf(tlist, item, cnt);

    // unlock the list
    cn_mutex_unlock(&tlist->key);

    return n;
}

/**
    * Determine wether a list contain an item.
    * @param tlist : target list.
    * @param item : item to look for.
    * @param cnt : count of ordered item, only used for non append address list.
    */
bool cn_listContains(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt)
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
static int listSplice(cn_list *tlist, int startIndx, int remlen, cn_syncFuncPTR itemDestructor)
{
    // declare status code
    int n = 0;

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
            void **items = (void **)*tlist->b;
            int loopi;
            for(loopi = 0; loopi < remlen; loopi++)
            {
                // catch the item addresses
                itemToDestruct[loopi] = items[startIndx + loopi];
            }
        }
    }

    // declare index after removed items
    int nextItemsIndex = startIndx + remlen;

    if(nextItemsIndex < (tlist->cnt) )
    {
        // seems like there is items at index higher than what we will remove.
        // we need to move them to the right index
        n = cn_bitcp((*tlist->b + (startIndx * tlist->perSize)), (*tlist->b + (nextItemsIndex * tlist->perSize)), (tlist->cnt - nextItemsIndex) * tlist->perSize);
        if(n < 0){goto unlock_reterr;}
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
            free(itemToDestruct);
        }
    }

    // decrease the list count by removed items count
    tlist->cnt -= remlen;

    // return fine
    return 0;

    // error handler
    unlock_reterr:

    // return failure
    return -1;
}

/** Interface for listSplice */
int cn_listSplice(cn_list *tlist, int startIndx, int remlen, cn_syncFuncPTR itemDestructor)
{
    if(tlist == NULL || startIndx < 0 || remlen < 0){return -1;}

    // lock the list
    cn_mutex_lock(&tlist->key);

    int n = listSplice(tlist, startIndx, remlen, itemDestructor);

    // unlock list
    cn_mutex_unlock(&tlist->key);

    return n;
}

/**
    * Remove item from list at index.
    * @param tlist : target list.
    * @param indx : index of item to be removed.
    */
int cn_listRemoveAt(cn_list *tlist, int indx)
{
    // real work in list splice
    return cn_listSplice(tlist, indx, 1, NULL);
}

/**
    * Remove item from list at index, without mutex locking.
    * @param tlist : target list.
    * @param indx : index of item to be removed.
    */
int cn_listRemoveAtULOCK(cn_list *tlist, int indx)
{
    if(tlist == NULL || indx < 0){return -1;}
    return listSplice(tlist, indx, 1, NULL);
}

/**
    * Look for Items matching in the list and remove it from the list.
    * @param tlist : Target list.
    * @param item : Items to be removed.
    * @param cnt : How much items to be removed.
    * return 0 for fine result.
    */
int cn_listRemove(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt, cn_syncFuncPTR itemDestructor)
{
    // Defensive code
    if(tlist == NULL || item == NULL){return -1;}
    if(tlist->appendTheAddress == true){cnt = 1;}
    else{if(cnt < 0){return -1;}}

    // Lock the list
    cn_mutex_lock(&tlist->key);

    // Find index of items
    int indx = listIndexOf(tlist, item, cnt);

    // Remove the items
    int n = listSplice(tlist, indx, cnt, itemDestructor);

    // Unlock the list
    cn_mutex_unlock(&tlist->key);

    // Return status
    return n;
}

/**
    * Expand the list buffer capacity (maxCnt property)
    * @param tlist : target list.
    * @param cnt : how much capacity will expand.
    */
static int listExpand(cn_list *tlist, int cnt)
{
    // Defensive code.
    if(tlist == NULL || cnt < 0) {return -1;}
    if(cnt == 0) {return 0;}

    // declare new capacity of list buffer.
    int newblen = tlist->blen + (cnt * tlist->perSize);

    // Allocate new list buffer defensively.
    char *newb = malloc(newblen);
    if(newb == NULL) {return -1;}

    // Declare status code.
    int n = 0;

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

    // return fine
    return 0;

    // error handler
    unlock_reterr:

    // return failure.
    return -1;
}

/** Interface for listExpand */
int cn_listExpand(cn_list *tlist, int cnt)
{
    // Defensive code
    if(tlist == NULL || cnt < 0){return -1;}

    // Lock the list
    cn_mutex_lock(&tlist->key);

    // Real function
    int n = listExpand(tlist, cnt);

    // Unlock the list
    cn_mutex_unlock(&tlist->key);

    // return status
    return n;
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
static int listInsertAt(cn_list *tlist, int indx, void *items, int cnt)
{
    // If pointer list. item count to append always 1
    if(tlist->appendTheAddress){cnt = 1;}

    // declare indication for expansion is required
    bool needExpand = false;

    // temporary list capacity, list buffer lenght, list buffer
    int tmpMaxCnt;
    int tmpblen;
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
    if(needExpand == true && indx > 0)
    {
        // seems like tmp list buffer is new buffer, copy items before insert index, defensively.
        n = cn_bitcp(tmpb, *tlist->b, (indx * tlist->perSize));
        if(n < 0) {goto unlock_reterr;}

    }
    if(indx < tlist->cnt && tlist->cnt > 0)
    {
        /*  so we are inserting items in the middle of the list
            ( not after last index and not the first item for the list )
            we need to move the items at insert index to
            the new place so we can insert our items. */
        // take the bufferlength of the items at insert index to the list end.
        int tmpblen2 = ((tlist->cnt - indx) * tlist->perSize);

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
    if(tlist->appendTheAddress)
    {
        void **litems = *tlist->b;
        litems[indx] = items;
    }
    else
    {
        n = cn_bitcp((tmpb + (indx * tlist->perSize)), items, (cnt * tlist->perSize));
    }

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

    // return fine.
    return 0;

    // error handler, return failure.
    unlock_reterr:
    return -1;
}

/** Interface for listInsertAt */
int cn_listInsertAt(cn_list *tlist, int indx, void *items, int cnt)
{
    // Defensive code.
    if(tlist == NULL || items == NULL || indx < 0 || indx > tlist->cnt || cnt < 1) {return -1;}

    // Lock the list
    cn_mutex_lock(&tlist->key);

    // Real function
    int n = listInsertAt(tlist, indx, items, cnt);

    // Unlock the list
    cn_mutex_unlock(&tlist->key);

    // Return status
    return n;
}
