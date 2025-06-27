/* ************************************************************************
> File Name:     vm.c
> Author:        qlc
> 微信:          a2744561792
> Created Time:  四  6/12 10:20:04 2025
> Description:   
 ************************************************************************/
#include "vm.h"
#include <stdio.h>
#include "common.h"
#include "debug.h"
#include <stdarg.h>
#include "compiler.h"
#include <time.h>
#include "object.h"
VM vm;//全局变量
      //
static void resetStack() {
    vm.frameCount = 0;
    vm.stackTop = vm.stack;
    vm.openUpvalues = NULL;
}

static void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void freeVM() {
    freeObjects();
    vm.initString = NULL;
    freeTable(&vm.globals);
    freeTable(&vm.strings);
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}
Value pop(){
    //stackTop 代表的是一个数组中的值
    vm.stackTop--;
    return *vm.stackTop;

}
static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}
static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

void initVM() {        
    resetStack();
    vm.objects = NULL;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
    vm.initString = NULL;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    initTable(&vm.strings);
    vm.initString = copyString("init", 4);
    initTable(&vm.globals);
    defineNative("clock", clockNative);
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    //追栈
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();

}

static bool call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.",
            closure->function->arity, argCount);
        return false;
    }    
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;//参数的前一个 函数名字
    return true;
}


static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                //printf("function call\n");
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            case OBJ_CLASS: { //再本身类的位置创建示例
                ObjClass* klass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                Value initializer;
                if (tableGet(&klass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.",argCount);
                    return false;
                }
                return true;
            }
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;//我槽0放入一个接收器
                return call(bound->method, argCount);
            }
            default:
                break; 
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

//类
static bool bindMethod(ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) { 
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), //实例：peek(0)
                                            AS_CLOSURE(method));
    pop();//实例取出
    push(OBJ_VAL(bound));//放到栈顶
    return true;
}



static ObjUpvalue* captureUpvalue(Value* local) {
    
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {//栈的地址
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }
    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }
    ObjUpvalue* createdUpvalue = newUpvalue(local);//指向那个已经存在在栈上的值 后面要放到堆上去
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    return createdUpvalue;
}
static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL &&
            vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
  }
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    //先进行查看 防止allocate的时候 对其进行删除了
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

// var closure = instance.method;
// closure(argument);  可以分开
static void defineMethod(ObjString* name) {
    //printf("name %p initname %p \n",name,vm.initString);
    Value method = peek(0);////函数方法
    ObjClass* klass = AS_CLASS(peek(1));
    // printf("method\n");
    // printf("%s\n",name->chars);
    tableSet(&klass->methods, name, method);
    //2025.6.25 name与initstring不一样
    pop();//方法去除
}

static bool invokeFromClass(ObjClass* klass, ObjString* name,
                            int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString* name, int argCount) {
    Value receiver = peek(argCount);
    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(receiver);
    Value value;
    if(tableGet(&instance->fields,name,&value)){ //变量
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }
    return invokeFromClass(instance->klass, name, argCount);
}

static InterpretResult run() {
CallFrame* frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

#define READ_STRING() AS_STRING(READ_CONSTANT())

    for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
#ifdef DEBUG_TRACE_EXECUTION
        printf("栈中的变量          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");//说明堆栈有很多
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code));
#endif  
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {

            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NEGATE:{
                //printf("负数符号里面\n");
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;    
            }
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            case OP_ADD:{
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                        "Operands must be two numbers or two strings.");
                        return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL,-); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL,*); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL,/); break;
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop());
                if (!invokeFromClass(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_NOT:
                 push(BOOL_VAL(isFalsey(pop())));
                 break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:{
                BINARY_OP(BOOL_VAL, >); break;
            }
            case OP_LESS:{
                BINARY_OP(BOOL_VAL, <); break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();
                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); //实例取出 放入属性值
                    push(value);
                    break;
                }
                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);//设置的属性值
                break;
            }
            case OP_GET_GLOBAL: {
                    ObjString* name = READ_STRING();
                    Value value;
                    if (!tableGet(&vm.globals, name, &value)) {
                        runtimeError("Undefined variable '%s'.", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(value);
                    break;
             }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                //先有才能重新设置值
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name); 
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);//如果使用init的话变为了示例
                break;
            }
            case OP_GET_SUPER: {
                //this super method
                ObjString* name = READ_STRING();//name
                ObjClass* superclass = AS_CLASS(pop());
                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            //father class inherit
            case OP_INHERIT: {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjClass* subclass = AS_CLASS(peek(0));
                tableAddAll(&AS_CLASS(superclass)->methods,
                            &subclass->methods);//父类的方法调用到子类 每个子类都有一个隐藏变量，
                            //用于存储对其超类的引用。当我们需要执行一个超类调用时，我们就从这个变量访问超类，并告诉运行时从那里开始查找方法。
                pop(); //father
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_INVOKE: //invoke name count
                ObjString* method = READ_STRING();//没错啊
                printf("%s\n",method->chars);
                int argCount = READ_BYTE();
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            case OP_METHOD:
                ObjString* s=READ_STRING();

                //printf("OP_Method %s %p\n",s->chars,s);
                defineMethod(s);
                break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_CALL: {
                //printf("准备进入函数调用里面\n");
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;//栈上面的最前面count是从左到右的参数
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] =captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];//从外层往里运行的，所以外层应用的已经在堆上分配好内存了
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);//我其实没有这个也可以吧
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }
                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
                //printValue(pop());
                //printf("\n");
                
            }
        }
    }
#undef READ_STRING
#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_SHORT
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;
    push(OBJ_VAL(function));
    //printf("栈中变量：%d\n",vm.stackTop-vm.stack);
    ObjClosure* closure = newClosure(function);
    pop();//
    push(OBJ_VAL(closure));
    call(closure, 0);
    //call(function, 0);
    //printf("conpiler over");
    return run();
}
