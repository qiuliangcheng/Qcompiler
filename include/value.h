/* ************************************************************************
> File Name:     value.h
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 16时28分07秒
> Description:   
 ************************************************************************/
#ifndef QLC_VALUE_H
#define QLC_VALUE_H
#include <string.h>
#include "common.h"


typedef enum{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
}ValueType;

typedef struct{
    ValueType type;
    union{
        bool boolean;
        double number;
    }as;

}Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;//常量数组 

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

void printValue(Value value);






#endif

