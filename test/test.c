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
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
        printf("\n");
        break;
        }
        interpret(line);
    }
}
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {//文件没有读取完全
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';
    
    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source); 

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc,char** argv){
    initVM();
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    /* Chunk chunk; */
    /* initChunk(&chunk); */
    /* int constant = addConstant(&chunk, NUMBER_VAL(1.2)); */
    /* writeChunk(&chunk, OP_CONSTANT, 123); */
    /* writeChunk(&chunk, constant, 123); */
    /* constant = addConstant(&chunk, NUMBER_VAL(2.4)); */
    /* writeChunk(&chunk, OP_CONSTANT, 123); */
    /* writeChunk(&chunk, constant, 123); */

    /* writeChunk(&chunk, OP_SUBTRACT, 123); */

    /* constant = addConstant(&chunk, NUMBER_VAL(3.6)); */
    /* writeChunk(&chunk, OP_CONSTANT, 123); */
    /* writeChunk(&chunk, constant, 123); */
    /* writeChunk(&chunk, OP_DIVIDE, 123); */
    /* writeChunk(&chunk,OP_NEGATE,123); */
    /* writeChunk(&chunk, OP_RETURN, 12); */
    /* disassembleChunk(&chunk, "test chunk"); */
    /* interpret(&chunk); */
    freeVM();
    /* freeChunk(&chunk); */
    return 0;
}

