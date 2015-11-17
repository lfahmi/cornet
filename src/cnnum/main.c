#include "cornet/cnnum.h"

int cn_cnnumDefaultItemDestructor(void *item)
{
    if(item != NULL){free(item);}
    return 0;
}
