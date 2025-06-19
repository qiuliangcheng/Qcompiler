/* ************************************************************************
> File Name:     table.h
> Author:        qlc
> 微信:          a2744561792
> Created Time:  日  6/15 11:38:45 2025
> Description:   
 ************************************************************************/
#ifndef QLC_TABLE_H
#define QLC_TABLE_H


#include "common.h"
#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;


typedef struct {
    int count;//装的数量
    int capacity;//容量
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableGet(Table* table, ObjString* key, Value* value);

void tableAddAll(Table* from, Table* to);//将一个哈希表 放到 另一个哈希中
bool tableDelete(Table* table, ObjString* key);//删除条目放置一个tomb 这样下次有同样的key进来就可以找到
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
#endif
