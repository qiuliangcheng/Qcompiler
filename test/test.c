/* ************************************************************************
> File Name:     test.c
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 17时23分23秒
> Description:   
 ************************************************************************/

#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "value.h"
int main(int argc,char** argv){
    Chunk chunk;
    initChunk(&chunk);
    int constant = addConstant(&chunk, NUMBER_VAL(1.2));
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    writeChunk(&chunk, OP_RETURN, 123);
    disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);
    return 0;
}

