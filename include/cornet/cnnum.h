#ifndef _CNNUM_H
#define _CNNUM_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <pthread.h>

#define CN_NEVER_USE_THIS_WITH_LIST_APPENDADDRESS
#define CN_NOT_USED_FOR_LIST_APPENDADDRESS

/* LIST */

typedef struct
{
    void **b;
    uint32_t blen;
    uint16_t perSize;
    uint32_t cnt;
    uint32_t maxCnt;
    bool appendTheAddress;
    pthread_mutex_t key;
} cn_list;

/* QUEUE */

struct cn_queueNode
{
    void *item;
    struct cn_queueNode *next;
};

typedef struct
{
    struct cn_queueNode *frontNode;
    struct cn_queueNode *rearNode;
    uint16_t perSize;
    uint32_t cnt;
    pthread_mutex_t key;
    bool appendTheAddress;
} cn_queue;

#include "cornet/cntype.h"
#include "cornet/cndebug.h"
#include "cornet/cnbitop.h"

#define CN_DEBUG_MODE 1

extern int cn_cnnumDefaultItemDestructor(void *item);

/* LIST */

extern cn_list *cn_makeList(uint16_t perSize, uint32_t firstMaxCnt, bool appendTheAddress);

extern int cn_desList(cn_list *tlist);

extern int cn_listAppend(cn_list *tlist, void *item);

extern void *cn_listGet(cn_list *tlist, uint32_t indx);

extern int cn_listSet(cn_list *tlist, uint32_t indx, void *item);

extern int cn_listIndexOf(cn_list *tlist, void *item, uint16_t CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt);

extern bool cn_listContains(cn_list *tlist, void *item, uint16_t CN_NOT_USED_FOR_LIST_APPENDADDRESS cnt);

extern int cn_listSplice(cn_list *tlist, uint32_t startIndx, uint16_t len, cn_syncFuncPTR itemDestructor);

extern int cn_listRemoveAt(cn_list *tlist, uint32_t indx);

extern int cn_listExpand(cn_list *tlist, uint32_t len);

extern int cn_listEmpty(cn_list *tlist, cn_syncFuncPTR itemDestructor);

extern int CN_NEVER_USE_THIS_WITH_LIST_APPENDADDRESS cn_listInsertAt(cn_list *tlist, uint32_t indx, void *items, uint16_t cnt);

/* QUEUE */

extern cn_queue *cn_makeQue(uint16_t perSize);

extern int cn_queEmpty(cn_queue *tque, cn_syncFuncPTR itemDestructor);
extern int cn_desQue(cn_queue *tque, cn_syncFuncPTR itemDestructor);

extern int cn_queEn(cn_queue *tque, void *item);
extern void *cn_queDe(cn_queue *tque);

#endif
