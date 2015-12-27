#include "cornet/cnthread.h"

static struct timespec __zeroTime = {.tv_sec = 0, .tv_nsec = 0};

/** threadpool type id */
cn_type_id_t cn_threadpool_type_id = 0;

/**
    * Argument structure for AsyncThread Function
    */
struct asyncThreadArgs
{
    cn_syncFuncPTR syncTask; // Sync Task that will be executed
    void *args; // Arguments for Sync Task
    cn_action *callback; // Action to take if Sync Task success
    pthread_t tid; // Thread id for AsyncTask Execution.
};

/**
    * A Sync Task Warping Frame, make it Async Task
    */
static void *asyncThread(void *args)
{
    // Parse the arguments
    struct asyncThreadArgs *arguments = args;
    if(arguments->syncTask(arguments->args) == 0)
    {
        // If sync task success, execute callback.
        if(arguments->callback != NULL)
        {
            // If callback is not requested to cancell. then execute it
            if(!arguments->callback->cancel)
            {
                arguments->callback->funcptr(arguments->callback->args);
            }
            // This is where callback action destroyed.
            cn_desAction(arguments->callback);
        }
    }
    // finnaly dealloc the arguments then exit thread.
    free(arguments);
    return NULL;
}

/**
    * Argument structure for endTryAttempt Function
    */
struct endTryAttemptArgs
{
    bool cancel; // Cancel the operation
    bool *trigg; // Triger for the operation
};

/**
    * This function is used to pull a trigger to false
    * When an operation trying to do something until it success
    * this function will limit how long the operation will
    * keep trying. This is usualy called from scheduler.
    */
static void *endTryAttempt(void *args)
{
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker end flushing start phase %d\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    // Parse the arguments
    struct endTryAttemptArgs *targs = args;
    if(targs->cancel == false)
    {
        // We are allowed to pull the trigger.
        // set trigger to false.
        *targs->trigg = false;
    }
    // Dealocate arguments
    free(targs);
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker end flushing end phase %d\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    return NULL;
}


/**
    * Threadpool thread working frame.
    * This frame receive job from threadpool
    * queue and execute it.
    * Argument is cn_threadpoolWorker structure.
    */
static void *threadWork(void *arg)
{
    // Parse the arguments.
    cn_threadpoolWorker *thread = arg;

    // Get the job queue lock key and condition.
    cn_mutex_t *key = &thread->parent->jobsKey;
    cn_cond_t *cond = &thread->parent->jobsCond;

    // Get the working worker counter
    uint32_t *workingThread = &thread->parent->workingThread;

    // Get the job queue.
    cn_queue *actions = thread->parent->actions;

    // Get the thread activity variable address
    bool *gotThplWork = &thread->gotThplWork;

    // Set the thread running status
    thread->running = true;
    bool *running = &thread->running;

    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("worker thread %d created\n", (int)thread->threadid);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    while(*running == true)
    {
        // Thread is still allowed to run, keep doing our job.
        if(__sync_fetch_and_add(&actions->cnt, 0) == 0)
        {
            #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
            cn_log("worker thread %d entering wait\n", (int)thread->threadid);
            #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

            // Lock the jobs queue
            cn_mutex_lock(key);

            // The job queue is empty. lets wait for a change of condition.
            cn_cond_wait(cond, key);

            // Unlock the jobs queue
            cn_mutex_unlock(key);

            if(!*running)
            {
                // Condition has changed, it seems this thread is not allowed
                // to run anymore, go to thread termination.
                // cn_mutex_unlock(key);
                break;
            }
        }
        // Dequeue job from jobs queue
        cn_action *work = cn_typeGet(cn_queDe(actions), cn_action_type_id);

        if(work != NULL)
        {
            if(!work->cancel && work->funcptr != NULL)
            {
                // the job is not null, and not requested to cancel.
                // set the thread activity to got work, add wokringTask
                // counter by one and do the job.
                //*gotThplWork = true;
                __sync_bool_compare_and_swap(gotThplWork, false, true);
                //int tmpint = *workingThread;
                //tmpint++;
                //*workingThread = tmpint;
                __sync_add_and_fetch(workingThread, 1);
                #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
                cn_log("thread %d got work\n", (int)thread->threadid);
                #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
                work->funcptr(work->args);

                // we are done working. subtract the workingThread by one.
                //tmpint = *workingThread;
                //tmpint--;
                //*workingThread = tmpint;
                __sync_bool_compare_and_swap(gotThplWork, true, false);
                __sync_sub_and_fetch(workingThread, 1);
            }
            // call the job destructor
            cn_desAction(work);
        }
        else
        {
            #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
            cn_log("worker thread %d got false signal with no work inside\n", (int)thread->threadid);
            #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
            // We got a null reference of job. let other thread do their stuff.
            cn_sleep(1);
        }
        // we have done our job. lets set our activity to dont get a job.
        //*gotThplWork = false;
    }
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("worker thread %d got termination sign\n", (int)thread->threadid);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

    /*  We are about to terminate thread.
        to allow better debugging. we need to know the index of our thread
        in the threadpool, so we need to lock the thread list for termination
        at this level. so we can get the index and remove it safely.
        */
    cn_mutex_lock(&thread->parent->threadWorkerListKey);

    // get the thread list
    cn_list *threads = thread->parent->threads;

    // find our thread index
    int myIndx = cn_listIndexOf(threads, thread, 0);

    // remove our thread from list
    cn_listRemoveAt(threads, myIndx);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("worker thread %d removing index %d from thread list %d, GOODBYE\n", (int)thread->threadid, myIndx, threads->cnt);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

    // finnaly unlock the list
    cn_mutex_unlock(&thread->parent->threadWorkerListKey);

    // dealloc the thread structure
    free(thread);
    return NULL;
}

/**
    * Initialize new threadpool.
    * @param refname : object reference name.
    * @param threadCnt : first thread count.
    * return NULL for failure.
    */
cn_threadpool *cn_makeThpl(const char *refname, int threadCnt)
{
    // Defensive code
    if(threadCnt < 0){return NULL;}

    // Allocate threadpool structure, defensively.
    cn_threadpool *result = malloc(sizeof(cn_threadpool));
    if(result == NULL){return NULL;}

    // Make cn_threadpoolWorker list, defensively.
    result->threads = cn_makeList("cn_threadpool.threads", sizeof(cn_threadpoolWorker *), (threadCnt * 2), true);
    if(result->threads == NULL){free(result); return NULL;}

    // Set the reference name.
    result->refname = refname;

    // Initialize keys and condition.
    cn_mutex_init(&result->jobsKey, NULL);
    cn_mutex_init(&result->threadWorkerListKey, NULL);
    cn_cond_init(&result->jobsCond, NULL);
    pthread_attr_init(&result->threadAttr);
    pthread_attr_setdetachstate(&result->threadAttr, PTHREAD_CREATE_DETACHED);

    // Make cn_queue of cn_action for jobs queue, defensively.
    result->actions = cn_makeQue("cn_threadpool.jobs", sizeof(cn_action *));
    if(result->actions == NULL){goto reterr1;}

    // Increase the cn_threadpoolWorker, defensively.
    if(cn_thplIncWorker(result, threadCnt) < 0){goto reterr2;}

    // if threadpool type has no identifier, request identifier.
    if(cn_threadpool_type_id < 1){cn_threadpool_type_id = cn_typeGetNewID();}

    // initialize object type definition
    cn_typeInit(&result->t, cn_threadpool_type_id);

    // return Result.
    return result;

    // Error handler, destroy all that were created here.
    reterr2:
    cn_desQue(result->actions, NULL);
    reterr1:
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 1
    cn_log("threadpool creation error %s\n", refname);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    cn_desList(result->threads);
    cn_mutex_destroy(&result->jobsKey);
    cn_mutex_destroy(&result->threadWorkerListKey);
    cn_cond_destroy(&result->jobsCond);
    pthread_attr_destroy(&result->threadAttr);
    free(result);
    return NULL;
}

/**
    * Enqueue a job to threadpool.
    * @param thpl : target threadpool.
    * @param action : the job to enqueue.
    * @param return 0 for fine.
    */
int cn_thplEnq(cn_threadpool *thpl, cn_action *action)
{
    // Defensive code
    if(thpl == NULL || action == NULL)
    {return -1;}

    // Lock the jobs queue
    //cn_mutex_lock(&thpl->jobsKey);

    // Enqueue the job to jobs queue, if successful, signal waiting thread.
    if(cn_queEn(thpl->actions, action) == 0)
    {
        cn_cond_signal(&thpl->jobsCond);
        //nanosleep(&__zeroTime, NULL);
    }
    else
    {
        // If failed, unlock the jobs queue, then return failure.
        //cn_mutex_unlock(&thpl->jobsKey);
        return -1;
    }
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
    cn_log("threadpool enq success\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

    // enqueue job finished, unlock the queue then return fine.
    //cn_mutex_unlock(&thpl->jobsKey);
    return 0;
}

/**
    * cancel all the waiting jobs to be done.
    * @param thpl : target threadpool.
    */
int cn_thplCancelAll(cn_threadpool *thpl)
{
    // lock the jobs queue
    cn_mutex_lock(&thpl->jobsKey);

    // empty the jobs queue.
    int n = cn_queEmpty(thpl->actions, cn_desActionNumerableInterface);

    // unlock the jobs queue then return execution status.
    cn_mutex_unlock(&thpl->jobsKey);
    return n;
}

/**
    * Argument structure for ThplIncWOrkerRealWork function
    */
struct ThplIncWorkerRealWorkArgs
{
    cn_threadpool *thpl; // Target threadpool.
    int incCnt; // the number of thread to add.
};

/**
    * Increase the threadpool worker.
    * @param args : pointer of ThplIncWOrkerRealWorkArgs structure.
    * return 0 fine, 1 acceptable, -1 failure.
    */
static int ThplIncWorkerRealWork(void *args)
{
    // Parse the arguments.
    struct ThplIncWorkerRealWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    int incCnt = targs->incCnt;

    // free the arguments holder.
    free(args);

    // declare iterator and execution status.
    int i;
    int retCode = 0;

    // lock the thread worker list.
    cn_mutex_lock(&thpl->threadWorkerListKey);

    // record the thread count before adding new thread.
    int lastWorkerCnt = thpl->threads->cnt;
    for(i = 0; i < incCnt; i++)
    {
        // Allocate a structure of cn_threadpool_worker, defensively.
        cn_threadpoolWorker *thread = malloc(sizeof(cn_threadpoolWorker));
        if(thread == NULL){continue;}

        // set default value.
        thread->gotThplWork = false;
        thread->parent = thpl;
        thread->threadid = thpl->lastThreadId++;

        // Creating new thread
        int rc = pthread_create(&thread->thread_t, &thpl->threadAttr, threadWork, (void *)thread);
        if(rc)
        {
            // something wrong with creating thread. set return code to 1 (acceptable).
            retCode = 1;
            #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
            cn_log("pthread_create threadpool make error code %d", rc);
            #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
        }
        else
        {
            #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
            cn_log("pthread_create called for threadid %d ptr %d\n", thread->threadid, (int)thread);
            #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

        }

        // append the new worker thread to threadpool worker list.
        cn_listAppend(thpl->threads, thread);
    }
    // if the worker count doesn't increase at all, set return code to -1 (failure).
    if(lastWorkerCnt >= thpl->threads->cnt){retCode = -1;}

    // unlock the threadpool worker list.
    cn_mutex_unlock(&thpl->threadWorkerListKey);

    // return execution status
    return retCode;
}

/**
    * Increase threadpool worker.
    * @param thpl : target threadpool.
    * @param incCnt : how much worker to add.
    * return 0 fine, 1 acceptable, -1 failure.
    */
int cn_thplIncWorker(cn_threadpool *thpl, int incCnt)
{
    // Defensive code.
    if(thpl == NULL || incCnt < 0){return -1;}

    // Prepare the argument
    struct ThplIncWorkerRealWorkArgs *args = malloc(sizeof(struct ThplIncWorkerRealWorkArgs));
    if(args == NULL){return -1;}
    args->thpl = thpl;
    args->incCnt = incCnt;

    // execute the real work.
    return ThplIncWorkerRealWork(args);
}

/**
    * Increase threadpool worker Asynchronously.
    * @param thpl : target threadpool.
    * @param incCnt : how much worker to add.
    * @param callback : the action to do after task complete in any condition.
    */
int cn_thplIncWorkerAsync(cn_threadpool *thpl, int incCnt, cn_action *callback)
{
    // Defensive code.
    if(thpl == NULL || incCnt < 0){return -1;}

    // Prepare the arguments
    struct ThplIncWorkerRealWorkArgs *incWorkArgs = malloc(sizeof(struct ThplIncWorkerRealWorkArgs));
    if(incWorkArgs == NULL){return -1;}
    incWorkArgs->thpl = thpl;
    incWorkArgs->incCnt = incCnt;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = ThplIncWorkerRealWork;
    args->args = incWorkArgs;
    args->callback = callback;

    // run async thread
    int n = pthread_create(&args->tid, NULL, asyncThread, (void *)args);
    pthread_detach(args->tid);
    // return real work new thread execution status
    return n;
}

/**
    * Argument structure for ThplDecWorkerRealWork function.
    */
struct ThplDecWorkerRealWorkArgs
{
    cn_threadpool *thpl; // target threadpool
    int decCnt; // how much thread to subtract
};

/**
    * Decrease threadpool worker
    * @param args : argument structure of ThplDecWorkerRealWorkArgs
    * return 0 for fine, and -1 for failure.
    */
static int ThplDecWorkerRealWork(void *args)
{
    // Parse the argument
    struct ThplDecWorkerRealWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    int decCnt = targs->decCnt;

    // deallocate argument envelope
    free(args);

    // Defensive code.
    if(thpl == NULL || decCnt < 0){return -1;}
    if(decCnt > thpl->threads->cnt)
    {
        // the decrement count is higher than worker count
        // set decrement count by worker count
        decCnt = thpl->threads->cnt;
    }

    //Make list of thread that's not doing job to be terminated. defensively.
    cn_list *threadRunning = cn_makeList("cn_thplDecWorker.threadRunning", sizeof(cn_threadpoolWorker), decCnt, true);
    cn_list *threadIddling = cn_makeList("cn_thplDecWorker.threadIddling", sizeof(cn_threadpoolWorker), decCnt, true);
    if(threadRunning == NULL || threadIddling == NULL){return -1;}

    // declare iterator
    int i;

    // lock the worker list, and job queue
    cn_mutex_lock(&thpl->threadWorkerListKey);
    cn_mutex_lock(&thpl->jobsKey);

    // get the array of worker pointer.
    cn_threadpoolWorker **threads = *thpl->threads->b;
    for(i = 0; i < thpl->threads->cnt; i++)
    {
        if(!threads[i]->gotThplWork)
        {
            // this worker is not doing a job. lets give it termination sign.
            // and add it to the iddling worker list
            cn_listAppend(threadIddling, threads[i]);
            threads[i]->running = false;
            if(threadIddling->cnt >= decCnt)
            {
                // break the loop for signaling termination because, has sufficent to decCnt
                break;
            }
        }
        else
        {
            // this worker is doing a job. add to running worker list
            cn_listAppend(threadRunning, threads[i]);
        }
    }
    if(decCnt > threadIddling->cnt)
    {
        // seems like by terminating the iddling worker alone is not sufficent to decCnt
        // lets terminate some of the running worker.

        // get the array of running worker pointer.
        cn_threadpoolWorker **threadRunningB = (cn_threadpoolWorker **)threadRunning->b;

        // count how much we need to take from running worker
        int leftToTerminate = decCnt - threadIddling->cnt;
        for(i = 0; i < leftToTerminate; i++)
        {
            // give the running thread termination sign.
            threadRunningB[i]->running = false;
        }
    }
    // unlock the worker list and jobs queue.
    cn_mutex_unlock(&thpl->jobsKey);
    cn_mutex_unlock(&thpl->threadWorkerListKey);

    // declare endTryAttempt Function trigger, defensively.
    bool *stopFlushing = malloc(sizeof(bool));
    if(stopFlushing == NULL){return -1;}

    // set the trigger to untriggered
    *stopFlushing = false;

    // prepare the argument structure for endTryAttempt funciton, defensively.
    struct endTryAttemptArgs *endTryArgs = malloc(sizeof(struct endTryAttemptArgs));
    // if fail to allocate, return acceptable error
    // the thread will be terimated and removed from the workerlist eventually.
    if(endTryArgs == NULL){return 1;}
    endTryArgs->trigg = stopFlushing;
    endTryArgs->cancel = false;

    // add endTryAttempt to schedule.
    cn_doDelayedAction(cn_makeAction("cn_thplDecWorker.stopFlusing", endTryAttempt, endTryArgs, NULL), 30);

    // declare the next worker count
    int nextThreadsCount = thpl->threads->cnt - decCnt;
    if(nextThreadsCount < 0){nextThreadsCount = 0;}

    // declare try attempt counter.
    int tryAttempt = 0;
    while(*stopFlushing == false)
    {
        #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
        cn_log("keep flush %d cnthread DEBUG cn_thplDecWorker begining flush at try attempt %d, cnt %d target %d\n", *stopFlushing, tryAttempt, thpl->threads->cnt, nextThreadsCount);
        #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

        // increment try attempt count
        tryAttempt++;

        // lock the jobs queue
        cn_mutex_lock(&thpl->jobsKey);
        for(i = 0; i < (thpl->threads->cnt * 5); i++)
        {
            // signal worker that condition has changed
            cn_cond_signal(&thpl->jobsCond);
        }
        // unlock the jobs queue
        cn_mutex_unlock(&thpl->jobsKey);

        // let other thread run, wait for 20ms
        cn_sleep(20);
        if(nextThreadsCount >= thpl->threads->cnt)
        {
            #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
            cn_log("cnthread DEBUG cn_thplDecWorker complete flush at try attempt %d\n", tryAttempt);
            #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
            // flush has completed.
            break;
        }
        // take controll back after 20ms, but flush is not complete, retry.
    }

    if(*stopFlushing == false)
    {
        // because operation complete before endTryAttempt Func pull trigger to true.
        // tell it to not pull the trigger. because we will dealloc the trigger here.
        endTryArgs->cancel = true;
    }
    // dealloc endTryAttempt trigger
    free(stopFlushing);

    // destroy temporary worker list.
    cn_desList(threadRunning);
    cn_desList(threadIddling);

    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 1
    cn_log("cn_thplDecWorker finished for %s with worker count %d. GOODBYE.\n", thpl->refname, thpl->threads->cnt);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

    // return fine
    return 0;
}

/**
    * Decrease threadpool worker
    * @param thpl : target threadpool.
    * @param decCent : how much worker to subtract.
    */
int cn_thplDecWorker(cn_threadpool *thpl, int decCnt)
{
    // Defensive code.
    if(thpl == NULL || decCnt < 0){return -1;}

    // Prepare argument structure
    struct ThplDecWorkerRealWorkArgs *args = malloc(sizeof(struct ThplDecWorkerRealWorkArgs));
    if(args == NULL){return -1;}
    args->thpl = thpl;
    args->decCnt = decCnt;

    // return real work execution status
    return ThplDecWorkerRealWork(args);
}

/**
    * Decrease threadpool worker Asynchronously, execute callback when it finished.
    * @param thpl : target threadpool.
    * @param decCnt : how much worker to subtract.
    * @param callback : the action to do after task complete in any condition.
    */
int cn_thplDecWorkerAsync(cn_threadpool *thpl, int decCnt, cn_action *callback)
{
    // Defensive code
    if(thpl == NULL || decCnt < 1){return -1;}

    // Prepare argument structure
    struct ThplDecWorkerRealWorkArgs *decWorkerArgs = malloc(sizeof(struct ThplDecWorkerRealWorkArgs));
    if(decWorkerArgs == NULL){return -1;}
    decWorkerArgs->thpl = thpl;
    decWorkerArgs->decCnt = decCnt;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = ThplDecWorkerRealWork;
    args->args = decWorkerArgs;
    args->callback = callback;

    // run async thread
    int n = pthread_create(&args->tid, NULL, asyncThread, (void *)args);
    pthread_detach(args->tid);
    // return real work new thread execution status
    return n;
}

/**
    * Argument structure for desThplRealWork function.
    */
struct desThplRealWorkArgs
{
    cn_threadpool *thpl; // target threadpool
    cn_syncFuncPTR itemDestructor; // jobs item destructor
};

/**
    * Destruc threadpool object.
    * all jobs will be canceled, and worker will be terminated.
    * @param args : argument structure of desThplRealWorkArgs
    */
static int desThplRealWork(void *args)
{
    // Parse the argument.
    struct desThplRealWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    cn_syncFuncPTR itemDestructor = targs->itemDestructor;

    // dealloc argument envelope
    free(args);

    //defensive code
    if(thpl == NULL){return -1;}

    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
    cn_log("desThplTrueWprl started for %d with worker count %d.\n", (int)thpl, thpl->threads->cnt);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL

    // declare execution status
    int n;

    // cancel all waiting jobs
    n = cn_thplCancelAll(thpl);
    if(n < 0){return n;}

    // terminate all worker
    n = cn_thplDecWorker(thpl, thpl->threads->cnt);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
    cn_log("desThplTrueWprl after decwork.\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    if(n < 0){return n;}

    // destruct worker list
    n = cn_desList(thpl->threads);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
    cn_log("desThplTrueWprl after deslist.\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    if(n < 0){return n;}

    // destruct jobs queue
    n = cn_desQue(thpl->actions, itemDestructor);
    if(n != 0){return n;}

    // destroy all keys and condition
    cn_mutex_destroy(&thpl->jobsKey);
    cn_mutex_destroy(&thpl->threadWorkerListKey);
    cn_cond_destroy(&thpl->jobsCond);

    // Destroy type definition
    cn_typeDestroy(&thpl->t);

    // finnaly dealloc threadpool structure.
    free(thpl);

    //return fine.
    return 0;
}

/**
    * Destruc threadpool object.
    * all jobs will be canceled, and worker will be terminated.
    * @param thpl : target threadpool
    * return 0 for fine
    */
int cn_desThpl(cn_threadpool *thpl)
{
    struct desThplRealWorkArgs *targs = malloc(sizeof(struct desThplRealWorkArgs));
    targs->thpl = thpl;
    targs->itemDestructor = cn_desActionNumerableInterface;
    return desThplRealWork(targs);
}

/**
    * Destruc threadpool object.
    * all jobs will be canceled, and worker will be terminated.
    * @param thpl : target threadpool.
    * @param callback : the action to do after task complete in any condition.
    */
int cn_desThplAsync(cn_threadpool *thpl, cn_action *callback)
{
    // Defensive code
    if(thpl == NULL){return -1;}

    // Prepare argument structure
    struct desThplRealWorkArgs *targs = malloc(sizeof(struct desThplRealWorkArgs));
    if(targs == NULL){return -1;}
    targs->thpl = thpl;
    targs->itemDestructor = cn_desActionNumerableInterface;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = desThplRealWork;
    args->args = targs;
    args->callback = callback;

    // run async thread
    int n = pthread_create(&args->tid, NULL, asyncThread, (void *)args);
    pthread_detach(args->tid);
    // return real work new thread execution status
    return n;
}




