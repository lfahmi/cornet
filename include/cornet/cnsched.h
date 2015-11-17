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

/* Scheduler Section */

/*  cn_sched type, scheduler object type. */
typedef struct
{
    cn_action action;
    struct timespec nextExecution, interval;
} cn_sched;

/*  give control to other thread, wait for miliseconds then do action */
#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif
extern void cn_sleep(int milisecond);  // cross-platform sleep function

/*  give control to other threads wait for miliseconds
    then do action */
extern int cn_doDelayedAction(cn_action *action, int miliseconds);

/*  make a schedule object. timeout is wait time before first action execution.
    interval is time between the next execution. keep execute every interval
    until schedule is removed */
extern cn_sched *cn_makeSched(cn_action, int timeout, int interval);

extern int cn_desSched(cn_sched *sched);

extern int cn_desSchedNumerableInterface(void *args);

/*  add schedule to the action execution scheduler. */
extern int cn_schedAdd(cn_sched *sched);

/*  Remove a schedule from the action execution scheduler */
extern int cn_schedRemove(cn_sched *sched);

#endif
