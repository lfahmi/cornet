#include "cornet/cnsched.h"

int cn_timespecAdd(struct timespec *dest, struct timespec *addition)
{
    if(dest == NULL || addition == NULL){return -1;}
    dest->tv_sec += addition->tv_sec;
    dest->tv_nsec += addition->tv_nsec;
    if (dest->tv_nsec >= CN_SEC_IN_NSEC)
    {
        dest->tv_nsec -= CN_SEC_IN_NSEC;
        dest->tv_sec++;
    }
    return 0;
}

int cn_timespecComp(struct timespec *tsA, struct timespec *tsB)
{
    if (tsA->tv_sec < tsB->tv_sec)
        return (-1) ; // Less than.
    else if (tsA->tv_sec > tsB->tv_sec)
        return (1) ; // Greater than.
    else if (tsA->tv_nsec < tsB->tv_nsec)
        return (-1) ; // Less than.
    else if (tsA->tv_nsec > tsB->tv_nsec)
        return (1) ; // Greater than.
    else
        return (0) ; // Equal.
}

static bool running = false, starter = false;
static pthread_t schedTid, workTid;
static pthread_mutex_t starterKey, schedsKey, dueTimeKey;
static pthread_cond_t starterCond, dueTimeCond;
static cn_list *scheds;
static cn_queue *dueTime;

void cn_sleep(int milliseconds)
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}

/*
struct doDelayedActionArgs
{
    cn_action *action;
    int miliseconds;
    pthread_t tid;
};

void *doDelayedAction(void *args)
{
    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
    cn_log("do delayed action started for %d\n", (int)args);
    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
    struct doDelayedActionArgs *targs = args;
    if(targs->action == NULL){return 0;}
    if(targs->miliseconds > 0)
    {
        cn_sleep(targs->miliseconds);
    }
    targs->action->funcptr(targs->action->args);
    cn_desAction(targs->action);
    free(args);
    return 0;
}
*/

int cn_doDelayedAction(cn_action *action, int CN_100_MILISECONDS timeout)
{
    /*
    struct doDelayedActionArgs *args = malloc(sizeof(struct doDelayedActionArgs));
    args->action = action;
    args->miliseconds = miliseconds;
    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
    cn_log("do delayed action invoked %d\n", (int)args);
    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
    return pthread_create(&args->tid, NULL, doDelayedAction, (void *)args);
    */
    return cn_schedAdd(cn_makeSched(action, timeout, 0));
}

void *workerTask(void *args)
{
    pthread_mutex_t *key = &dueTimeKey;
    pthread_cond_t *cond = &dueTimeCond;
    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
    cn_log("worker thread %d created\n", (int)workTid);
    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
    while(running == true)
    {
        pthread_mutex_lock(key);
        if(dueTime->cnt == 0)
        {
            #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
            cn_log("worker thread %d entering wait\n", (int)workTid);
            #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
            pthread_cond_wait(cond, key);
        }
        #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
        cn_log("worker thread %d got signal\n", (int)workTid);
        #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
        if(!running)
        {
            pthread_mutex_unlock(key);
            break;
        }
        cn_sched *work = cn_queDe(dueTime);
        pthread_mutex_unlock(key);
        if(work != NULL)
        {
            if(work->action != NULL && !work->action->cancel && work->action->funcptr != NULL)
            {
                #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
                cn_log("thread %d got work\n", (int)workTid);
                #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
                work->action->funcptr(work->action->args);
            }
            if(work->interval.tv_sec == 0 && work->interval.tv_nsec == 0)
            {
                cn_schedRemove(work);
                cn_desSched(work);
            }
            else
            {
                cn_timespecAdd(&work->nextExecution, &work->interval);
            }
        }
        else
        {
            #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
            cn_log("worker thread %d got false signal with no work inside\n", (int)workTid);
            #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
            cn_sleep(5);
        }
    }
    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
    cn_log("worker thread %d got termination sign\n", (int)workTid);
    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
    return NULL;
}

void *schedulerTask(void *args)
{
    pthread_mutex_lock(&starterKey);
    scheds = cn_makeList(sizeof(cn_sched), 100, true);
    if(scheds == NULL){goto doErr;}
    dueTime = cn_makeQue(sizeof(cn_action));
    if(dueTime == NULL){cn_desList(scheds); goto doErr;}
    running = true;
    pthread_cond_signal(&starterCond);
    pthread_mutex_unlock(&starterKey);
    pthread_mutex_init(&schedsKey, NULL);
    pthread_mutex_init(&dueTimeKey, NULL);
    pthread_cond_init(&dueTimeCond, NULL);
    pthread_create(&workTid, NULL, workerTask, NULL);
    int i;
    while(running)
    {
        pthread_mutex_lock(&schedsKey);
        cn_sched **schedsPtrs = (cn_sched **)*scheds->b;
        cn_sched *sched;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        for(i = 0; i < scheds->cnt; i++)
        {
            sched = schedsPtrs[i];
            if(sched->lastExecution.tv_sec != sched->nextExecution.tv_sec || sched->lastExecution.tv_nsec != sched->nextExecution.tv_nsec)
            {
                if(cn_timespecComp(&now, &sched->nextExecution) >= 0)
                {
                    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
                    cn_log("scheduler due time %lu:%lu for %d before QUEUEEN cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
                    if(cn_queEn(dueTime, sched) != 0)
                    {
                        #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
                        cn_log("scheduler due time %lu:%lu for %d dueTime failed QUEUEEN cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                        #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
                    }
                    sched->lastExecution = sched->nextExecution;
                    pthread_mutex_lock(&dueTimeKey);
                    pthread_cond_signal(&dueTimeCond);
                    pthread_mutex_unlock(&dueTimeKey);
                    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
                    cn_log("scheduler due time %lu:%lu for %d dueTime cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
                }
                else
                {
                    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
                    cn_log("scheduler no due time %lu:%lu for %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched);
                    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
                }

            }
            else
            {
                pthread_mutex_lock(&dueTimeKey);
                pthread_cond_signal(&dueTimeCond);
                pthread_mutex_unlock(&dueTimeKey);
                #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
                cn_log("scheduler not done worked. triggering! time %lu:%lu for %d dueTIme CNT = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
            }
        }
        pthread_mutex_unlock(&schedsKey);
        cn_sleep(100);
    }
    // The code would never actualy reach here because the scheduler wouldn't stop until app stop
    cn_desQue(dueTime, NULL);
    cn_listEmpty(scheds, cn_desSchedNumerableInterface);
    cn_desList(scheds);
    doErr:
    running = false;
    pthread_cond_signal(&starterCond);
    pthread_mutex_unlock(&starterKey);
    return NULL;
}

int startScheduler()
{
    if(starter){return 0;}
    starter = true;
    pthread_mutex_init(&starterKey, NULL);
    pthread_cond_init(&starterCond, NULL);
    pthread_mutex_lock(&starterKey);
    pthread_create(&schedTid, NULL, schedulerTask, NULL);
    pthread_cond_wait(&starterCond, &starterKey);
    pthread_mutex_unlock(&starterKey);
    if(!running)
    {
        starter = false;
        return -1;
    }
    return 0;
}

cn_sched *cn_makeSched(cn_action *action, int CN_100_MILISECONDS timeout, int CN_100_MILISECONDS interval)
{
    cn_sched *sched = malloc(sizeof(cn_sched));
    sched->action = action;
    struct timespec tpnow;
    sched->interval.tv_nsec = (interval % 10) * 100000000;
    sched->interval.tv_sec = interval / 10;
    clock_gettime(CLOCK_MONOTONIC, &tpnow);
    sched->nextExecution = tpnow;
    /*
    sched->nextExecution.tv_sec = (timeout / 10) + tpnow.tv_sec;
    sched->nextExecution.tv_nsec = ((timeout % 10) * 100000000) + tpnow.tv_nsec;
    if(interval > 0)
    {
        sched->interval.tv_sec = interval / 10;
        sched->interval.tv_nsec = (interval % 10) * 100000000;

    }
    */
    #if CN_DEBUG_MODE_CNSCHED_H_DEEP == 1
    cn_log("cn_makeSched now %lu:%lu timeout %lu:%lu interval %lu:%lu\n", tpnow.tv_sec, tpnow.tv_nsec,
        sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched->interval.tv_sec, sched->interval.tv_nsec);
    #endif // CN_DEBUG_MODE_CNSCHED_H_DEEP
    return sched;
}

int cn_desSched(cn_sched *sched)
{
    int n;
    n = cn_desAction(sched->action);
    if(n != 0){return n;}
    free(sched);
    return n;
}

int cn_desSchedNumerableInterface(void *args)
{
    return cn_desSched(args);
}

int cn_schedAdd(cn_sched *sched)
{
    int n;
    if(!running)
    {
        n = startScheduler();
        if(n != 0){return n;}
    }
    pthread_mutex_lock(&schedsKey);
    n = cn_listAppend(scheds, sched);
    pthread_mutex_unlock(&schedsKey);
    return n;
}

int cn_schedRemove(cn_sched *sched)
{
    int n;
    pthread_mutex_lock(&schedsKey);
    n = cn_listRemoveAt(scheds, cn_listIndexOf(scheds, sched, 0));
    pthread_mutex_unlock(&schedsKey);
    return n;
}
