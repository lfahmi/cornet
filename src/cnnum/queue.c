#include "cornet/cnnum.h"

/**
    * construct cn_queue object.
    * @param perSize : sizeof item to store.
    */
cn_queue *cn_makeQue(uint16_t perSize)
{
    /// allocate memory for object, set default value. return NULL if fails.
    cn_queue *result = malloc(sizeof(cn_queue));
    /// initialize key for queue
    pthread_mutex_init(&result->key, NULL);
    result->cnt = 0;
    result->perSize = 0;
    result->frontNode = NULL;
    result->rearNode = NULL;
    return result;
}

/**
    * Flush all item in queue.
    * @param tque : cn_queue object to be flushed.
    * @param itemDestructor : destructor for each item in queue.
    */
int cn_queEmpty(cn_queue *tque, cn_syncFuncPTR itemDestructor)
{
    /// Defensive code.
    if(tque == NULL){return -1;}
    /// keep looping until count reach zero.
    while(tque->cnt > 0)
    {
        void *item = cn_queDe(tque);
        if(itemDestructor != NULL){itemDestructor(item);}
    }
    /// return fine.
    return 0;
}

/**
    * Destruct cn_queue object.
    * @param tque : object to be destructed.
    * @param itemDestructor : destructor for each item in queue.
    */
int cn_desQue(cn_queue *tque, cn_syncFuncPTR itemDestructor)
{
    /// Make the queue empty first.
    cn_queEmpty(tque, itemDestructor);
    /// Destroy queue key
    pthread_mutex_destroy(&tque->key);
    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNNUM_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d] dealloc attempt next.\n", __FILE__, __func__, __LINE__);
    #endif // CN_DEBUG_MODE
    // CRASH POTENTIAL!
    /// Finnaly dealocaty cn_queue object.
    free(tque);
    return 0;
}

/**
    * Enqueue object to queue.
    * @param tque : queue object.
    * @param item : item to enqueue.
    */
int cn_queEn(cn_queue *tque, void *item)
{
    // Defensive code.
    if(tque == NULL || item == NULL) {return -1;}

    // Allocate queue node object. Defensively.
    struct cn_queueNode *tmpNode = malloc(sizeof(struct cn_queueNode));
    if(tmpNode == NULL){return -1;}

    // set node item by item and next node to null
    tmpNode->item = item;
    tmpNode->next = NULL;

    // lock queue
    pthread_mutex_lock(&tque->key);
    if(tque->rearNode == NULL || tque->frontNode == NULL)
    {
        // this is sole item in queue. set queue count to 0 first
        // set rear node and front node by current node.
        tque->cnt = 0;
        tque->rearNode = tmpNode;
        tque->frontNode = tmpNode;
    }
    else
    {
        // tell latest node that it next node is current node
        tque->rearNode->next = tmpNode;

        // now the latest node is current node
        tque->rearNode = tmpNode;
    }

    // finnaly increase queue count then unlock queue
    tque->cnt++;
    pthread_mutex_unlock(&tque->key);

    // operation finished fine
    return 0;
}

/**
    * Dequeue object from queue.
    * @param tque; queue object.
    */
void *cn_queDe(cn_queue *tque)
{
    // Defensive code.
    if(tque == NULL){return NULL;}
    if(tque->cnt < 1){return NULL;}
    if(tque->frontNode == NULL){return NULL;}

    // declare temporary node, set by front node of queue.
    struct cn_queueNode *tmpNode = tque->frontNode;

    // result item set by tmp node item.
    void *result = tmpNode->item;

    // lock the queue
    pthread_mutex_lock(&tque->key);
    if(tque->frontNode == tque->rearNode)
    {
        // seemslike tmp node was the only node in queue
        // set front and rear node to null
        tque->frontNode = NULL;
        tque->rearNode = NULL;
    }
    else
    {
        // tmp node is used. now the front node
        // must be the next node of the tmp node
        tque->frontNode = tmpNode->next;
    }
    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNNUM_H_DEEP == 1
    cn_log("[DEBUG][file:%s][func:%s][line:%d] dealloc attempt next.\n", __FILE__, __func__, __LINE__);
    #endif // CN_DEBUG_MODE
    free(tmpNode);

    // finnaly decrease queue count then we can unlock the queue
    tque->cnt--;
    pthread_mutex_unlock(&tque->key);

    // return the result
    return result;
}
