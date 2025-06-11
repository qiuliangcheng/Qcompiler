/* ************************************************************************
> File Name:     debug.h
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 17时25分45秒
> Description:   
 ************************************************************************/
#ifndef QLC_DEBUG_H
#define QLC_DEBUG_H

#include "chunk.h"
void disassembleChunk(Chunk* chunk,const char*name);//解析所有的指令
int disassembleInstruction(Chunk* chunk, int offset);//解析每一个指令

#endif
