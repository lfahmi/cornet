#ifndef _CNTYPE_H
#define _CNTYPE_H 1
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

/* MUTEX */

struct cn_mutex_t
{
    int mutex;
};
typedef struct cn_mutex_t cn_mutex_t;

struct cn_cond_t
{
    int cond;
    int nwaiters;
};
typedef struct cn_cond_t cn_cond_t;

/* Function Pointers */

/**
    * cn_syncFuncPTR type, a function pointer that take pointer
    * of arguments object and return int as status
    */
typedef int (*cn_syncFuncPTR)(void *args);

/**
    * cn_voidFunc type, a function pointer that take pointer
    * of arguments object and return object pointer
    */
typedef void *(*cn_voidFunc)(void *args);

/* TYPE */
typedef unsigned char cn_type_space_tag_size_t;

extern cn_type_space_tag_size_t cn_type_space_tag_bof, cn_type_space_tag_eof;

typedef uint64_t cn_type_id_t;

/**   cn_type definition
    * structure to identify cn object type
    * and store various object type in one pointer
    */
struct cn_type
{
    cn_type_space_tag_size_t bof;
    cn_type_id_t type_id;
    struct cn_list *objs;
    cn_mutex_t key;
    cn_type_space_tag_size_t eof;
};

typedef struct cn_type cn_type;

/* ACTION */

extern cn_type_id_t cn_action_type_id;

/**   cn_action definition.
    * NOTE: args wouldn't be automatically deallocated for each
    * function in this library. always deallocate inside funcptr if needed!
    */
struct cn_action
{
    cn_type t;
    const char *refname;
    cn_voidFunc funcptr;

    // the destructor for args objects.
    // read bellow for detail.
    cn_syncFuncPTR argsDestructor;

    /*  args. This object is going to be a pain to manage.
        you hove to keep track of what object that used for
        the function operational. and make the destructor for
        each kind of function that require operational object. */
    void *args;

    /*  in case that your action is used frequently
        and you are still using the same argument.
        you dont need to make each action for each operation */
    bool callSelfDestructor; // Default value: true

    /*  This is bad to do. you should destruct your arguments
        in the argsDestructor and never do it yourself inside
        the funcptr. but in such emergency case, here is a bool
        to prevent action destructor to call args destructor to
        prevent crash by double free. */
    bool callArgsDestructor; // Default value: true
    bool cancel;
};

typedef struct cn_action cn_action;

/* MACROS */

#define CN_TYPE_IS(_cn_macro_var_target, _cn_macro_var_type_id) \
    (((_cn_macro_var_target) != NULL) && (((cn_type *)(_cn_macro_var_target))->bof == cn_type_space_tag_bof) && \
    (((cn_type *)(_cn_macro_var_target))->eof == cn_type_space_tag_eof) && (((cn_type *)(_cn_macro_var_target))->type_id == _cn_macro_var_type_id))

#define CN_IS_TYPE(_cn_macro_var_target) \
    (((_cn_macro_var_target) != NULL) && (((cn_type *)(_cn_macro_var_target))->bof == cn_type_space_tag_bof) && \
    (((cn_type *)(_cn_macro_var_target))->eof == cn_type_space_tag_eof))


#include "cornet/cndebug.h"
#undef CN_DEBUG_MODE_CNTYPE_H_LVL
//#define CN_DEBUG_MODE_CNTYPE_H_LVL 1
#undef CN_DEBUG_MODE_FREE
//#define CN_DEBUG_MODE_FREE 0

/* MUTEX */

extern int cn_mutex_init(struct cn_mutex_t *key, void *ignore);

extern void cn_mutex_destroy(struct cn_mutex_t *key);

extern void cn_mutex_lock(struct cn_mutex_t *key);

#define cn_mutex_unlock(cn_mutex) \
    { \
    struct cn_mutex_t *__macro_var_cn_mutex = cn_mutex; \
    __sync_bool_compare_and_swap(&__macro_var_cn_mutex->mutex, 0, 1); }


extern void cn_cond_init(struct cn_cond_t *key, void *ignore);

extern void cn_cond_destroy(struct cn_cond_t *key);

extern void cn_cond_lock(struct cn_cond_t *cond, void *ignore);

extern void cn_cond_wait(struct cn_cond_t *cond, struct cn_mutex_t *key);

extern void cn_cond_signal(struct cn_cond_t *cond);


/* TYPE */

extern cn_type_id_t cn_typeGetNewID();

extern void cn_typeInit(cn_type *target, cn_type_id_t type_id);

extern int cn_typeAppendObject(void *target, void *object);

extern void *cn_typeGet(void *target, cn_type_id_t whatType);

extern int cn_typeRemoveObject(void *target, void *whatType);

extern int cn_typeDestroy(cn_type *target);


/* ACTION */

/**
    * Make action object.
    * Return cn_action object if successful,
    * otherwise return NULL.
    * @param refname : reference name. (variable name)
    * @param funcptr : function pointer.
    * @param args : is a struct container arguments for funcptr.
    * @param argsDestructor : is a destructor for args when action complete.
    * NOTE: never use shared args. each action need a copy of each args
    * and every object inside it, if it was global object then don't
    * destruct that global object inside argsDestructor.
    * if not, and if they were all just global variable. give
    * argsDestructor NULL.
    */
extern cn_action *cn_makeAction(const char *refname, cn_voidFunc funcptr, void *arg, cn_syncFuncPTR argsDestructor);

/**
    * Destroy action object.
    * Return 0 if successful.
    * @param action : target.
    */
extern int cn_desAction(cn_action *action);

/**
    * cn_desAction interface for numerable type
    * @param args : target.
    */
extern int cn_desActionNumerableInterface(void *args);

#include "cornet/cnnum.h"

#endif

