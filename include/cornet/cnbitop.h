#ifndef _CNBITOP_H
#define _CNBITOP_H 1
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>

/*  Copy bytes from src to dest.
    return 0 if successful. */
extern int cn_bitcp(void *dest, void* src, int len);

/*  Compare bytes objA with bytes objB.
    returns true if equal */
extern bool cn_bitcompare(char *objA, char* objB, int len);

#endif
