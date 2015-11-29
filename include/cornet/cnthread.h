#ifndef _CNTHREAD_H
#define _CNTHREAD_H 1

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "cornet/cnsched.h"
#include "cornet/cntype.h"
#include "cornet/cnnum.h"

/* THREADPOOL */

/**
    * Threadpool is a grouped running thread
    * that is waiting to get job to do,
    * allowing multithreading proccessing.
    * Definition of cn_threadpool structure
    */
typedef struct
{
    const char *refname; // Reference name, variable name
    cn_list *threads; // worker list
    uint64_t lastThreadId; // last used thread id
    uint32_t workingThread; // how much worker got job, important for threadpool auto enhance.
    cn_queue *actions; // Jobs queue
    pthread_mutex_t jobsKey; // jobs queue access key
    pthread_mutex_t threadWorkerListKey; // worker list access key
    pthread_cond_t jobsCond; // jobs queue condition
    pthread_attr_t threadAttr; // worker thread attribute
} cn_threadpool;

/**
    * cn_threadpoolWorker is worker thread for threadpool
    * each thread will update their status to this object
    */
typedef struct
{
    const char *refname; // variable name
    pthread_t thread_t; // pthread id
    bool gotThplWork; // indicating the worker working status
    bool running; // indicating permision to work
    uint64_t threadid; // threadpool specified thread id
    cn_threadpool *parent; // the threadpool that hold this worker
} cn_threadpoolWorker;

#include "cornet/cndebug.h"
#define CN_DEBUG_MODE_CNTHREAD_H_LVL 1
/* THREADPOOL */

/**
    * Initialize new threadpool.
    * @param refname : object reference name.
    * @param threadCnt : first thread count.
    * return NULL for failure.
    */
extern cn_threadpool *cn_makeThpl(const char *refname, int threadCnt);

/**
    * Enqueue a job to threadpool.
    * @param thpl : target threadpool.
    * @param action : the job to enqueue.
    * @param return 0 for fine.
    */
extern int cn_thplEnq(cn_threadpool *thpl, cn_action *action);

/**
    * Decrease threadpool worker
    * @param thpl : target threadpool.
    * @param decCent : how much worker to subtract.
    */
extern int cn_thplDecWorker(cn_threadpool *thpl, int decCnt);

/**
    * Decrease threadpool worker Asynchronously, execute callback when it finished.
    * @param thpl : target threadpool.
    * @param decCnt : how much worker to subtract.
    * @param callback : the action to do after task complete in any condition.
    */
extern int cn_thplDecWorkerAsync(cn_threadpool *thpl, int decCnt, cn_action *callback);

/**
    * Increase threadpool worker.
    * @param thpl : target threadpool.
    * @param incCnt : how much worker to add.
    * return 0 fine, 1 acceptable, -1 failure.
    */
extern int cn_thplIncWorker(cn_threadpool *thpl, int incCnt);

/**
    * Increase threadpool worker Asynchronously.
    * @param thpl : target threadpool.
    * @param incCnt : how much worker to add.
    * @param callback : the action to do after task complete in any condition.
    */
extern int cn_thplIncWorkerAsync(cn_threadpool *thpl, int incCnt, cn_action *callback);

/**
    * cancel all the waiting jobs to be done.
    * @param thpl : target threadpool.
    */
extern int cn_thplCancelAll(cn_threadpool *thpl);

/**
    * Destruc threadpool object.
    * all jobs will be canceled, and worker will be terminated.
    * @param thpl : target threadpool
    * return 0 for fine
    */
extern int cn_desThpl(cn_threadpool *thpl);

/**
    * Destruc threadpool object.
    * all jobs will be canceled, and worker will be terminated.
    * @param thpl : target threadpool.
    * @param callback : the action to do after task complete in any condition.
    */
extern int cn_desThplAsync(cn_threadpool *thpl, cn_action *callback);

#endif
