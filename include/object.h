/* ************************************************************************
> File Name:     object.h
> Author:        qlc
> 微信:          a2744561792
> Created Time:  六  6/14 18:11:29 2025
> Description:   
 ************************************************************************/

#ifndef QLC_OBJECT_H
#define QLC_OBJECT_H
#include "common.h"
#include "chunk.h"
#include "value.h"
#include "chunk.h"
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define AS_NATIVE(value) \
    (((ObjNative*)AS_OBJ(value))->function)



typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
} ObjType;


struct Obj {
    ObjType type;
    struct Obj* next;
};
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;//每个字符串存储一个hash 防止重复计算
};
typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;
//本地函数——可以从Lox调用，但是使用C语言实现

typedef Value (*NativeFn)(int argCount, Value* args);
typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;
ObjNative* newNative(NativeFn function);
ObjFunction* newFunction();
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
