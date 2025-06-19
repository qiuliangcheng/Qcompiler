/* ************************************************************************
> File Name:     chunk.c
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 16时43分44秒
> Description:   
 ************************************************************************/

#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}


void writeChunk(Chunk* chunk,uint8_t byte,int line){
    if(chunk->capacity<chunk->count+1){
        int oldCap=chunk->capacity;
        chunk->capacity=GROW_CAPACITY(oldCap);
        chunk->code=GROW_ARRAY(uint8_t,chunk->code,oldCap,chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,oldCap, chunk->capacity);
    }
    chunk->code[chunk->count]=byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeValueArray(&chunk->constants);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    initChunk(chunk);
}

//只是单纯的写到常量表里
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;//返回位于常量表的索引
}
