/* ************************************************************************
> File Name:     vm.h
> Author:        qlc
> 微信:          a2744561792
> Created Time:  四  6/12 10:18:52 2025
> Description:   
 ************************************************************************/
#ifndef QLC_VM_H
#define QLC_VM_H
#include "value.h"
#include "chunk.h"
#include "object.h"
#include "memory.h"
#include "table.h"
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;//指向函数里第一个变量的名称
} CallFrame;//一个CallFrame代表一个正在进行的函数调用
typedef struct {
    Chunk* chunk;
    uint8_t *ip;//指向正在运行指令的指针
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];//运行时的栈
    Value* stackTop;
    Table strings;
    Table globals;
    Obj* objects;
    ObjUpvalue* openUpvalues;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    size_t bytesAllocated;//分配字节
    size_t nextGC;//垃圾回收触发器 越多触发的越晚
    ObjString* initString;
} VM;
extern VM vm;
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,//编译错误
    INTERPRET_RUNTIME_ERROR //运行错误
} InterpretResult;
void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();



#endif
