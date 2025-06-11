/* ************************************************************************
> File Name:     memory.c
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 16时57分51秒
> Description:   
 ************************************************************************/

#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
void* reallocate(void* pointer,size_t oldSize,size_t newSize){
     if (newSize == 0) {
        free(pointer);
        return NULL;
     }

    void* result = realloc(pointer, newSize);
    if(result==NULL) exit(1);
    return result; 
}


