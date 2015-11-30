#ifndef _CNTYPE_H
#define _CNTYPE_H 1
#include <stdbool.h>
#include <stdlib.h>

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

/* ACTION */

/**
    * cn_action definition.
    * NOTE: args wouldn't be automatically deallocated for each
    * function in this library. always deallocate inside funcptr if needed!
    */
typedef struct
{
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
} cn_action;

#include "cornet/cndebug.h"
#undef CN_DEBUG_MODE_CNTYPE_H_LVL
#define CN_DEBUG_MODE_CNTYPE_H_LVL 1
#undef CN_DEBUG_MODE_FREE
#define CN_DEBUG_MODE_FREE 0

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

#endif
