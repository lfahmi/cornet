#include "cornet/cntype.h"

/*  Make action object, funcptr is a function pointer and
    args is pointer for arguments used for the function.
    Return cn_action object if successful,
    otherwise return NULL. */
cn_action *cn_makeAction(cn_voidFunc funcptr, void *args, cn_syncFuncPTR argsDestructor)
{
    // Allocate memory space for cn_action type
    cn_action *result = malloc(sizeof(cn_action));
    // if allocation failed return NULL
    if(result == NULL){return NULL;}
    // Set function pointer, arguments object pointer
    // and cancelation status pointer
    result->funcptr = funcptr;
    result->argsDestructor = argsDestructor;
    result->dontCallDestructor = false;
    result->args = args;
    result->cancel = false;
    // cn_action creation has done, return result.
    return result;
}

/*  Destroy action object, action is the target.
    Return 0 if successful. */
int cn_desAction(cn_action *action)
{
    // If action is not null then Dealloc cn_action type
    // crash potential.
    if(action != NULL)
    {
        // If argument objects destructor is not null and args is not null
        // then execute argument objects destructor.
        if(!action->dontCallDestructor && action->argsDestructor != NULL && action->args != NULL)
        {
            action->argsDestructor(action->args);
        }
        #if CN_DEBUG_MODE == 1
        cn_log("[DEBUG][file:%s][func:%s][line:%s] dealloc attempt next.", __FILE__, __func__, __LINE__);
        #endif // CN_DEBUG_MODE
        // Crash potential.
        free(action);
    }
    // Just return.
    return 0;
}

int cn_desActionNumberableInterface(void *args)
{
    return cn_desAction(args);
}
