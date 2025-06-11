/* ************************************************************************
> File Name:     /home/qlc/my_compiler/src/scanner.cpp
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年05月22日 星期四 21时50分53秒
> Description:   
 ************************************************************************/
#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include "common.h"
//手动普通的词法分析

typedef struct {
  const char* start;//指向正在处理的token
  const char* current;//指向正在扫描的字符位置 不断移动 直到识别出一个完整的token
  int line;//源代码行号
} Scanner;

Scanner scanner;
void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}


static char peek() {
  return *scanner.current;
}

static char advance() {
  scanner.current++;    
  return scanner.current[-1];  // 返回移动前的字符
}
/*
 *char arr[] = {'a', 'b', 'c', 'd'};
char* ptr = &arr[2];  // ptr 指向 'c'
printf("%c\n", ptr[-1]);  // 输出 'b'（合法）
 * */
static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}






