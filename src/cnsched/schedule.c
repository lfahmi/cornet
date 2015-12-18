#include "cornet/cnsched.h"

/** cn_sched type id */
cn_type_id_t cn_sched_type_id = 0;

#if CN_DEBUG_MODE_CNSCHED_H_LVL > 0
#define CN_DEBUG_TYPENAME "cn_sched"
#endif // CN_DEBUG_MODE_CNSCHED_H_LVL

/**
    * Add timespec dest by timespec addition
    * @param dest the timespec that will be increased
    * @param addition the timespec value to be added
    */
int cn_timespecAdd(struct timespec *dest, struct timespec *addition)
{
    // Checking parameter.
    if(dest == NULL || addition == NULL){return -1;}

    // adding timespec sec and nsec by addition sec and nsec.
    dest->tv_sec += addition->tv_sec;
    dest->tv_nsec += addition->tv_nsec;

    // if nsec value is more than a second.
    if (dest->tv_nsec >= CN_SEC_IN_NSEC)
    {
        // subtract dest nsec by a second.
        dest->tv_nsec -= CN_SEC_IN_NSEC;

        // add dest sec by one.
        dest->tv_sec++;
    }

    // return fine completion.
    return 0;
}

/**
    * Compare tsA by tsB, return value int represent tsA is greater, less, or equal  than tsB
    * @param tsA comparison object.
    * @param tsB comparison object.
    */
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

/// Indicate scheduler running status.
static bool cn_schedWorkerRunning = false, cn_schedTaskRunning = false;

/// Indicate if the scheduler is starting.
static bool starter = false;

/// store thread id for scheduler and scheduler worker.
static pthread_t schedTid, workTid;

/// PREVENT VALGRIND POSIBLE LEAK WARNING
static pthread_attr_t schedThreadAttr;

/// mutex lock for starter, scheduler and worker.
static pthread_mutex_t starterKey, schedsKey, dueTimeKey;

/// condition for starter and worker.
static pthread_cond_t starterCond, dueTimeCond;

/// number of execution waiting scheduler start
static int waitingStarted = 0;


/// schedule items list. list of cn_sched.
static cn_list *scheds;

/// worker work items. all schedule that reach due time. queue of cn_sched.
static cn_queue *dueTime;

/**
    * release control and wait for milisecond then use control again.
    * @param miliseconds : wait time before using up control again.
    */
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

/**
    * Execute action after timeout.
    * @param action : the action to do.
    * @param timeout : the wait time before doing action.
    */
int cn_doDelayedAction(cn_action *action, int CN_100_MILISECONDS timeout)
{
    // Make new action then add it to scheduler.
    // interval is set to zero so scheduler will remove the schedule after finish action.
    return cn_schedAdd(cn_makeSched("cn_sched.doDelayedAction.sched", action, timeout, 0));
}

/**
    * ThreadStart task for scheduler worker
    * @param args NULL.
    */
static void *workerTask(void *args)
{
    // interfacing object for operation
    pthread_mutex_t *key = &dueTimeKey;
    pthread_cond_t *cond = &dueTimeCond;
    #if CN_DEBUG_MODE_CNSCHED_H_LVL <= 2
    cn_log("worker thread %u created\n", (int)workTid);
    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
    while(cn_schedWorkerRunning == true)
    {
        // lock cn_queue dueTime access
        pthread_mutex_lock(key);

        // if cn_queue dueTime item count is zero then wait new item signal.
        if(dueTime->cnt == 0)
        {
            #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
            cn_log("worker thread %u entering wait\n", (int)workTid);
            #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
            pthread_cond_wait(cond, key);
        }
        #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
        cn_log("worker thread %u got signal\n", (int)workTid);
        #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

        // thread abortion, break work requesting loop and terminate thread.
        if(cn_schedWorkerRunning == false)
        {
            pthread_mutex_unlock(key);
            break;
        }

        // Take work item then unlock cn_queue dueTime
        cn_sched *work = cn_queDe(dueTime);
        pthread_mutex_unlock(key);
        if(work != NULL)
        {
            // Defensive check then do action
            work->action = cn_typeGet(work->action, cn_action_type_id);
            if(work->action != NULL && !work->action->cancel && work->action->funcptr != NULL)
            {
                #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                cn_log("thread %u got work %s\n", (int)workTid, work->action->refname);
                #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
                work->action->funcptr(work->action->args);
            }
            else
            {
                cn_schedRemove(work);
                cn_desSched(work);
            }

            // schedule was created for delay action. remove from scheduler after first action.
            if(work->interval.tv_sec == 0 && work->interval.tv_nsec == 0)
            {
                cn_schedRemove(work);
                cn_desSched(work);
            }
            // set next execution time by adding it by interval.
            else
            {
                cn_timespecAdd(&work->nextExecution, &work->interval);
            }
        }
        // Seems like we got false signal. let other thread take control by sleeping for 5ms.
        else
        {
            #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
            cn_log("worker thread %d got false signal with no work inside\n", (int)workTid);
            #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
            cn_sleep(5);
        }
    }
    #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
    cn_log("worker thread %d got termination sign\n", (int)workTid);
    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
    return NULL;
}

/**
    * ThreadStart task for scheduler
    * @param args NULL.
    */
static void *schedulerTask(void *args)
{
    // lock starter init schedule list, dueTime queue, mutexs cond and worker thread.
    pthread_mutex_lock(&starterKey);
    scheds = cn_makeList("cn_sched.scheduler.schedules", sizeof(cn_sched *), 20, true);
    if(scheds == NULL){goto doErr;}
    dueTime = cn_makeQue("cn_sched.scheduler.dueTime", sizeof(cn_action));
    if(dueTime == NULL){cn_desList(scheds); goto doErr;}
    cn_schedTaskRunning = true;
    cn_schedWorkerRunning = true;
    pthread_mutex_init(&schedsKey, NULL);
    pthread_mutex_init(&dueTimeKey, NULL);
    pthread_cond_init(&dueTimeCond, NULL);
    pthread_create(&workTid, &schedThreadAttr, workerTask, NULL);

    int i;

    // schedule starter complete signal then release starter lock
    pthread_mutex_unlock(&starterKey);
    while(waitingStarted > 0)
    {
        for(i = 0; i < waitingStarted; i++)
        {
            pthread_mutex_lock(&starterKey);
            pthread_cond_signal(&starterCond);
            #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
            cn_log("schedule starter signaled\n");
            #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
            pthread_mutex_unlock(&starterKey);
        }
        cn_sleep(1);
    }
    starter = false;

    // scheduler main task, live until proccess die.
    while(cn_schedTaskRunning)
    {
        // lock schedule list.
        pthread_mutex_lock(&schedsKey);

        if(scheds->cnt > 0)
        {

            // preparation for foreach item in schedules list.
            cn_sched **schedsPtrs = (cn_sched **)*scheds->b;
            cn_sched *sched;

            // get now time
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);

            // foreach schedule
            for(i = 0; i < scheds->cnt; i++)
            {
                #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                cn_log("SCHEDULE LOOP %d = %d cnt %d\n", i, (int)schedsPtrs[i], scheds->cnt);
                #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                // foreach work item
                sched = schedsPtrs[i];

                // next execution time is not last execution time
                if(sched->lastExecution.tv_sec != sched->nextExecution.tv_sec || sched->lastExecution.tv_nsec != sched->nextExecution.tv_nsec)
                {
                    // now time is greater than or equal than next execution time.
                    if(cn_timespecComp(&now, &sched->nextExecution) >= 0)
                    {
                        // lock dueTime
                        pthread_mutex_lock(&dueTimeKey);
                        #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                        cn_log("scheduler due time %lu:%lu for %d before QUEUEEN cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                        #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                        // enQueue sched to dueTime
                        if(cn_queEn(dueTime, sched) == 0)
                        {
                            // preventing double execution. set last execution by old next execution
                            sched->lastExecution = sched->nextExecution;

                            // always put signal in the end of operation
                            pthread_cond_signal(&dueTimeCond);
                        }
                        #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                        else
                        {
                            cn_log("scheduler due time %lu:%lu for %d dueTime failed QUEUEEN cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                        }
                        #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                        // unlock dueTime
                        pthread_mutex_unlock(&dueTimeKey);
                        #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                        cn_log("scheduler due time %lu:%lu for %d dueTime cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                        #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
                    }
                    #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                    else
                    {
                        cn_log("scheduler %s no due time %lu:%lu for %d\n", sched->action->refname, sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched);
                    }
                    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                }
                // seems like this schedule is never get done executed for sometime. let observer know.
                #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                else
                {
                    // lock dueTime
                    pthread_mutex_lock(&dueTimeKey);
                    #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                    cn_log("scheduler due time %lu:%lu for %d before QUEUEEN cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                    // enQueue sched to dueTime
                    if(cn_queEn(dueTime, sched) == 0)
                    {
                        // preventing double execution. set last execution by old next execution
                        sched->lastExecution = sched->nextExecution;

                        // always put signal in the end of operation
                        pthread_cond_signal(&dueTimeCond);
                    }
                    #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                    else
                    {
                        cn_log("scheduler due time %lu:%lu for %d dueTime failed QUEUEEN cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                    }
                    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                    // unlock dueTime
                    pthread_mutex_unlock(&dueTimeKey);
                    #if CN_DEBUG_MODE_CNSCHED_H_LVL == 1
                    cn_log("scheduler due time %lu:%lu for %d dueTime cnt = %d\n", sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

                    cn_log("scheduler %s not done worked. triggering! time %lu:%lu for %d dueTIme CNT = %d\n", sched->action->refname, sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched, dueTime->cnt);
                }
                #endif // CN_DEBUG_MODE_CNSCHED_H_LVL
            }
        }
        pthread_mutex_unlock(&schedsKey);
        cn_sleep(100);
    }
    // The code would never actualy reach here because the scheduler wouldn't stop until app stop
    // Destroy every component used by scheduler.

    cn_schedWorkerRunning = false;

    pthread_mutex_lock(&dueTimeKey);
    pthread_cond_signal(&dueTimeCond);
    pthread_mutex_unlock(&dueTimeKey);

    void **retval = malloc(sizeof(void *));
    pthread_join(workTid, retval);
    free(retval);

    cn_desQue(dueTime, NULL);
    cn_listEmpty(scheds, cn_desSchedNumerableInterface);
    cn_desList(scheds);
    pthread_cond_destroy(&starterCond);
    pthread_cond_destroy(&dueTimeCond);
    pthread_mutex_destroy(&starterKey);
    pthread_mutex_destroy(&schedsKey);
    pthread_mutex_destroy(&dueTimeKey);
    pthread_attr_destroy(&schedThreadAttr);
    cn_log("scheduler stoped!\n");
    return NULL;

    // error on starter
    doErr:
    cn_schedTaskRunning = false;
    pthread_cond_signal(&starterCond);
    pthread_mutex_unlock(&starterKey);
    return NULL;
}

/**
    * Internal Function, Start Scheduler ThreadStart Task and
    * Start Scheduler Worker THreadsStart Task.
    * return 0 fine, otherwise -1.
    */
int cn_startScheduler()
{
    // Defensive code.
    if(!starter)
    {
        // This is the first start attempt, disable incoming start try attempt.
        starter = true;

        // Initialize starter lock and condition.
        pthread_mutex_init(&starterKey, NULL);
        pthread_cond_init(&starterCond, NULL);
        pthread_attr_init(&schedThreadAttr);

        // PREVENT VALGRIND POSIBLE LEAK WARNING
        //pthread_attr_setdetachstate(&schedThreadAttr, PTHREAD_CREATE_DETACHED);

        // go inside starter and lock.
        pthread_mutex_lock(&starterKey);

        // start Scheduler ThreadStart task thread.
        pthread_create(&schedTid, &schedThreadAttr, schedulerTask, NULL);

        // tell scheduler that we are waiting.
        waitingStarted++;

        // wait for change condition signal.
        pthread_cond_wait(&starterCond, &starterKey);

        // we are no longer waiting
        waitingStarted--;

        // go outside starting scheduler wait entry.
        pthread_mutex_unlock(&starterKey);
    }
    else
    {
        // So, there is another start attempt happening
        // go inside waiting entry
        pthread_mutex_lock(&starterKey);

        // we just tell we are also waiting
        waitingStarted++;

        // wait for condition change signal
        pthread_cond_wait(&starterCond, &starterKey);

        // we are no longer waiting
        waitingStarted--;

        // go outside starting scheduler wait entry.
        pthread_mutex_unlock(&starterKey);
    }

    // if the scheduler is still not running then return failure
    if(!cn_schedTaskRunning)
    {
        starter = false;
        return -1;
    }

    // otherwise return fine.
    return 0;
}

int cn_stopScheduler()
{
    cn_schedTaskRunning = false;
    void **retval = malloc(sizeof(void *));
    pthread_join(schedTid, retval);
    free(retval);
    return 0;
}


/**
    * make cn_sched object.
    * @param refname : reference name. (variable name)
    * @param action : action to be executed.
    * @param timeout : wait time before first action execution.
    * @param interval : wait time before next action execution.
    */
cn_sched *cn_makeSched(const char *refname, cn_action *action, int CN_100_MILISECONDS timeout, int CN_100_MILISECONDS interval)
{
    // Defensive code
    action = cn_typeGet(action, cn_action_type_id);
    if(action == NULL){return NULL;}

    // Allocate result, init vars
    cn_sched *sched = malloc(sizeof(cn_sched));
    if(sched == NULL){return NULL;}
    sched->refname = refname;
    sched->action = action;
    struct timespec tpnow;
    struct timespec tptout;

    // convert 100ms interval to timespec
    sched->interval.tv_nsec = (interval % 10) * 100000000;
    sched->interval.tv_sec = interval / 10;

    // convert 100ms timout to timespec
    tptout.tv_nsec = (timeout % 10) * 100000000;
    tptout.tv_sec = timeout / 10;

    // set next execution by adding now time by timout
    clock_gettime(CLOCK_MONOTONIC, &tpnow);
    sched->nextExecution = tpnow;
    cn_timespecAdd(&sched->nextExecution, &tptout);
    #if CN_DEBUG_MODE_CNSCHED_H_LVL >= 10
    cn_log("cn_makeSched now %lu:%lu timeout %lu:%lu interval %lu:%lu\n", tpnow.tv_sec, tpnow.tv_nsec,
        sched->nextExecution.tv_sec, sched->nextExecution.tv_nsec, sched->interval.tv_sec, sched->interval.tv_nsec);
    #endif // CN_DEBUG_MODE_CNSCHED_H_LVL

    // set last execution to zero
    cn_bitzero(&sched->lastExecution, sizeof(struct timespec));

    // if schedule type has no identifier, request identifier.
    if(cn_sched_type_id < 1){cn_sched_type_id = cn_typeGetNewID();}

    // initialize object type definition
    cn_typeInit(&sched->t, cn_sched_type_id);

    // return result
    return sched;
}

/**
    * destruct cn_sched object.
    * @param sched : schedule object to be destructed.
    */
int cn_desSched(cn_sched *sched)
{
    int n = 0;
    // call action destructor.
    n = cn_desAction(sched->action);

    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNSCHED_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d][%s:%s] destroy type next.\n", __FILE__, __func__, __LINE__, sched->refname, CN_DEBUG_TYPENAME);
    #endif // CN_DEBUG_MODE

    // Destroy object type definition
    cn_typeDestroy(&sched->t);

    #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNSCHED_H_LVL > 0
    cn_log("[DEBUG][file:%s][func:%s][line:%d][%s:%s] dealloc attempt next.\n", __FILE__, __func__, __LINE__, sched->refname, CN_DEBUG_TYPENAME);
    #endif // CN_DEBUG_MODE
    free(sched);
    // return fine
    return n;
}

/**
    * cn_sched object destructor interface for numerable type
    * @param args cn_sched object.
    */
int cn_desSchedNumerableInterface(void *args)
{
    return cn_desSched(args);
}

/**
    * add cn_sched object to scheduler
    * @param sched cn_sched object to be added.
    */
int cn_schedAdd(cn_sched *sched)
{
    int n;
    // if scheduler is not running then call starter.
    if(!cn_schedTaskRunning)
    {
        n = cn_startScheduler();
        if(n != 0){return n;}
    }

    // add cn_sched inside scheduler lock
    pthread_mutex_lock(&schedsKey);
    n = cn_listAppend(scheds, sched);
    pthread_mutex_unlock(&schedsKey);

    // return fine
    return n;
}

/**
    * remove cn_sched object from scheduler
    * @param sched cn_sched object to be removed.
    */
int cn_schedRemove(cn_sched *sched)
{
    int n;

    // remove cn_sched object inside scheduler lock.
    pthread_mutex_lock(&schedsKey);
    n = cn_listRemoveAt(scheds, cn_listIndexOf(scheds, sched, 0));
    pthread_mutex_unlock(&schedsKey);

    // return status.
    return n;
}
