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
  int line;//源代码行号  扫描整个文件 将整个的行号传递给token
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
//读取当前的current字符 并且将字符向后挪动一个
static char advance() {
  scanner.current++;    
  return scanner.current[-1];  // 返回移动前的字符
}

//匹配当前字符 匹配成功往下走一个字符
static bool match(char c){
    if(isAtEnd()) return false;
    if(*scanner.current!=c) return false;
    scanner.current++;
    return true;

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

//未终止的字符串和无法识别的字符 才会有error
static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
  return token;
}
static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;//errortoken的信息自己定义
}
//返回当前的字符
static char peek() {
  return *scanner.current;
}
static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();//跳过空格、回车、制表符
                break;
            case '\n':
                //printf("换行");
                scanner.line++;
                advance();
                break;
        case '/':
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                return;
            }
        break;
            default:
                return;
    }
  }
}

static Token number() {
    while (isDigit(peek())) advance();
    //123.456
    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}


static Token string(){
    while(peek()!='"'&&!isAtEnd()){
        if(peek()=='\n') scanner.line++;
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string.");
    //匹配到了"
    advance();//长度包含从引号开始
    return makeToken(TOKEN_STRING);
}

static TokenType checkKeyword(int start, int length,const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;

    }
    return TOKEN_IDENTIFIER;//普通的字符名字
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}
Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;


    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advance();
    if (isAlpha(c)) return identifier();//字符标识
    if (isDigit(c)) return number();
    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(
                match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
      }
    return errorToken("Unexpected character.");
}




