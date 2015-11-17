#include "cornet/cnsched.h"

static bool running = false, starter = false;
static pthread_t schedTid;
static pthread_mutex_t starterKey, schedsKey;
static pthread_cond_t starterCond, schedsCond;
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

struct doDelayedActionArgs
{
    cn_action *action;
    int miliseconds;
    pthread_t tid;
};

void *doDelayedAction(void *args)
{
    #ifdef CN_DEBUG_MODE
    cn_log("do delayed action started for %d\n", (int)args);
    #endif // CN_DEBUG_MODE
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


int cn_doDelayedAction(cn_action *action, int miliseconds)
{
    struct doDelayedActionArgs *args = malloc(sizeof(struct doDelayedActionArgs));
    args->action = action;
    args->miliseconds = miliseconds;
    #ifdef CN_DEBUG_MODE
    cn_log("do delayed action invoked %d\n", (int)args);
    #endif // CN_DEBUG_MODE
    return pthread_create(&args->tid, NULL, doDelayedAction, (void *)args);
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
    pthread_cond_init(&schedsCond, NULL);
    int i;
    while(running)
    {
        pthread_mutex_lock(&schedsKey);
        cn_sched **schedsPtrs = (cn_sched **)*scheds->b;
        cn_sched *sched;
        struct timespec now;
        for(i = 0; i < scheds->cnt; i++)
        {
            sched = schedsPtrs[i];
            clock_gettime(CLOCK_MONOTONIC, &now);
            if(sched->nextExecution.tv_sec >= now.tv_sec && sched->nextExecution.tv_nsec >= now.tv_nsec)
            {
                cn_queEn(dueTime, sched);
                pthread_cond_signal(&schedsCond);
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
    pthread_mutex_init(&starterKey, NULL);
    pthread_cond_init(&starterCond, NULL);
    pthread_mutex_lock(&starterKey);
    pthread_create(&schedTid, NULL, schedulerTask, NULL);
    pthread_cond_wait(&starterCond, &starterKey);
    pthread_mutex_unlock(&starterKey);
    if(!running)
    {
        return -1;
    }
    return 0;
}
