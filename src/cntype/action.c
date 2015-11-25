#include "cornet/cntype.h"

/**
    * Make action object.
    * Return cn_action object if successful,
    * otherwise return NULL.
    * @param funcptr : function pointer.
    * @param args : is a struct container arguments for funcptr.
    * @param argsDestructor : is a destructor for args when action complete.
    */
cn_action *cn_makeAction(cn_voidFunc funcptr, void *args, cn_syncFuncPTR argsDestructor)
{
    // Allocate memory space for cn_action type, defensively.
    cn_action *result = malloc(sizeof(cn_action));
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

/**
    * Destroy action object.
    * Return 0 if successful.
    * @param action : target.
    */
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
        #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNTYPE_H_LVL == 1
        cn_log("[DEBUG][file:%s][func:%s][line:%d] dealloc attempt next.\n", __FILE__, __func__, __LINE__);
        #endif // CN_DEBUG_MODE
        // Crash potential.
        free(action);
    }
    // Just return.
    return 0;
}

/**
    * cn_desAction interface for numerable type
    * @param args : target.
    */
int cn_desActionNumberableInterface(void *args)
{
    // true work is in cn_desAction
    return cn_desAction(args);
}
