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
#include "table.h"
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define AS_NATIVE(value) \
    (((ObjNative*)AS_OBJ(value))->function)

#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))

#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))

#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))

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
    bool isMarked;
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
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;//这是一个指向Value的指针,不是Value本身
    struct ObjUpvalue* next;
    Value closed;
} ObjUpvalue;
typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;//upvalue数组
    int upvalueCount;
} ObjClosure;
typedef struct {
    Obj obj;
    ObjString* name;
    Table methods;//类方法
} ObjClass;
ObjUpvalue* newUpvalue(Value* slot);
//本地函数——可以从Lox调用，但是使用C语言实现
typedef Value (*NativeFn)(int argCount, Value* args);
typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

typedef struct {
    Obj obj;
    ObjClass* klass;//示例属于某个类
    Table fields; //hash表 存储类变量以及函数
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;//方法只能在ObjInstances上调用  谁进行调用的什么方法
    ObjClosure* method;
} ObjBoundMethod;


ObjNative* newNative(NativeFn function);
ObjFunction* newFunction();
ObjClosure* newClosure(ObjFunction* function);
ObjClass* newClass(ObjString* name);
ObjBoundMethod* newBoundMethod(Value receiver,ObjClosure* method);
ObjInstance* newInstance(ObjClass* klass);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);


void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
