#ifndef _CNTYPE_H
#define _CNTYPE_H 1
#include <stdbool.h>
#include <stdlib.h>

/*  cn_syncFuncPTR type, a function pointer that take pointer
    of arguments object and return int as status */
typedef int (*cn_syncFuncPTR)(void *args);

/*  cn_voidFunc type, a function pointer that take pointer
    of arguments object and return object pointer */
typedef void *(*cn_voidFunc)(void *args);

/* ACTION */

/* NOTE: args wouldn't be automatically deallocated for each
function in this library. always deallocate inside funcptr if needed! */
typedef struct
{
    cn_voidFunc funcptr;

    // the destructor for args objects.
    // read bellow for detail.
    cn_syncFuncPTR argsDestructor;

    /*  args. This object is going to be a pain to manage.
        you hove to keep track of what object that used for
        the function operational. and make the destructor for
        each kind of function that require operational object. */
    void *args;

    /*  This is bad to do. you should destruct your arguments
        in the argsDestructor and never do it yourself inside
        the funcptr. but in such emergency case, here is a bool
        to prevent action destructor to call args destructor to
        prevent crash by double free. */
    bool dontCallDestructor;
    bool cancel;
} cn_action;

/* ACTION */

/*  Make action object, funcptr is a function pointer and
    args is pointer for arguments used for the function.
    Return cn_action object if successful,
    NOTE: never use shared args. each action need a copy of each args
    and every object inside it, if it was global object then don't
    destruct that global object inside argsDestructor.
    if not, and if they were all just global variable. give
    argsDestructor NULL.
    otherwise return NULL. */
extern cn_action *cn_makeAction(cn_voidFunc funcptr, void *arg, cn_syncFuncPTR argsDestructor);

/*  Destruct action object, action is the target.
    Return 0 if successful. */
extern int cn_desAction(cn_action *action);

/*  An interface for cn_action destrutor for numerable object
    destructor. */
extern int cn_desActionNumerableInterface(void *args);

#endif
