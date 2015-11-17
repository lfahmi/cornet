#include "cornet/cnnum.h"

cn_queue *cn_makeQue(uint16_t perSize)
{
    cn_queue *result = malloc(sizeof(cn_queue));
    result->cnt = 0;
    result->perSize = 0;
    result->frontNode = NULL;
    result->rearNode = NULL;
    return result;
}

int cn_queEmpty(cn_queue *tque, cn_syncFuncPTR itemDestructor)
{
    if(tque == NULL){return -1;}
    int cnt = tque->cnt;
    int i;
    for(i = 0; i < cnt; i++)
    {
        void *item = cn_queDe(tque);
        if(itemDestructor != NULL){itemDestructor(item);}
    }
    return 0;
}

int cn_desQue(cn_queue *tque, cn_syncFuncPTR itemDestructor)
{
    #ifdef CN_DEBUG_MODE
    cn_log("cm_desQue has begining for %d\n", (int)tque);
    #endif // CN_DEBUG_MODE
    cn_queEmpty(tque, itemDestructor);
    pthread_mutex_destroy(&tque->key);
    free(tque);
    #ifdef CN_DEBUG_MODE
    cn_log("cm_desQue has finished for %d\n", (int)tque);
    #endif // CN_DEBUG_MODE
    return 0;
}

int cn_queEn(cn_queue *tque, void *item)
{
    if(tque == NULL || item == NULL) {return -1;}
    struct cn_queueNode *tmpNode = malloc(sizeof(struct cn_queueNode));
    tmpNode->item = item;
    tmpNode->next = NULL;
    pthread_mutex_lock(&tque->key);
    if(tque->rearNode == NULL || tque->frontNode == NULL)
    {
        tque->rearNode = tmpNode;
        tque->frontNode = tmpNode;
    }
    else
    {
        tque->rearNode->next = tmpNode;
        tque->rearNode = tmpNode;
    }
    tque->cnt++;
    pthread_mutex_unlock(&tque->key);
    return 0;
}

void *cn_queDe(cn_queue *tque)
{
    if(tque == NULL){return NULL;}
    if(tque->frontNode == NULL){return NULL;}
    struct cn_queueNode *tmpNode = tque->frontNode;
    void *result = tmpNode->item;
    pthread_mutex_lock(&tque->key);
    if(tque->frontNode == tque->rearNode)
    {
        tque->frontNode = NULL;
        tque->rearNode = NULL;
    }
    else
    {
        tque->frontNode = tmpNode->next;
    }
    free(tmpNode);
    tque->cnt--;
    pthread_mutex_unlock(&tque->key);
    return result;
}
