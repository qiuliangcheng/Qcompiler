/* ************************************************************************
> File Name:     value.c
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 17时38分51秒
> Description:   
 ************************************************************************/
#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "value.h"


void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values,
                               oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
        case VAL_NIL: printf("nil"); break;
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
        //case VAL_OBJ: printObject(value); break;
  }



}
