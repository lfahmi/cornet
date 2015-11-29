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
    free(arguments);
    return NULL;
}


struct EndTryAttemptArgs
{
    bool cancel;
    bool *trigg;
};

// This will be executed on new thread to end some try attempt
// by setting the arg to false and free it.
static void *EndTryAttempt(void *args)
{
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker end flushing start phase %d\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    struct EndTryAttemptArgs *targs = args;
    if(targs->cancel == false)
    {
        *targs->trigg = false;
    }
    free(targs);
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker end flushing end phase %d\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    return NULL;
}


// threadpool worker thread function;
static void *threadWork(void *arg)
{
    cn_thread *thread = arg;
    pthread_mutex_t *key = &thread->parent->key;
    pthread_cond_t *cond = &thread->parent->cond;
    cn_queue *actions = thread->parent->actions;
    bool *gotThplWork = &thread->gotThplWork;
    thread->running = true;
    bool *running = &thread->running;
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("worker thread %d created\n", (int)thread->threadid);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    while(*running == true)
    {
        #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
        cn_log("worker thread %d before lock\n", (int)thread->threadid);
        #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
        pthread_mutex_lock(key);
        //int www = actions->cnt;
        #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
        cn_log("worker thread %d after lock\n", (int)thread->threadid);
        #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
        if(actions->cnt == 0)
        {
            #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
            cn_log("worker thread %d entering wait\n", (int)thread->threadid);
            #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
            pthread_cond_wait(cond, key);
        }
        #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
        cn_log("worker thread %d got signal\n", (int)thread->threadid);
        #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
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
                #ifdef CN_DEBUG_MODE_CNTHREAD_H
                cn_log("thread %d got work\n", (int)thread->threadid);
                #endif // CN_DEBUG_MODE_CNTHREAD_H
                work->funcptr(work->args);
            }
            cn_desAction(work);
        }
        else
        {
            #ifdef CN_DEBUG_MODE_CNTHREAD_H
            cn_log("worker thread %d got false signal with no work inside\n", (int)thread->threadid);
            #endif // CN_DEBUG_MODE_CNTHREAD_H
            cn_sleep(1);
        }
        *gotThplWork = false;
    }
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 10
    cn_log("worker thread %d got termination sign\n", (int)thread->threadid);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    //pthread_mutex_lock(&thread->parent->terminatorKey);
    cn_list *threads = thread->parent->threads;
    int myIndx = cn_listIndexOf(threads, thread, 0);
    //cn_listRemoveAt(threads, myIndx);
    cn_listRemove(threads, thread, 1, NULL);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("worker thread %d removing index %d from thread list %d, GOODBYE\n", (int)thread->threadid, myIndx, threads->cnt);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    //pthread_mutex_unlock(&thread->parent->terminatorKey);
    free(thread);
    return NULL;
}

// Initialize new threadpool
cn_threadpool *cn_makeThpl(const char *refname, uint32_t threadCnt)
{
    cn_threadpool *result = malloc(sizeof(cn_threadpool));
    if(result == NULL){return NULL;}
    result->threads = cn_makeList("cn_threadpool.threads", sizeof(cn_thread *), (threadCnt * 2), true);
    if(result->threads == NULL){free(result); return NULL;}
    result->refname = refname;
    pthread_mutex_init(&result->key, NULL);
    pthread_mutex_init(&result->terminatorKey, NULL);
    pthread_cond_init(&result->cond, NULL);
    pthread_attr_init(&result->threadAttr);
    result->actions = cn_makeQue("cn_threadpool.jobs", sizeof(cn_action *));
    if(result->actions == NULL)
    {
        goto reterr;
    }
    if(cn_thplIncWorker(result, threadCnt) != 0){goto reterr;}
    return result;

    reterr:
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("threadpool creation error %s\n", refname);
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    cn_desList(result->threads);
    pthread_mutex_destroy(&result->key);
    pthread_mutex_destroy(&result->terminatorKey);
    pthread_cond_destroy(&result->cond);
    pthread_attr_destroy(&result->threadAttr);
    return NULL;
}

// Add new job for threadpool
int cn_thplEnq(cn_threadpool *thpl, cn_action *action)
{
    pthread_mutex_lock(&thpl->key);
    if(cn_queEn(thpl->actions, action) == 0){pthread_cond_signal(&thpl->cond);}
    else {pthread_mutex_unlock(&thpl->key); return -1;}
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("threadpool enq success\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    pthread_mutex_unlock(&thpl->key);
    return 0;
}

int cn_thplCancelAll(cn_threadpool *thpl)
{
    pthread_mutex_lock(&thpl->key);
    cn_queEmpty(thpl->actions, cn_desActionNumerableInterface);
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
static int ThplIncWorkerTrueWork(void *args)
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
            #ifdef CN_DEBUG_MODE_CNTHREAD_H
            cn_log("pthread_create threadpool make error code %d", rc);
            #endif // CN_DEBUG_MODE_CNTHREAD_H
        }
        else
        {
            #ifdef CN_DEBUG_MODE_CNTHREAD_H
            cn_log("pthread_create called for threadid %d ptr %d\n", thread->threadid, (int)thread);
            #endif // CN_DEBUG_MODE_CNTHREAD_H
        }
        cn_listAppend(thpl->threads, thread);
        #ifdef CN_DEBUG_MODE_CNTHREAD_H
        cn_log("pthread append called for threadid %d ptr %d\n", thread->threadid, (int)thread);
        #endif // CN_DEBUG_MODE_CNTHREAD_H
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
    struct ThplIncWorkerTrueWorkArgs *incWorkArgs = malloc(sizeof(struct ThplIncWorkerTrueWorkArgs));
    if(incWorkArgs == NULL){return -1;}
    incWorkArgs->thpl = thpl;
    incWorkArgs->incCnt = incCnt;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = ThplIncWorkerTrueWork;
    args->args = incWorkArgs;
    args->callback = callback;
    return pthread_create(&args->tid, NULL, asyncThread, (void *)args);
}

// argument warper for ThplDecWorkerTrueWork
struct ThplDecWorkerTrueWorkArgs
{
    cn_threadpool *thpl;
    int decCnt;
};

// The core of every threadpool worker decrement.
static int ThplDecWorkerTrueWork(void *args)
{
    //Parameter proccessing
    struct ThplDecWorkerTrueWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    int decCnt = targs->decCnt;
    free(args);

    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    int debug = 1;
    cn_log("cnthread DEBUG cn_thplDecWorker arg thpl %d decCnt %d phase %d\n", (int)thpl, decCnt, debug++);
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    if(thpl == NULL || decCnt < 0){return -1;}
    //Make list of thread that's not doing job to be terminated.
    cn_list *threadRunning = cn_makeList("cn_thplDecWorker.threadRunning", sizeof(cn_thread), decCnt, true);
    cn_list *threadIddling = cn_makeList("cn_thplDecWorker.threadIddling", sizeof(cn_thread), decCnt, true);
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
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker finised set worker running false phase %d\n", debug++);
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    pthread_mutex_unlock(&thpl->key);
    bool *keepFlushing = malloc(sizeof(bool));
    if(keepFlushing == NULL){return -1;}
    *keepFlushing = true;
    struct EndTryAttemptArgs *endTryArgs = malloc(sizeof(struct EndTryAttemptArgs));
    endTryArgs->trigg = keepFlushing;
    endTryArgs->cancel = false;
    cn_doDelayedAction(cn_makeAction("cn_thplDecWorker.stopFlusing", EndTryAttempt, endTryArgs, NULL), 30);
    int nextThreadsCount = thpl->threads->cnt - decCnt;
    if(nextThreadsCount < 0){nextThreadsCount = 0;}
    int tryAttempt = 0;
    while(*keepFlushing == true)
    {
        tryAttempt++;
        #ifdef CN_DEBUG_MODE_CNTHREAD_H
        cn_log("keep flush %d cnthread DEBUG cn_thplDecWorker begining flush at try attempt %d, cnt %d target %d\n", *keepFlushing, tryAttempt, thpl->threads->cnt, nextThreadsCount);
        #endif // CN_DEBUG_MODE_CNTHREAD_H
        pthread_mutex_lock(&thpl->key);
        for(i = 0; i < (thpl->threads->cnt * 5); i++)
        {
            pthread_cond_signal(&thpl->cond);
        }
        pthread_mutex_unlock(&thpl->key);
        cn_sleep(20);
        if(nextThreadsCount >= thpl->threads->cnt)
        {
            #ifdef CN_DEBUG_MODE_CNTHREAD_H
            cn_log("cnthread DEBUG cn_thplDecWorker complete flush at try attempt %d\n", tryAttempt);
            #endif // CN_DEBUG_MODE_CNTHREAD_H
            break;
        }
    }
    if(*keepFlushing == true){endTryArgs->cancel = true;}
    free(keepFlushing);
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker phase %d\n", debug++);
    #endif // CN_DEBUG_MODE_CNTHREAD_H
    cn_desList(threadRunning);
    cn_desList(threadIddling);
    #ifdef CN_DEBUG_MODE_CNTHREAD_H
    cn_log("cnthread DEBUG cn_thplDecWorker phase %d\n", debug++);
    #endif // CN_DEBUG_MODE_CNTHREAD_H
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
    return pthread_create(&args->tid, NULL, asyncThread, (void *)args);
}


struct desThplTrueWorkArgs
{
    cn_threadpool *thpl;
    cn_syncFuncPTR itemDestructor;
};

static int desThplTrueWork(void *args)
{
    struct desThplTrueWorkArgs *targs = args;
    cn_threadpool *thpl = targs->thpl;
    cn_syncFuncPTR itemDestructor = targs->itemDestructor;
    free(args);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("desThplTrueWprl started for %d with worker count %d.\n", (int)thpl, thpl->threads->cnt);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    int n = -1;
    n = cn_thplCancelAll(thpl);
    if(n != 0){return n;}
    n = cn_thplDecWorker(thpl, thpl->threads->cnt);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("desThplTrueWprl after decwork.\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    if(n != 0){return n;}
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("desThplTrueWprl before deslist.\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    n = cn_desList(thpl->threads);
    if(n != 0){return n;}
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 5
    cn_log("desThplTrueWprl after deslist.\n");
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    n = cn_desQue(thpl->actions, itemDestructor);
    if(n != 0){return n;}
    pthread_mutex_destroy(&thpl->key);
    pthread_mutex_destroy(&thpl->terminatorKey);
    pthread_cond_destroy(&thpl->cond);
    #if CN_DEBUG_MODE_CNTHREAD_H_LVL >= 1
    cn_log("desThplTrueWprl finished for %s with worker count %d. GOODBYE.\n", thpl->refname, thpl->threads->cnt);
    #endif // CN_DEBUG_MODE_CNTHREAD_H_LVL
    free(thpl);

    return 0;
}

int cn_desThpl(cn_threadpool *thpl)
{
    struct desThplTrueWorkArgs *targs = malloc(sizeof(struct desThplTrueWorkArgs));
    targs->thpl = thpl;
    targs->itemDestructor = cn_desActionNumerableInterface;
    return desThplTrueWork(targs);
}

int cn_desThplAsync(cn_threadpool *thpl, cn_action *callback)
{
    struct desThplTrueWorkArgs *targs = malloc(sizeof(struct desThplTrueWorkArgs));
    if(targs == NULL){return -1;}
    targs->thpl = thpl;
    targs->itemDestructor = cn_desActionNumerableInterface;
    struct asyncThreadArgs *args = malloc(sizeof(struct asyncThreadArgs));
    if(args == NULL){return -1;}
    args->syncTask = desThplTrueWork;
    args->args = targs;
    args->callback = callback;
    return pthread_create(&args->tid, NULL, asyncThread, (void *)args);
}




