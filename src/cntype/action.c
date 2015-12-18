#include "cornet/cntype.h"

#if CN_DEBUG_MODE_CNTYPE_H_LVL > 0
#define CN_DEBUG_TYPENAME "cn_action"
#endif // CN_DEBUG_MODE_CNTYPE_H_LVL

cn_type_id_t cn_action_type_id = 0;


/**
    * Default value for the first time cn_action created.
    */
    /*
static cn_action actionDefaultValue =
{
    .refname = NULL,
    .funcptr = NULL,
    .argsDestructor = NULL,
    .args = NULL,
    .callSelfDestructor = true,
    .callArgsDestructor = true,
    .cancel = false
};
*/

/**
    * Make action object.
    * Return cn_action object if successful,
    * otherwise return NULL.
    * @param refname : reference name. (variable name)
    * @param funcptr : function pointer.
    * @param args : is a struct container arguments for funcptr.
    * @param argsDestructor : is a destructor for args when action complete.
    */
cn_action *cn_makeAction(const char *refname, cn_voidFunc funcptr, void *args, cn_syncFuncPTR argsDestructor)
{
    // Allocate memory space for cn_action type, defensively.
    cn_action *result = malloc(sizeof(cn_action));
    if(result == NULL){return NULL;}

    // Set default value
   // *result = actionDefaultValue;

    // Set reference name
    result->refname = refname;

    // Set function pointer, arguments object pointer
    // and cancelation status pointer
    result->funcptr = funcptr;
    result->argsDestructor = argsDestructor;
    result->args = args;

    result->callSelfDestructor = true;
    result->callArgsDestructor = true;
    result->cancel = false;

    // if action type has no identifier, request identifier.
    if(cn_action_type_id < 1){cn_action_type_id = cn_typeGetNewID();}

    // initialize object type definition
    cn_typeInit(&result->t, cn_action_type_id);

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
    if(action != NULL && action->callSelfDestructor)
    {
        // If argument objects destructor is not null and args is not null
        // then execute argument objects destructor.
        if(action->callArgsDestructor && action->argsDestructor != NULL && action->args != NULL)
        {
            action->argsDestructor(action->args);
        }

        // Destruct object type definition
        cn_typeDestroy(&action->t);

        #if CN_DEBUG_MODE_FREE == 1 && CN_DEBUG_MODE_CNTYPE_H_LVL >= 1
        cn_log("[DEBUG][file:%s][func:%s][line:%d][%s:%s] dealloc attempt next.\n", __FILE__, __func__, __LINE__, action->refname, CN_DEBUG_TYPENAME);
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
int cn_desActionNumerableInterface(void *args)
{
    // true work is in cn_desAction
    return cn_desAction(args);
}
