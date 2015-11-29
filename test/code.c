
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
unsigned long mtime, secs, usecs;

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
    cn_desList(mylist);
    //printf("content %s\n", cn_listGet(mylist, 0));
    //sleep(1);

    cnt++;
    return NULL;
}

void printElapsed()
{
    clock_gettime(CLOCK_MONOTONIC, &end);
    secs  = end.tv_sec  - start.tv_sec;
    usecs = end.tv_nsec - start.tv_nsec;
    //mtime = ((secs) * 1000 + usecs/1000000.0) + 0.5;
    double ops = (double)(cnt - lastcnt) / secs;
    printf(">%d Elapsed time: %lu sec, nsec %lu, cnt %d opration per sec %f priode %fus\n", recIndex++, secs, usecs, cnt - lastcnt, ops, (1/ops) * 1000000);
    lastcnt = cnt;
    start = end;
}

void dominic(int *ptra)
{
    int a = 123;
    ptra = &a;
    printf("chaning ptra to %lu\n", (long unsigned int)ptra);
}

int main(int argc, char *argv[])
{
    pthread_mutex_t key;
    pthread_mutex_init(&key,NULL);
    pthread_mutex_lock(&key);
    pthread_mutex_unlock(&key);
    printer(NULL);
    setpriority(PRIO_PROCESS, getpid(), 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    cn_doDelayedAction(cn_makeAction("printer", printer, (void *)987, NULL), 10);
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
    printElapsed();
    cn_log("tp address %d\n\n", (int)tp);
    pthread_mutex_lock(&tp->jobsKey);
    for(i = 0; i < 3000000; i++)
    {
        cn_action *job = cn_makeAction("printer", printer, (void *)i, NULL);
        //cn_thplEnq(tp, job);
        cn_queEn(tp->actions, job);
        pthread_cond_signal(&tp->jobsCond);
    }
    pthread_mutex_unlock(&tp->jobsKey);
    cn_thplEnq(tp, cn_makeAction("printTimeDone", (cn_voidFunc)printElapsed, NULL, NULL));
    printf("just done submit jobs\n");
    printElapsed();
    cn_sleep(5000);
    for(i = 0; i < 2; i++)
    {
        cn_thplEnq(tp, cn_makeAction("printer", printer, (void *)i, NULL));
    }
    cn_sleep(5000);
    printElapsed();
    cn_desThplAsync(tp, NULL);
    printElapsed();
    printf("sizeof sec %d, nsec %d\n", sizeof(end.tv_sec), sizeof(end.tv_nsec));
    cn_sleep(20000);

    return 0;
}
