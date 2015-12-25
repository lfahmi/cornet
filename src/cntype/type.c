#include "cornet/cntype.h"

cn_type_space_tag_size_t cn_type_space_tag_bof = 85, cn_type_space_tag_eof = 170;

static cn_type_id_t cn_type_last_id = 1;

cn_type_id_t cn_typeGetNewID()
{
    return cn_type_last_id++;
}

/**
    * Initialize cn_type, type definiton structure
    * @param target : target cn_type object.
    * @param type_id : type_id for this type definition.
    */
void cn_typeInit(cn_type *target, cn_type_id_t type_id)
{
    // Set the begin of file
    target->bof = cn_type_space_tag_bof;
    // Set the end of file
    target->eof = cn_type_space_tag_eof;
    // Set the type id
    target->type_id = type_id;
    // Set objects to null
    target->objs = NULL;
    // Init target key
    pthread_mutex_init(&target->key, NULL);
}

/**
    * Append an object to target object.
    * @param target : target type object.
    * @param object : object to be added.
    * return -200, target is not cn_type.
    * return -300, object is not cn_type.
    */
int cn_typeAppendObject(void *target, void *object)
{
    // Defensive code.
    if(!CN_IS_TYPE(target)){return -200;}
    if(!CN_IS_TYPE(object)){return -300;}

    // Parse the target and object
    cn_type *s = target, *o = object;

    // lock the target
    pthread_mutex_lock(&s->key);
    pthread_mutex_lock(&o->key);

    // declare execution status
    int n = 0;

    // prepare parameter
    bool sHasList = false;
    if(CN_TYPE_IS(s->objs, cn_list_type_id)){sHasList = true;}
    bool oHasList = false;
    if(CN_TYPE_IS(o->objs, cn_list_type_id)){oHasList = true;}

    if(sHasList && oHasList)
    {
        // subject and object has list
        // compare wich has higher count, to be choosen list
        if(s->objs->cnt >= o->objs->cnt)
        {
            /*  object object-list has less item, move them
                to subject object-list */
            // lock the object object-list;
            pthread_mutex_lock(&o->objs->key);

            // take the items from object object-list
            cn_type **items = *o->objs->b;

            // declare iterator
            int i;
            for(i = 0; i < o->objs->cnt; i++)
            {
                // declare work item
                cn_type *item = items[i];
                if(item->bof == cn_type_space_tag_bof && item->eof == cn_type_space_tag_eof)
                {
                    // item is cn_type, set item object-list by subject object-list
                    // lock the item, in any case there is someone using the item
                    // for cn_type operation. lets wait for it first
                    pthread_mutex_lock(&item->key);
                    item->objs = s->objs;

                    // then add the item to subject-object list
                    cn_listAppend(s->objs, item);

                    // unlock the item
                    pthread_mutex_unlock(&item->key);
                }
            }
            // unlock object object-list, then destrpy it.
            pthread_mutex_unlock(&o->objs->key);
            cn_desList(o->objs);
        }
        else
        {
            /*  subject object-list has less item , move them
                to object object-list */
            // lock the subject object-list;
            pthread_mutex_lock(&s->objs->key);

            // take the items from subject object-list
            cn_type **items = *s->objs->b;

            // declare iterator
            int i;
            for(i = 0; i < o->objs->cnt; i++)
            {
                // declare work item
                cn_type *item = items[i];
                if(item->bof == cn_type_space_tag_bof && item->eof == cn_type_space_tag_eof)
                {
                    // item is cn_type, set item object-list by object object-list
                    // lock the item, in any case there is someone using the item
                    // for cn_type operation. lets wait for it first
                    pthread_mutex_lock(&item->key);
                    item->objs = o->objs;

                    // then add the item to object object-list
                    cn_listAppend(o->objs, item);

                    // unlock the item
                    pthread_mutex_unlock(&item->key);
                }
            }
            // unlock subject object-list, then destrpy it.
            pthread_mutex_unlock(&s->objs->key);
            cn_desList(s->objs);
        }
    }
    else if(sHasList)
    {
        // subject has object-list already, set object object-list by subject object-list.
        o->objs = s->objs;
        n = cn_listAppend(s->objs, o);
    }
    else if(oHasList)
    {
        // object has object-list already, set target object-list by object object-list.
        s->objs = o->objs;
        n = cn_listAppend(s->objs, s);
    }
    else
    {
        // object-list is not created, lets initialize it.
        s->objs = cn_makeList("cn_type.objs", sizeof(void*), 4, true);
        if(s->objs == NULL)
        {
            // error occured
            pthread_mutex_unlock(&o->key);
            pthread_mutex_unlock(&s->key);
            return -1;
        }
        // share the object-list to object
        o->objs = s->objs;

        // add target to object-list
        n = cn_listAppend(s->objs, s);

        // finnaly lets append the object to target object-list.
        n = cn_listAppend(s->objs, object);
    }

    // unlock target and return status.
    pthread_mutex_unlock(&o->key);
    pthread_mutex_unlock(&s->key);
    return n;
}

/**
    * Get object of type matching type id from target
    * @param target : target type to fetch result.
    * @param whatType : type id of object to fetch.
    * return NULL for error.
    */
void *cn_typeGet(void *target, cn_type_id_t whatType)
{
    // Defensive code, type id never higher than signed id
    if(whatType == 0 || whatType > cn_type_last_id || !CN_IS_TYPE(target)){return NULL;}


    // parse the target
    cn_type *s = target;

    // subject it self is what we look for, return subject
    if(s->type_id == whatType){return s;}

    // subject is not match, lets look for it inside list
    // lock the target
    pthread_mutex_lock(&s->key);

    // lets check the list
    cn_list *objs = s->objs;
    if(CN_TYPE_IS(objs, cn_list_type_id) == false){pthread_mutex_unlock(&s->key); return NULL;}

    // lock the list access
    pthread_mutex_lock(&objs->key);

    // declare result pointer and iterator
    void *result;
    int i;

    // take list buffer as objects array
    cn_type **items = *objs->b;
    for(i = 0; i < s->objs->cnt; i++)
    {
        // check each object
        if(items[i]->type_id == whatType && items[i]->bof == cn_type_space_tag_bof && items[i]->eof == cn_type_space_tag_eof)
        {
            // we found the object that is the type we look for
            // set the result by item found and break the search loop
            result = items[i];
            break;
        }
    }

    // unlock the target and list.
    pthread_mutex_unlock(&objs->key);
    pthread_mutex_unlock(&s->key);

    // return result object.
    return result;
}

/**
    * Destroy cn_type object
    * @param target : target cn_type object
    */
int cn_typeDestroy(cn_type *target)
{
    // Defensive code
    if(!CN_IS_TYPE(target)){return -1;}

    // lock the target
    pthread_mutex_lock(&target->key);

    // declare status
    int n = 0;
    if(CN_TYPE_IS(target->objs, cn_list_type_id))
    {
        // target object-list is not null, lets do something
        if(target->objs->cnt > 1)
        {
            // we are not only member of object-list, remove our object
            n = cn_listRemove(target->objs, target, 1, NULL);
        }
        else
        {
            // we are the only member, just destruct the list.
            n = cn_desList(target->objs);
        }
    }
    // unlock, destroy key, return status.
    pthread_mutex_unlock(&target->key);
    pthread_mutex_destroy(&target->key);
    return n;
}
