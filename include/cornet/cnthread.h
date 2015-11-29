#ifndef _CNTHREAD_H
#define _CNTHREAD_H 1

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "cornet/cnsched.h"
#include "cornet/cntype.h"
#include "cornet/cnnum.h"

/* THREADPOOL */

typedef struct
{
    const char *refname;
    cn_list *threads;
    uint32_t workingThread;
    cn_queue *actions;
    pthread_mutex_t key;
    pthread_mutex_t terminatorKey;
    pthread_cond_t cond;
    pthread_attr_t threadAttr;
} cn_threadpool;

typedef struct
{
    const char *refname;
    pthread_t thread_t;
    bool gotThplWork;
    bool running;
    int threadid;
    cn_threadpool *parent;
} cn_thread;

#include "cornet/cndebug.h"

//#define CN_DEBUG_MODE_CNTHREAD_H 1
#define CN_DEBUG_MODE_CNTHREAD_H_LVL 1
/* THREADPOOL */

extern cn_threadpool *cn_makeThpl(const char *refname, uint32_t threadCnt);

extern int cn_thplEnq(cn_threadpool *thpl, cn_action *action);

extern int cn_thplDecWorker(cn_threadpool *thpl, int decCnt);

extern int cn_thplDecWorkerAsync(cn_threadpool *thpl, int decCnt, cn_action *callback);

extern int cn_thplIncWorker(cn_threadpool *thpl, int incCnt);

extern int cn_thplIncWorkerAsync(cn_threadpool *thpl, int incCnt, cn_action *callback);

extern int cn_thplCancelAll(cn_threadpool *thpl);

extern int cn_desThpl(cn_threadpool *thpl);

extern int cn_desThplAsync(cn_threadpool *thpl, cn_action *callback);

#endif
