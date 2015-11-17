#include "cornet/cnthread.h"

struct asyncThreadArgs
{
    cn_syncFuncPTR syncTask;
    void *args;
    cn_action *callback;
    pthread_t tid;
};

// warp sync decrease thread worker so we can call this on pthread create
void *asyncThread(void *args)
{
    struct asyncThreadArgs *arguments = args;
    if(arguments->syncTask(arguments->args) == 0)
    {
        if(arguments->callback != NULL)
        {
            if(!arguments->callback->cancel)
            {
                arguments->callback->funcptr(arguments->callback->args);
            }
            cn_desAction(arguments->callback);
        }
    }
    free(args);
    return NULL;
}

// This will be executed on new thread to end some try attempt
// by setting the arg to false and free it.
void *EndTryAttempt(void *args)
{
    #ifdef CN_DEBUG_MODE
    cn_log("cnthread DEBUG cn_thplDecWorker end flushing start phase %d\n");
    #endif // CN_DEBUG_MODE
    bool *keepFlushing = args;
    *keepFlushing = false;
    free(keepFlushing);
    #ifdef CN_DEBUG_MODE
    cn_log("cnthread DEBUG cn_thplDecWorker end flushing end phase %d\n");
    #endif // CN_DEBUG_MODE
    return NULL;
}


// threadpool worker thread function;
void *threadWork(void *arg)
{
    cn_thread *thread = arg;
    pthread_mutex_t *key = &thread->parent->key;
    pthread_cond_t *cond = &thread->parent->cond;
    cn_queue *actions = thread->parent->actions;
    bool *gotThplWork = &thread->gotThplWork;
    thread->running = true;
    bool *running = &thread->running;
    #ifdef CN_DEBUG_MODE
    cn_log("worker thread %d created\n", (int)thread->threadid);
    #endif // CN_DEBUG_MODE
    while(*running == true)
    {
        pthread_mutex_lock(key);
        if(actions->cnt == 0)
        {
            #ifdef CN_DEBUG_MODE
            cn_log("worker thread %d entering wait\n", (int)thread->threadid);
            #endif // CN_DEBUG_MODE
            pthread_cond_wait(cond, key);
        }
        #ifdef CN_DEBUG_MODE
        cn_log("worker thread %d got signal\n", (int)thread->threadid);
        #endif // CN_DEBUG_MODE
        if(!*running)
        {
            pthread_mutex_unlock(key);
            break;
        }
        cn_action *work = cn_queDe(actions);
        pthread_mutex_unlock(key);
        if(work != NULL)
        {
            if(!work->cancel && work->funcptr != NULL)
            {
                *gotThplWork = true;
                #ifdef CN_DEBUG_MODE
                cn_log("thread %d got work\n", (int)thread->threadid);
                #endif // CN_DEBUG_MODE
                work->funcptr(work->args);
            }
            cn_desAction(work);
        }
        else
        {
            #ifdef CN_DEBUG_MODE
            cn_log("worker thread %d got false signal with no work inside\n", (int)thread->threadid);
            #endif // CN_DEBUG_MODE
            cn_sleep(5);
        }
        *gotThplWork = false;
    }
    #ifdef CN_DEBUG_MODE
    cn_log("worker thread %d got termination sign\n", (int)thread->threadid);
    #endif // CN_DEBUG_MODE
    cn_list *threads = thread->parent->threads;
    int myIndx = cn_listIndexOf(threads, thread, 0);
    pthread_mutex_lock(&thread->parent->terminatorKey);
    cn_listRemoveAt(threads, myIndx);
    #ifdef CN_DEBUG_MODE
    cn_log("worker thread %d removing index %d from thread list, GOODBYE\n", (int)thread->threadid, myIndx);
    #endif // CN_DEBUG_MODE
    pthread_mutex_unlock(&thread->parent->terminatorKey);
    free(thread);
    return NULL;
}

// Initialize new threadpool
cn_threadpool *cn_makeThpl(uint32_t threadCnt)
{
    cn_threadpool *result = malloc(sizeof(cn_threadpool));
    if(result == NULL){return NULL;}
    result->threads = cn_makeList(sizeof(cn_thread *), (threadCnt * 2), true);
    if(result->threads == NULL){free(result); return NULL;}
    pthread_mutex_init(&result->key, NULL);
    pthread_mutex_init(&result->terminatorKey, NULL);
    pthread_cond_init(&result->cond, NULL);
    pthread_attr_init(&result->threadAttr);
    cn_thplIncWorker(result, threadCnt);
    /*
    int i;
    for(i = 0; i < threadCnt; i++)
    {
        cn_thread *thread = malloc(sizeof(cn_thread));
        if(thread == NULL){continue;}
        thread->gotThplWork = false;
        thread->parent = result;
        thread->threadid = i;
        int rc = pthread_create(&thread->thread_t, &result->threadAttr, threadWork, (void *)thread);
        if(rc)
        {
            #ifdef CN_DEBUG_MODE
            cn_log("pthread_create threadpool make error code %d", rc);
            #endif // CN_DEBUG_MODE
        }
        else
        {
            #ifdef CN_DEBUG_MODE
            cn_log("pthread_create called for threadid %d ptr %d\n", thread->threadid, (int)thread);
            #endif // CN_DEBUG_MODE
        }
        cn_listAppend(result->threads, thread);
    }
    */
    result->actions = cn_makeQue(sizeof(cn_action *));
    return result;
}

// Add new job for threadpool
int cn_thplEnq(cn_threadpool *thpl, cn_action *action)
{
    pthread_mutex_lock(&thpl->key);
    if(cn_queEn(thpl->actions, action) == 0){pthread_cond_signal(&thpl->cond);}
    else {pthread_mutex_unlock(&thpl->key); return -1;}
    #ifdef CN_DEBUG_MODE
    cn_log("threadpool enq success\n");
    #endif // CN_DEBUG_MODE
    pthread_mutex_unlock(&thpl->key);
    return 0;
}

int cn_thplCancelAll(cn_threadpool *thpl, cn_syncFuncPTR itemDestructor)
{
    pthread_mutex_lock(&thpl->key);
    cn_queEmpty(thpl->actions, itemDestructor);
    pthread_mutex_unlock(&thpl->key);
    return 0;
}

// argument warper for ThplIncWorkerTrueWork
struct ThplIncWorkerTrueWorkArgs
{
    cn_threadpool *thpl;
    int incCnt;
};

// The core of every threadpool worker increment.
int ThplIncWorkerTrueWork(void *args)
{
    struct ThplIncWorkerTrueWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    int incCnt = targs->incCnt;
    free(args);
    int i;
    int retCode = 0;
    for(i = 0; i < incCnt; i++)
    {
        cn_thread *thread = malloc(sizeof(cn_thread));
        if(thread == NULL){continue;}
        thread->gotThplWork = false;
        thread->parent = thpl;
        thread->threadid = i;
        int rc = pthread_create(&thread->thread_t, &thpl->threadAttr, threadWork, (void *)thread);
        if(rc)
        {
            retCode = 1;
            #ifdef CN_DEBUG_MODE
            cn_log("pthread_create threadpool make error code %d", rc);
            #endif // CN_DEBUG_MODE
        }
        else
        {
            #ifdef CN_DEBUG_MODE
            cn_log("pthread_create called for threadid %d ptr %d\n", thread->threadid, (int)thread);
            #endif // CN_DEBUG_MODE
        }
        cn_listAppend(thpl->threads, thread);
    }
    return retCode;
}

// Increase the number of thread from a threadpool by incCnt, wait until increment done.
int cn_thplIncWorker(cn_threadpool *thpl, int incCnt)
{
    struct ThplIncWorkerTrueWorkArgs *args = malloc(sizeof(struct ThplIncWorkerTrueWorkArgs));
    if(args == NULL){return -1;}
    args->thpl = thpl;
    args->incCnt = incCnt;
    return ThplIncWorkerTrueWork(args);
}

// Increase the number of thread from a threadpool by incCnt, wait until increment done.
int cn_thplIncWorkerAsync(cn_threadpool *thpl, int incCnt, cn_action *callback)
{
    struct ThplIncWorkerTrueWorkArgs *args = malloc(sizeof(struct ThplIncWorkerTrueWorkArgs));
    if(args == NULL){return -1;}
    args->thpl = thpl;
    args->incCnt = incCnt;
    struct asyncThreadArgs *args2 = malloc(sizeof(struct asyncThreadArgs));
    if(args2 == NULL){return -1;}
    args2->syncTask = ThplIncWorkerTrueWork;
    args2->args = args;
    args2->callback = callback;
    pthread_create(&args2->tid, NULL, asyncThread, (void *)args2);
    return 0;
}

// argument warper for ThplDecWorkerTrueWork
struct ThplDecWorkerTrueWorkArgs
{
    cn_threadpool *thpl;
    int decCnt;
};

// The core of every threadpool worker decrement.
int ThplDecWorkerTrueWork(void *args)
{
    //Parameter proccessing
    struct ThplDecWorkerTrueWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    int decCnt = targs->decCnt;
    free(args);

    #ifdef CN_DEBUG_MODE
    int debug = 1;
    cn_log("cnthread DEBUG cn_thplDecWorker arg thpl %d decCnt %d phase %d\n", (int)thpl, decCnt, debug++);
    #endif // CN_DEBUG_MODE
    if(thpl == NULL || decCnt < 0){return -1;}
    //Make list of thread that's not doing job to be terminated.
    cn_list *threadRunning = cn_makeList(sizeof(cn_thread), decCnt, true);
    cn_list *threadIddling = cn_makeList(sizeof(cn_thread), decCnt, true);
    int i;
    pthread_mutex_lock(&thpl->key);
    cn_thread **threads = cn_listGet(thpl->threads, 0);
    for(i = 0; i < thpl->threads->cnt; i++)
    {
        if(!threads[i]->gotThplWork)
        {
            cn_listAppend(threadIddling, threads[i]);
            threads[i]->running = false;
            if(threadIddling->cnt >= decCnt){break;}
        }
        else
        {
            cn_listAppend(threadRunning, threads[i]);
        }
    }
    if(decCnt > threadIddling->cnt)
    {
        cn_thread **threadRunningB = (cn_thread **)threadRunning->b;
        int leftToTerminate = decCnt - threadIddling->cnt;
        if(leftToTerminate > threadRunning->cnt){leftToTerminate = threadRunning->cnt;}
        for(i = 0; i < leftToTerminate; i++)
        {
            threadRunningB[i]->running = false;
        }
    }
    #ifdef CN_DEBUG_MODE
    cn_log("cnthread DEBUG cn_thplDecWorker finised set worker running false phase %d\n", debug++);
    #endif // CN_DEBUG_MODE
    pthread_mutex_unlock(&thpl->key);
    bool *keepFlushing = malloc(sizeof(bool));
    if(keepFlushing == NULL){return -1;}
    *keepFlushing = true;
    cn_doDelayedAction(cn_makeAction(EndTryAttempt, keepFlushing, NULL), 3000);
    int nextThreadsCount = thpl->threads->cnt - decCnt;
    if(nextThreadsCount < 0){nextThreadsCount = 0;}
    int tryAttempt = 0;
    while(*keepFlushing == true)
    {
        tryAttempt++;
        #ifdef CN_DEBUG_MODE
        cn_log("cnthread DEBUG cn_thplDecWorker begining flush at try attempt %d\n", tryAttempt);
        #endif // CN_DEBUG_MODE
        for(i = 0; i < (thpl->threads->cnt * 5); i++)
        {
            pthread_mutex_lock(&thpl->key);
            pthread_cond_signal(&thpl->cond);
            pthread_mutex_unlock(&thpl->key);
        }
        cn_sleep(20);
        if(nextThreadsCount >= thpl->threads->cnt)
        {
            #ifdef CN_DEBUG_MODE
            cn_log("cnthread DEBUG cn_thplDecWorker complete flush at try attempt %d\n", tryAttempt);
            #endif // CN_DEBUG_MODE
            break;
        }
    }
    #ifdef CN_DEBUG_MODE
    cn_log("cnthread DEBUG cn_thplDecWorker phase %d\n", debug++);
    #endif // CN_DEBUG_MODE
    cn_desList(threadRunning);
    cn_desList(threadIddling);
    #ifdef CN_DEBUG_MODE
    cn_log("cnthread DEBUG cn_thplDecWorker phase %d\n", debug++);
    #endif // CN_DEBUG_MODE
    return 0;
}

// Decrease the number of thread from a threadpool by decCnt, wait until decrement done.
int cn_thplDecWorker(cn_threadpool *thpl, int decCnt)
{
    struct ThplDecWorkerTrueWorkArgs *args = malloc(sizeof(struct ThplDecWorkerTrueWorkArgs));
    if(args == NULL){return -1;}
    args->thpl = thpl;
    args->decCnt = decCnt;
    return ThplDecWorkerTrueWork(args);
}

// Decrease the number of thread from a threapool by decCnt Asynchoronously
// then execute callback when it complete.
int cn_thplDecWorkerAsync(cn_threadpool *thpl, int decCnt, cn_action *callback)
{
    if(thpl == NULL || decCnt < 1){return -1;}
    struct ThplDecWorkerTrueWorkArgs *decWorkerArgs = malloc(sizeof(struct ThplDecWorkerTrueWorkArgs));
    if(decWorkerArgs == NULL){return -1;}
    decWorkerArgs->thpl = thpl;
    decWorkerArgs->decCnt = decCnt;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = ThplDecWorkerTrueWork;
    args->args = decWorkerArgs;
    args->callback = callback;
    return pthread_create(&args->tid, NULL, asyncThread, (void*)args);
}


struct desThplTrueWorkArgs
{
    cn_threadpool *thpl;
    cn_syncFuncPTR itemDestructor;
};

int desThplTrueWork(void *args)
{
    struct desThplTrueWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    cn_syncFuncPTR itemDestructor = targs->itemDestructor;
    free(args);
    #ifdef CN_DEBUG_MODE
    cn_log("desThplTrueWprl started for %d with worker count %d.\n", (int)thpl, thpl->threads->cnt);
    #endif // CN_DEBUG_MODE
    int n = -1;
    n = cn_thplCancelAll(thpl, itemDestructor);
    if(n != 0){return n;}
    n = cn_thplDecWorker(thpl, thpl->threads->cnt);
    #ifdef CN_DEBUG_MODE
    cn_log("desThplTrueWprl after decwork.\n");
    #endif // CN_DEBUG_MODE
    if(n != 0){return n;}
    #ifdef CN_DEBUG_MODE
    cn_log("desThplTrueWprl before deslist.\n");
    #endif // CN_DEBUG_MODE
    n = cn_desList(thpl->threads);
    if(n != 0){return n;}
    #ifdef CN_DEBUG_MODE
    cn_log("desThplTrueWprl after deslist.\n");
    #endif // CN_DEBUG_MODE
    n = cn_desQue(thpl->actions, itemDestructor);
    if(n != 0){return n;}
    pthread_mutex_destroy(&thpl->key);
    pthread_mutex_destroy(&thpl->terminatorKey);
    pthread_cond_destroy(&thpl->cond);
    #ifdef CN_DEBUG_MODE
    cn_log("desThplTrueWprl finished for %d with worker count %d. GOODBYE.\n", (int)thpl, thpl->threads->cnt);
    #endif // CN_DEBUG_MODE
    free(thpl);

    return 0;
}

int cn_desThpl(cn_threadpool *thpl, cn_syncFuncPTR itemDestructor)
{
    struct desThplTrueWorkArgs *targs = malloc(sizeof(struct desThplTrueWorkArgs));
    targs->thpl = thpl;
    targs->itemDestructor = itemDestructor;
    return desThplTrueWork(targs);
}

int cn_desThplAsync(cn_threadpool *thpl, cn_syncFuncPTR itemDestructor, cn_action *callback)
{
    struct desThplTrueWorkArgs *targs = malloc(sizeof(struct desThplTrueWorkArgs));
    if(targs == NULL){return -1;}
    targs->thpl = thpl;
    targs->itemDestructor = itemDestructor;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = desThplTrueWork;
    args->args = targs;
    args->callback = callback;
    return pthread_create(&args->tid, NULL, asyncThread, (void *)args);
}




