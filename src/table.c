/* ************************************************************************
> File Name:     table.c
> Author:        qlc
> 微信:          a2744561792
> Created Time:  日  6/15 11:38:53 2025
> Description:   
 ************************************************************************/
#include "table.h"

#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table){
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}
void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity,
                        ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

//重新分配桶数组 并将原来的数据拷贝
static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;
        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        table->count++;
        dest->value = entry->value;
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;

}

bool tableSet(Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    Entry* entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) table->count++;
    entry->key = key;
    entry->value = value;
    return isNewKey;
}
void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
        tableSet(to, entry->key, entry->value);
        }
    }
}
bool tableGet(Table* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;
    *value = entry->value;
    return true;
}
bool tableDelete(Table* table, ObjString* key) {
    if (table->count == 0) return false;
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;
    // tomb设置一个bool值 检索的时候碰到了就往下继续检索
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}
//字符串驻留 相同的字符串指向的同一个指针 然后使用==进行比较
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) return NULL;
        }else if (entry->key->length == length &&
            entry->key->hash == hash &&
            memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }
        index = (index + 1) % table->capacity;
  }
}

