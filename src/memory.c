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
#include "vm.h"
void* reallocate(void* pointer,size_t oldSize,size_t newSize){
     if (newSize == 0) {
        free(pointer);
        return NULL;
     }

    void* result = realloc(pointer, newSize);
    if(result==NULL) exit(1);
    return result; 
}
static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
  }
}

