#include "cornet/cntype.h"
#include "time.h"

#define __CN_DEF_FIRSTWAIT 5
#define __CN_DEF_WAITNEXTPOWER 4
#define __CN_DEF_MAXWAITTIME 200000000

//static struct timespec __zeroTime = {.tv_sec = 0, .tv_nsec = 0};

int cn_mutex_init(struct cn_mutex_t *cn_mutex, void *ignore)
{
    __sync_synchronize();
    cn_mutex->mutex = 1;
    __sync_fetch_and_sub(&cn_mutex->mutex, __sync_fetch_and_add(&cn_mutex->mutex, 0));
    __sync_fetch_and_add(&cn_mutex->mutex, 1);
    return 0;
}

void cn_mutex_destroy(struct cn_mutex_t *cn_mutex)
{
    __sync_synchronize();
    __sync_fetch_and_sub(&cn_mutex->mutex, __sync_fetch_and_add(&cn_mutex->mutex, 1));
}

void cn_mutex_lock(struct cn_mutex_t *cn_mutex)
{
    __sync_synchronize();
    struct timespec t;
    int nsec = 0;
    t.tv_sec = 0;
    while(!__sync_bool_compare_and_swap(&cn_mutex->mutex, 1, 0))
    {
        t.tv_nsec = nsec;
        if(nsec == 0){nsec += __CN_DEF_FIRSTWAIT;}
        else if(nsec < __CN_DEF_MAXWAITTIME){nsec *= __CN_DEF_WAITNEXTPOWER;}
        else if(nsec > __CN_DEF_MAXWAITTIME){nsec = __CN_DEF_MAXWAITTIME;}
        nanosleep(&t, NULL);
    }
}

/*void cn_mutex_unlock(struct cn_mutex_t *cn_mutex)
{
    __sync_bool_compare_and_swap(&cn_mutex->mutex, 0, 1);
}*/


void cn_cond_init(struct cn_cond_t *cond, void *ignore)
{
    __sync_synchronize();
    cond->cond = 0;
    cond->nwaiters = 0;
    __sync_fetch_and_sub(&cond->cond, __sync_fetch_and_add(&cond->cond, 0));
    __sync_fetch_and_sub(&cond->cond, __sync_fetch_and_add(&cond->nwaiters, 0));
}

void cn_cond_destroy(struct cn_cond_t *cond)
{
    __sync_synchronize();
    __sync_fetch_and_sub(&cond->cond, __sync_fetch_and_add(&cond->cond, 1));
}

void cn_cond_wait(struct cn_cond_t *cond, struct cn_mutex_t *cn_mutex)
{
    __sync_synchronize();
    struct timespec t;
    int nsec = 0;
    t.tv_sec = 0;

    __sync_bool_compare_and_swap(&cn_mutex->mutex, 0, 1);
    __sync_fetch_and_add(&cond->nwaiters, 1);
    int tcond = __sync_fetch_and_add(&cond->cond, 0);
    while(tcond < 1 || !__sync_val_compare_and_swap(&cond->cond, tcond, tcond - 1))
    {
        t.tv_nsec = nsec;
        if(nsec == 0){nsec += __CN_DEF_FIRSTWAIT;}
        else if(nsec < __CN_DEF_MAXWAITTIME){nsec *= __CN_DEF_WAITNEXTPOWER;}
        else if(nsec > __CN_DEF_MAXWAITTIME){nsec = __CN_DEF_MAXWAITTIME;}
        nanosleep(&t, NULL);
        tcond = __sync_fetch_and_add(&cond->cond, 0);
    }
    __sync_fetch_and_sub(&cond->nwaiters, 1);
    nsec = 0;
    while(!__sync_bool_compare_and_swap(&cn_mutex->mutex, 1, 0))
    {
        t.tv_nsec = nsec;
        if(nsec == 0){nsec += __CN_DEF_FIRSTWAIT;}
        else if(nsec < __CN_DEF_MAXWAITTIME){nsec *= __CN_DEF_WAITNEXTPOWER;}
        else if(nsec > __CN_DEF_MAXWAITTIME){nsec = __CN_DEF_MAXWAITTIME;}
        nanosleep(&t, NULL);
    }
}

void cn_cond_signal(struct cn_cond_t *cond)
{
    __sync_synchronize();
    if(__sync_fetch_and_add(&cond->cond, 0) < __sync_fetch_and_add(&cond->nwaiters, 0))
    {
        __sync_fetch_and_add(&cond->cond, 1);
    }
}
