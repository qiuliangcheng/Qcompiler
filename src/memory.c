/* ************************************************************************
> File Name:     memory.c
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年06月11日 星期三 16时57分51秒
> Description:   
 ************************************************************************/

#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "compiler.h"

#define GC_HEAP_GROW_FACTOR 1.5
void* reallocate(void* pointer,size_t oldSize,size_t newSize){
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {//如果是申请新内存 看看有没有不用的内存可以进行回收
        #ifdef DEBUG_STRESS_GC
            collectGarbage();
        #endif  
        // if (vm.bytesAllocated > vm.nextGC) {
        //     collectGarbage();
        // }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if(result==NULL) exit(1);
    return result; 
}
static void freeObject(Obj* object) {

    #ifdef DEBUG_LOG_GC
        printf("%p free type %d\n", (void*)object, object->type);
    #endif   
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues,
                 closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;

        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
    free(vm.grayStack);
}

void markObject(Obj* object) {
    if (object == NULL) return;
    if (object->isMarked) return;
    #ifdef DEBUG_LOG_GC
        printf("%p mark ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif    
    
    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
    }
    if (vm.grayStack == NULL) exit(1);
    vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}



static void markRoots() {
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        //正在堆栈中的不能进行回收
        markValue(*slot);
    }
    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);//函数调用的过程中需要分配新值
    }
    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

     markTable(&vm.globals);//标记全局变量
     markCompilerRoots();//编译器会定期从堆中获取内存 存储字面量和常量表 在编译期间进行分配的话 也需要进行标记
}
static void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}
static void blackenObject(Obj* object) {
    #ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif
    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);//上值所对应的具体的值
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
        break;
    }
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);//跟踪对象及其引用
    }
}

static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;//变为白色
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }
            freeObject(unreached);
        }
    }
}


//每次进行内存回收的时候都会进行垃圾回收
void collectGarbage() {
    #ifdef DEBUG_LOG_GC
        printf("-- gc begin\n");
        size_t before = vm.bytesAllocated;
    #endif
    //白色: 在垃圾回收的开始阶段，每个对象都是白色的。这种颜色意味着我们根本没有达到或处理该对象。
    markRoots();
    //灰色: 在标记过程中，当我们第一次达到某个对象时，我们将其染为灰色。这种颜色意味着我们知道这个对象本身是可达的，不应该被收集。
    //但我们还没有通过它来跟踪它引用的其它对象。用图算法的术语来说，这就是工作列表（worklist）——我们知道但还没有被处理的对象集合。
    traceReferences();//跟踪第一个标记后的后续引用
    //当我们接受一个灰色对象，并将其引用的所有对象全部标记后，我们就把这个灰色对象变为黑色

    tableRemoveWhite(&vm.strings);
    sweep();
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
    #ifdef DEBUG_LOG_GC
        printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
                before - vm.bytesAllocated, before, vm.bytesAllocated,
                vm.nextGC);
        printf("-- gc end\n");
    #endif
}