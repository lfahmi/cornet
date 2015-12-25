#ifndef _CNNUM_H
#define _CNNUM_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <pthread.h>

#define CN_NEVER_USE_THIS_WITH_LIST_APPENDADDRESS
#define CN_NOT_USED_FOR_LIST_APPENDADDRESS


#include "cornet/cntype.h"

/* LIST */

/// type id for list
extern cn_type_id_t cn_list_type_id;

/** Definition for List structure */
struct cn_list
{
    cn_type t;
    const char *refname;
    void **b;
    int blen;
    int perSize;
    int cnt;
    int maxCnt;
    bool appendTheAddress;
    pthread_mutex_t key;
};

typedef struct cn_list cn_list;

/* QUEUE */

/// type id for queue
extern cn_type_id_t cn_queue_type_id;

struct cn_queueNode
{
    void *item;
    struct cn_queueNode *next;
};

/**  Definition for Queue structure */
struct cn_queue
{
    cn_type t;
    const char *refname;
    struct cn_queueNode *frontNode;
    struct cn_queueNode *rearNode;
    int perSize;
    int cnt;
    pthread_mutex_t key;
    bool appendTheAddress;
};

typedef struct cn_queue cn_queue;

/* DICTIONARY 4bytes key */

/// type id for dictionary 4 bytes
extern cn_type_id_t cn_dictionary4b_type_id;

/** Definition for Dictionary 4bytes-key structure
    this dictionary allow no collision, ipv4 is suitable key
*/
struct cn_dictionary4b
{
    cn_type t;
    const char *refname;
    cn_list *itemKey;
    cn_list *itemValue;
    int cnt;
    pthread_mutex_t key;
};

typedef struct cn_dictionary4b cn_dictionary4b;

/* MACROS DEFINITION */

/**
    * usage CN_LIST_FOREACH(item pointer, cn_list){statement body}
    */

#define CN_LIST_FOREACH(item, list) \
    for((item) = *(list)->b; \
    (void *)(item) < (*(list)->b + ((list)->cnt * (list)->perSize)); \
    (item) += (list)->perSize)
/*
#define CN_QUEUE_FOREACH(item, queue) \
    for((item) = (queue)->frontNode; (item) != NULL; (item) = (item)->next) */

/**
    * usage CN_QUEUE_FOREACH(item pointer, cn_queue, {nested statement; nested statement; ...});
    */

#define CN_QUEUE_FOREACH(item, queue, body) \
    {struct cn_queueNode *_cn_macro_qnode = (queue)->frontNode; \
    while(_cn_macro_qnode != NULL) \
    { \
        (item) = _cn_macro_qnode->item; \
        body \
        _cn_macro_qnode = _cn_macro_qnode->next; \
    }}

/* END OF DEFINITION */

#include "cornet/cndebug.h"
#include "cornet/cnbitop.h"

#define CN_DEBUG_MODE 1

#undef CN_DEBUG_MODE_FREE
#define CN_DEBUG_MODE_FREE 1
#undef CN_DEBUG_MODE_CNNUM_H_LVL
//#define CN_DEBUG_MODE_CNNUM_H_LVL 10

extern int cn_cnnumDefaultItemDestructor(void *item);

/* LIST */

/**
    * Construct list object
    * @param refname : reference name. (variable name)
    * @param perSize : sizeof item type that will be stored.
    * @param firstMaxCnt : first size of array to store items.
    * @param appendTheAddress : set true if you want to store pointer.
    */
extern cn_list *cn_makeList(const char *refname, int perSize, int firstMaxCnt, bool appendTheAddress);

/**
    * Destruct list object
    * @param tlist : target
    */
extern int cn_desList(cn_list *tlist);

/**
    * Append an item to the list
    * @param tlist : target list.
    * @param item : the item to add.
    * return 0 for fine otherwise -1
    */
extern int cn_listAppend(cn_list *tlist, void *item);

/**
    * Get an item from list.
    * @param tlist : target list.
    * @param indx : item to be retreived index from the list.
    * return NULL if fail.
    */
extern void *cn_listGet(cn_list *tlist, int indx);

/**
    * Set an item value from list by parameter item.
    * @param tlist : target list.
    * @param indx : target item index.
    * @param item : new item value for item at index.
    * return 0 for fine.
    */
extern int cn_listSet(cn_list *tlist, int indx, void *item);

/**
    * Get the index of item that equal to parameter item
    * @param tlist : target list.
    * @param item : item to look for.
    * @param cnt : count of ordered item, only used for non append address list.
    * return -1 for failure.
    */
extern int cn_listIndexOf(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt);

/**
    * Determine wether a list contain an item.
    * @param tlist : target list.
    * @param item : item to look for.
    * @param cnt : count of ordered item, only used for non append address list.
    */
extern bool cn_listContains(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt);

/**
    * Remove items from list at startIndex and merge items after and before it.
    * @param tlist : target list.
    * @param startIndx : start index of removal.
    * @param remlen : how much item to be removed.
    * @param itemDestructor : destructor for the item, set NULL for dont destruct.
    */
extern int cn_listSplice(cn_list *tlist, int startIndx, int len, cn_syncFuncPTR itemDestructor);

/**
    * Remove item from list at index.
    * @param tlist : target list.
    * @param indx : index of item to be removed.
    */
extern int cn_listRemoveAt(cn_list *tlist, int indx);

/**
    * Look for Items matching in the list and remove it from the list.
    * @param tlist : Target list.
    * @param item : Items to be removed.
    * @param cnt : How much items to be removed.
    * return 0 for fine result.
    */
extern int cn_listRemove(cn_list *tlist, void *item, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt, cn_syncFuncPTR itemDestructor);

/**
    * Expand the list buffer capacity (maxCnt property)
    * @param tlist : target list.
    * @param cnt : how much capacity will expand.
    */
extern int cn_listExpand(cn_list *tlist, int len);

/**
    * Remove all items in list.
    * @param tlist : target list.
    * @param itemDestructor : destructor for the item, set NULL for dont destruct.
    */
extern int cn_listEmpty(cn_list *tlist, cn_syncFuncPTR itemDestructor);

/**
    * Insert items to the list at index.
    * @param tlist : target list.
    * @param indx : position where to insert the items.
    * @param items : item objects array.
    * @param cnt : how much item is goint to be added.
    */
extern int cn_listInsertAt(cn_list *tlist, int indx, void *items, int CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt);

/* QUEUE */

/**
    * construct cn_queue object.
    * @param refname : reference name. (variable name)
    * @param perSize : sizeof item to store.
    */
extern cn_queue *cn_makeQue(const char *refname, uint16_t perSize);

/**
    * Flush all item in queue.
    * @param tque : cn_queue object to be flushed.
    * @param itemDestructor : destructor for each item in queue.
    */
extern int cn_queEmpty(cn_queue *tque, cn_syncFuncPTR itemDestructor);

/**
    * Destruct cn_queue object.
    * @param tque : object to be destructed.
    * @param itemDestructor : destructor for each item in queue.
    */
extern int cn_desQue(cn_queue *tque, cn_syncFuncPTR itemDestructor);

/**
    * Enqueue object to queue.
    * @param tque : queue object.
    * @param item : item to enqueue.
    */
extern int cn_queEn(cn_queue *tque, void *item);

/**
    * Dequeue object from queue.
    * @param tque; queue object.
    */
extern void *cn_queDe(cn_queue *tque);

/* DICTIONARY 4bytes key */

/**
    * Construct Dictionary 4bytes-key object structure.
    * @param refname : reference name.
    * return NULL for error
    */
extern cn_dictionary4b *cn_makeDict4b(const char *refname);

/**
    * Get the index in dictionary mapping containing key.
    * @param tdict : target dictionary
    * @param key : key to look for in key list
    * return < 0 for not found
    */
extern int cn_dict4bIndexOfKey(cn_dictionary4b *tdict, uint32_t key);

/**
    * Get the index in dictionary mapping containing value
    * @param tdict : target dictionary
    * @param value : value to look for in value list
    * return < 0 for not found
    */
extern int cn_dict4bIndexOfValue(cn_dictionary4b *tdict, void *value);

/**
    * Set/Add(if key not exist) key-value pair item to dictionary.
    * @param tdict : target dictionary
    * @param key : key for dictionary item
    * @param value : value for dictionary item
    * return 0 for fine
    */
extern int cn_dict4bSet(cn_dictionary4b *tdict, uint32_t key, void *value);

/**
    * Get dictionary item value from dictionary by key
    * @param tdict : target dictionary
    * @param key : key to find the item value
    * return NULL for not found
    */
extern void *cn_dict4bGet(cn_dictionary4b *tdict, uint32_t key);

/**
    * Get dictionary item value from dictionary by index
    * @param tdict : target dictionary
    * @param indx : index of the item
    * return NULL for out of index range
    */
extern void *cn_dict4bGetByIndex(cn_dictionary4b *tdict, int indx);

/**
    * Get the key for dictionary item that has matching dictionary item value
    * @param tdict : target dictionary
    * @param value : dictionary item value for search input
    * @param out : unsigned 32bit integer pointer to hold result dictionary item key
    * return 0 for FOUND
    */
extern int cn_dict4bGetKey(cn_dictionary4b *tdict, void *value, uint32_t *out);

/**
    * remove dictionary item by their key
    * @param tdict : target dictionary
    * @param key : key of dictionary item to delete
    * return 0 for fine
    */
extern int cn_dict4bRemoveByKey(cn_dictionary4b *tdict, uint32_t key);

/**
    * remove dictionary item by their value
    * @param tdict : target dictionary
    * @param value : value of dictionary item to delete
    * return 0 for fine
    */
extern int cn_dict4bRemoveByValue(cn_dictionary4b *tdict, void *value);

/**
    * remove dictionary item by their index
    * @param tdict : target dictionary
    * @param indx : index of dictionary item to delete
    * return 0 for fine
    */
extern int cn_dict4bRemoveByIndex(cn_dictionary4b *tdict, int indx);

/**
    * Remove all items in dictionary
    * @param tdict : target dictionary
    * @param itemDestructor : sync function for destruction of dictionary item
    * return 0 for fine
    */
extern int cn_dict4bEmpty(cn_dictionary4b *tdict, cn_syncFuncPTR itemDestructor);

/**
    * Destruct cn_dictionary4b object structure
    * @param tdict : target dictionary
    * return 0 for fine
    */
extern int cn_desDict4b(cn_dictionary4b *tdict);

#endif
