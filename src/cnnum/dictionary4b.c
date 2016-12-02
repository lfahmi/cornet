#include "cornet/cnnum.h"

cn_type_id_t cn_dictionary4b_type_id = 0;

/**
    * Construct Dictionary 4bytes-key object structure.
    * @param refname : reference name.
    * return NULL for error
    */
cn_dictionary4b *cn_makeDict4b(const char *refname)
{
    // Defensive code
    if(refname == NULL){return NULL;}

    // Allocate memory for object structure, defensively
    cn_dictionary4b *result = malloc(sizeof(cn_dictionary4b));
    if(result == NULL){return NULL;}

    // create list for key and value mapping
    result->itemKey = cn_makeList("cn_ictionary.key", sizeof(uint32_t), 10, false);
    if(result->itemKey == NULL)
    {
        // failure happen, cleanup.
        free(result);
        return NULL;
    }
    result->itemValue = cn_makeList("cn_dictionary4b.value", sizeof(void *), 10, true);
    if(result->itemValue == NULL)
    {
        // failure happen, cleanup.
        cn_desList(result->itemKey);
        free(result);
        return NULL;
    }

    // initial object count in dictionary
    result->cnt = 0;

    // initialize mutex key
    cn_mutex_init(&result->key, NULL);

    // if type has not get it's id, get new id. initialize type structure for object
    if(cn_dictionary4b_type_id <= 0){cn_dictionary4b_type_id = cn_typeGetNewID();}
    cn_typeInit(&result->t, cn_dictionary4b_type_id);

    // return created object
    return result;
}

/**
    * Get the index in dictionary mapping containing key.
    * @param tdict : target dictionary
    * @param key : key to look for in key list
    * return < 0 for not found
    */
int cn_dict4bIndexOfKey(cn_dictionary4b *tdict, uint32_t key)
{
    // Defensive code
    if(tdict == NULL){return -1;}

    // lock the dictionary
    cn_mutex_lock(&tdict->key);

    // look for var key in key list
    int i = cn_listIndexOf(tdict->itemKey, &key, 1);

    // unlock the dictionary
    cn_mutex_unlock(&tdict->key);

    // return the value from search function
    return i;
}

/**
    * Get the index in dictionary mapping containing value
    * @param tdict : target dictionary
    * @param value : value to look for in value list
    * return < 0 for not found
    */
int cn_dict4bIndexOfValue(cn_dictionary4b *tdict, void *value)
{
    // Defensive code
    if(tdict == NULL || value == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // look for var value in value list
    int i = cn_listIndexOf(tdict->itemValue, value, 1);

    // unlock the dictionary access
    cn_mutex_unlock(&tdict->key);

    // return the value from search funtion
    return i;
}

/**
    * Set/Add(if key not exist) key-value pair item to dictionary.
    * @param tdict : target dictionary
    * @param key : key for dictionary item
    * @param value : value for dictionary item
    * return 0 for fine
    */
int cn_dict4bSet(cn_dictionary4b *tdict, uint32_t key, void *value)
{
    // Defensive code
    if(tdict == NULL || value == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);
    int i = cn_listIndexOf(tdict->itemKey, &key, 1);
    if(i < 0)
    {
        // no dictionary item with this key
        // new item index must be the end of the item list
        i = tdict->cnt;
    }

    // declare execution status
    int n = 0;

    // finaly insert the dictionary item at the end of the list
    n = cn_listInsertAt(tdict->itemKey, i, &key, 1);
    n = cn_listInsertAt(tdict->itemValue, i, value, 1);

    if(n == 0)
    {
        // insert method went smoothly. add dictionary items count by one
        tdict->cnt++;
    }

    // unlock dictionary access
    cn_mutex_unlock(&tdict->key);

    // return execution status
    return n;
}

/**
    * Get dictionary item value from dictionary by key
    * @param tdict : target dictionary
    * @param key : key to find the item value
    * return NULL for not found
    */
void *cn_dict4bGet(cn_dictionary4b *tdict, uint32_t key)
{
    // Defensive code
    if(tdict == NULL){return NULL;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // get the index of item key in key list
    int i = cn_listIndexOf(tdict->itemKey, &key, 1);

    // get the item value by item key index
    void *result = cn_listGet(tdict->itemValue, i);

    // unlock the dictionary access
    cn_mutex_unlock(&tdict->key);

    // return item value
    return result;
}

/**
    * Get dictionary item value from dictionary by index
    * @param tdict : target dictionary
    * @param indx : index of the item
    * return NULL for out of index range
    */
void *cn_dict4bGetByIndex(cn_dictionary4b *tdict, int indx)
{
    // Defensive code
    if(tdict == NULL || indx < 0){return NULL;}

    // return the item value in list at index
    return cn_listGet(tdict->itemValue, indx);
}

/**
    * Get the key for dictionary item that has matching dictionary item value
    * @param tdict : target dictionary
    * @param value : dictionary item value for search input
    * @param out : unsigned 32bit integer pointer to hold result dictionary item key
    * return 0 for FOUND
    */
int cn_dict4bGetKey(cn_dictionary4b *tdict, void *value, uint32_t *out)
{
    // Defensive code
    if(tdict == NULL || value == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // get index of dictionary item that has matching value
    int i = cn_listIndexOf(tdict->itemValue, value, 1);

    // if fails, return failure code without setting output.
    if(i < 0){return i;}

    // we found the dictionary item value matching. now set the result output value
    *out = *((uint32_t *)cn_listGet(tdict->itemKey, i));

    // unlock the dictionary access
    cn_mutex_unlock(&tdict->key);

    // report that we found the key
    return 0;
}

/**
    * remove dictionary item by their index
    * @param tdict : target dictionary
    * @param indx : index of dictionary item to delete
    * return 0 for fine
    */
static int dictRemoveByIndex(cn_dictionary4b *tdict, int indx)
{
    // this function is warped, no need protection

    // remove the key and value from key and value list.
    int n = cn_listRemoveAt(tdict->itemValue, indx);
    n = cn_listRemoveAt(tdict->itemKey, indx);

    // subtract dictionary count by one
    tdict->cnt--;

    // return execution status
    return n;
}

/**
    * remove dictionary item by their key
    * @param tdict : target dictionary
    * @param key : key of dictionary item to delete
    * return 0 for fine
    */
int cn_dict4bRemoveByKey(cn_dictionary4b *tdict, uint32_t key)
{
    // Defensive code
    if(tdict == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // get index of dictionary item to delete
    int i = cn_listIndexOf(tdict->itemKey, &key, 1);
    // the validity of index, will be checked by remove

    // remove the dictionary item by index
    int n = dictRemoveByIndex(tdict, i);

    // unlock dictioanry access
    cn_mutex_unlock(&tdict->key);

    // return execution status
    return n;
}

/**
    * remove dictionary item by their value
    * @param tdict : target dictionary
    * @param value : value of dictionary item to delete
    * return 0 for fine
    */
int cn_dict4bRemoveByValue(cn_dictionary4b *tdict, void *value)
{
    // Defensive code
    if(tdict == NULL || value == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // get the index of item to be removed
    int i = cn_listIndexOf(tdict->itemValue, value, 1);
    // the index validity checked by remove

    // remove the dictionary item by index
    int n = dictRemoveByIndex(tdict, i);

    // unlock the dictionary access
    cn_mutex_unlock(&tdict->key);

    // return execution status
    return n;
}

/**
    * remove dictionary item by their index
    * @param tdict : target dictionary
    * @param indx : index of dictionary item to delete
    * return 0 for fine
    */
int cn_dict4bRemoveByIndex(cn_dictionary4b *tdict, int indx)
{
    // Defensive code
    if(tdict == NULL || indx < 0){return -1;}

    // lockt the dictionary access
    cn_mutex_lock(&tdict->key);

    // remove the dictionary item by index
    int n = dictRemoveByIndex(tdict, indx);

    // unlock the dictionary access
    cn_mutex_unlock(&tdict->key);

    // return execution status
    return n;
}

/**
    * Remove all items in dictionary
    * @param tdict : target dictionary
    * @param itemDestructor : sync function for destruction of dictionary item
    * return 0 for fine
    */
int cn_dict4bEmpty(cn_dictionary4b *tdict, cn_syncFuncPTR itemDestructor)
{
    // Defensive code
    if(tdict == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // remove all dictionary item key
    int n = cn_listEmpty(tdict->itemKey, NULL);

    // remove all dictionary item value and destruct it
    n = cn_listEmpty(tdict->itemValue, itemDestructor);

    // set the dictionary items count to empty (0)
    tdict->cnt = 0;

    // unlock the dictioanry access
    cn_mutex_unlock(&tdict->key);

    // return execution status
    return n;
}

/**
    * Destruct cn_dictionary4b object structure
    * @param tdict : target dictionary
    * return 0 for fine
    */
int cn_desDict4b(cn_dictionary4b *tdict)
{
    // Defensive code
    if(tdict == NULL){return -1;}

    // lock the dictionary access
    cn_mutex_lock(&tdict->key);

    // destruct dictionary key list and value list
    int n = cn_desList(tdict->itemKey);
    n = cn_desList(tdict->itemValue);

    // destruct type structure
    n = cn_typeDestroy(&tdict->t);

    // set item count to empty
    tdict->cnt = 0;

    // unlock the dictionary access
    cn_mutex_unlock(&tdict->key);

    // destroy the dictionary key
    cn_mutex_destroy(&tdict->key);

    // deallocate dictionary structure from memory
    free(tdict);

    // return execution status
    return n;
}

