#ifndef _CNSCHED_H
#define _CNSCHED_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include "cornet/cntype.h"
#include "cornet/cnnum.h"

#define CN_SEC_IN_NSEC 1000000000

#define CN_100_MILISECONDS

/* Scheduler Section */

extern cn_type_id_t cn_sched_type_id;

/**
    * cn_sched definition, scheduler object type
    */
struct cn_sched
{
    cn_type t;
    const const char *refname;
    cn_action *action;
    struct timespec lastExecution;
    struct timespec nextExecution;
    struct timespec interval;
};

typedef struct cn_sched cn_sched;

#include "cornet/cndebug.h"

#undef CN_DEBUG_MODE_CNSCHED_H_LVL
//#define CN_DEBUG_MODE_CNSCHED_H_LVL 1
#undef CN_DEBUG_MODE_FREE
#define CN_DEBUG_MODE_FREE 1

/**
    * Add timespec dest by timespec addition
    * @param dest the timespec that will be increased
    * @param addition the timespec value to be added
    */
extern int cn_timespecAdd(struct timespec *dest, struct timespec *addition);

/**
    * Compare tsA by tsB, return value int represent tsA is greater, less, or equal  than tsB
    * @param tsA comparison object.
    * @param tsB comparison object.
    */
extern int cn_timespecComp(struct timespec *tsA, struct timespec *tsB);

/*  used by cn_sleep to generate crossplatform compability. */
#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

/**
    * release control and wait for milisecond then use control again.
    * @param miliseconds wait time before using up control again.
    */
extern void cn_sleep(int milisecond);  // cross-platform sleep function

/**
    * Execute action after timeout.
    * @param action the action to do.
    * @param timeout the wait time before doing action.
    */
extern int cn_doDelayedAction(cn_action *action, int CN_100_MILISECONDS timeout);

extern int cn_startScheduler();

extern int cn_stopScheduler();

/**
    * make cn_sched object.
    * @param refname : reference name. (variable name)
    * @param action : action to be executed.
    * @param timeout : wait time before first action execution.
    * @param interval : wait time before next action execution.
    */
extern cn_sched *cn_makeSched(const char *refname, cn_action *action, int CN_100_MILISECONDS timeout, int CN_100_MILISECONDS interval);

/**
    * destruct cn_sched object.
    * @param sched : schedule object to be destructed.
    */
extern int cn_desSched(cn_sched *sched);

/**
    * cn_sched object destructor interface for numerable type
    * @param args cn_sched object.
    */
extern int cn_desSchedNumerableInterface(void *args);

/**
    * add cn_sched object to scheduler
    * @param sched cn_sched object to be added.
    */
extern int cn_schedAdd(cn_sched *sched);

/**
    * remove cn_sched object from scheduler
    * @param sched cn_sched object to be removed.
    */
extern int cn_schedRemove(cn_sched *sched);

#endif
