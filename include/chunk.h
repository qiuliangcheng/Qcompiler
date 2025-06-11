#ifndef QLC_CHUNK_H
#define QLC_CHUNK_H

#include "common.h"
#include "value.h"
typedef enum{

    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,//局部变量
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,//定义全局变量
    OP_SET_GLOBAL,//修改已经在的全局变量
    OP_GET_UPVALUE,//定义上值 用于闭包 捕获作用域外的值
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,//类的字段属性
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,//负数
    OP_PRINT,//print
    OP_JUMP,//函数调用 或者 if else
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,//优化方法调用 比如class.method() 直接运行
    
    OP_SUPER_INVOKE,//超类方法的优化调用
    OP_CLOSE_OPVALUE,
    OP_RETURN,
    OP_CLASS,
    OP_INHERIT,//继承
    OP_METHOD


}OpCode;

typedef struct{
    int count;//已经使用了多少code
    int capacity;//数组中的容量
    uint8_t* code;
    int* lines;//每个指令处于的行数 
    ValueArray constants;//常量数组
}Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk,uint8_t byte,int line);
int addConstant(Chunk* chunk,Value value);



#endif
