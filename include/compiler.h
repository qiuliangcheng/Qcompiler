/* ************************************************************************
> File Name:     compiler.h
> Author:        qlc
> 微信:          a2744561792
> Created Time:  四  6/12 15:36:23 2025
> Description:   
 ************************************************************************/
#ifndef QLC_COMPILER_H
#define QLC_COMPILER_H
#include "chunk.h"
#include "object.h"
ObjFunction* compile(const char* source);
#endif
