
#include <stdio.h>
//#include <sys/types.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>
//#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include "cornet.h"

int cnt, lastcnt;
int recIndex;
struct timespec start, end;
int mtime, secs, usecs;

void *printer(void *arg)
{

    //printf("ohhhmmmyyygod printer %d cnt %d\n", (int)arg, cnt++);
    cn_list *mylist = cn_makeList("string opration", sizeof(char), 200, false);
    char mytext[] = "hey my name is [name], you know... [name] mean brain. [name] i love you.";
    cn_listInsertAt(mylist, 0, &mytext, sizeof(mytext));
    //printf("content %s\n", *mylist->b);
    char nameKey[] = "[name]";
    char nameVal[] = "fahmi";
    int o = 0;
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);


    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameVal, sizeof(nameVal) - 1);
        cn_listSplice(mylist, o, sizeof(nameVal) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameKey, sizeof(nameKey) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);

    cn_desList(mylist);
    //printf("content %s\n", cn_listGet(mylist, 0));
    //sleep(1);

    __sync_fetch_and_add(&cnt, 1);
    return NULL;
}

void printElapsed()
{
    clock_gettime(CLOCK_MONOTONIC, &end);
    secs  = end.tv_sec  - start.tv_sec;
    if(secs > 0)
    {
        usecs = end.tv_nsec + (1000000000 - start.tv_nsec);
    }
    else
    {
        usecs = end.tv_nsec - start.tv_nsec;
    }
    //mtime = ((secs) * 1000 + usecs/1000000.0) + 0.5;
    double ops = (double)(cnt - lastcnt) / secs;
    printf(">%d Elapsed time: %d sec, nsec %d total-cnt %d, cnt %d opration per sec %f priode %fus\n", recIndex++, secs, usecs, __sync_fetch_and_add(&cnt, 0), __sync_fetch_and_add(&cnt, 0) - lastcnt, ops, (1/ops) * 1000000);
    lastcnt = __sync_fetch_and_add(&cnt, 0);
    start = end;
}

void dominic(int *ptra)
{
    int a = 123;
    ptra = &a;
    printf("chaning ptra to %lu\n", (long unsigned int)ptra);
}

void *workerPrinter(void *arg)
{
    cn_threadpool *thpl = arg;
    cn_threadpoolWorker **workers = *thpl->threads->b;
    printf("work left = %d. worked 1=%d 2=%d 3=%d == %u\n", __sync_fetch_and_add(&thpl->actions->cnt, 0),
        __sync_fetch_and_add(&workers[0]->gotThplWork, 0),
        __sync_fetch_and_add(&workers[1]->gotThplWork, 0),
        __sync_fetch_and_add(&workers[2]->gotThplWork, 0),
        __sync_fetch_and_add(&thpl->workingThread, 0));
    return NULL;
}

int main(int argc, char *argv[])
{
    printf("dic size %d list size %d\n", sizeof(cn_dictionary4b), sizeof(cn_list));
    cn_queue *myQue = cn_makeQue("testQue", sizeof(char));
    cn_queEn(myQue, "1");
    cn_queEn(myQue, "2");
    cn_queEn(myQue, "3");
    char *item;
    CN_QUEUE_FOREACH(item, myQue,
    {
        printf("%c\n", *item);
    });
    cn_desQue(myQue, NULL);
    printf("sizeof cn_mutex_t %d\n", sizeof(cn_mutex_t));
    cn_action *act = cn_makeAction("test", NULL, NULL, NULL);
    cn_list *pls = cn_makeList("pls", 1, 1, false);
    cn_listInsertAt(pls, pls->cnt, "1234567890", 11);
    char *cChar;
    CN_LIST_FOREACH(cChar, pls)
    {
        printf("%c\n", *cChar);
    }
    cn_typeAppendObject(pls, act);
    cn_action *actparsed = cn_typeGet(pls, cn_action_type_id);
    printf("ref name of act parsed %s\n", actparsed->refname);
    cn_list *plsparsed = cn_typeGet(actparsed, cn_list_type_id);
    printf("ref name of pls parsed %s\n", plsparsed->refname);
    cn_desList(plsparsed);
    cn_desAction(actparsed);

    cn_dictionary4b *dict = cn_makeDict4b("testdictionary");
    cn_dict4bSet(dict, 12, "ohayoooooo");
    cn_dict4bSet(dict, 800, "trampolines");
    cn_dict4bSet(dict, 1234, "MYONETWOTHREEFOUR");
    printf("dict key 12 = %s, 800 = %s, 1234 = %s\n", (char *)cn_dict4bGet(dict, 12), (char *)cn_dict4bGet(dict, 800), (char *)cn_dict4bGet(dict, 1234));
    cn_desDict4b(dict);
    printer(NULL);
    setpriority(PRIO_PROCESS, getpid(), -19);
    clock_gettime(CLOCK_MONOTONIC, &start);
    cn_doDelayedAction(cn_makeAction("printerTROUBLEMATIC", printer, (void *)987, NULL), 10);
    cn_list *mylist = cn_makeList("string opration", sizeof(char), 200, false);
    char mytext[] = "hey my name is [name], you know... [name] mean brain. [name] i love you.";
    cn_listInsertAt(mylist, 0, &mytext, sizeof(mytext));
    printf("content %s\n", (char *)*mylist->b);
    char nameKey[] = "[name]";
    char nameVal[] = "fahmi";
    int o = 0;
    while(o >= 0)
    {
        o = cn_listIndexOf(mylist, &nameKey, sizeof(nameKey) - 1);
        cn_listSplice(mylist, o, sizeof(nameKey) - 1, NULL);
        cn_listInsertAt(mylist, o, &nameVal, sizeof(nameVal) - 1);
    }
    cn_listSplice(mylist, cn_listIndexOf(mylist, "456", 3), 3, NULL);
    printf("content %s\n", (char *)cn_listGet(mylist, 0));
    cn_desList(mylist);
    //cn_schedAdd(cn_makeSched("timer100ms", cn_makeAction("timePrinter", printer, (void *)1234, NULL), 0, 1));

    struct timespec ts, tstart, tend;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tstart);

    int i;
    for(i = 0; i < 1000; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        //printf("time_t size %d value %u nsec %lu\n", sizeof(time_t), (int)ts.tv_sec, (long int)ts.tv_nsec);
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tend);
    printf("elapsed nsec thread cputime %lu, end %lu\n", (long unsigned int)(tend.tv_nsec - tstart.tv_nsec), (long unsigned int)tend.tv_nsec);

    printElapsed();
    cn_threadpool *tp = cn_makeThpl("threadPoolTest", 3);
    cn_sched *workCntPrint = cn_makeSched("workedThread count schedule", cn_makeAction("the workerThread Count action", workerPrinter, (void *)tp, NULL), 0, 4);
    cn_schedAdd(workCntPrint);
    printElapsed();
    cn_log("tp address %d\n\n", (int)tp);

    cn_action *job = cn_makeAction("main.printerRepeated", printer, (void *)i, NULL);
    job->callSelfDestructor = false;

    //cn_mutex_lock(&tp->jobsKey);
    for(i = 0; i < 1000000; i++)
    {
        cn_thplEnq(tp, job);
        //cn_queEn(tp->actions, job);
        //cn_cond_signal(&tp->jobsCond);
    }
    //cn_mutex_unlock(&tp->jobsKey);

    printf("<<<<<<<<>>>>>>>> COND %d\n", __sync_fetch_and_add(&tp->jobsCond.cond, 0));

    cn_thplEnq(tp, cn_makeAction("printTimeDone", (cn_voidFunc)printElapsed, NULL, NULL));
    printf("just done submit jobs\n");
    printElapsed();
    cn_sleep(10000);
    cn_desSched(workCntPrint);
    cn_sleep(4000);
    printElapsed();
    cn_desThplAsync(tp, NULL);
    printElapsed();
    printf("sizeof sec %d, nsec %d\n", sizeof(end.tv_sec), sizeof(end.tv_nsec));
    cn_sleep(5000);
    job->callSelfDestructor = true;
    cn_desAction(job);
    cn_stopScheduler();
    return 0;
}
