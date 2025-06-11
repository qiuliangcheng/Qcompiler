/* ************************************************************************
> File Name:     scanner.h
> Author:         qlc
> WeChat:           a2744561792
> Created Time:  2025年05月22日 星期四 21时44分05秒
> Description:   
 ************************************************************************/
#ifndef MY_SCANNER_H
#define MY_SCANNER_H

typedef enum {
    // 单字符符号
    TOKEN_LEFT_PAREN,    // (
    TOKEN_RIGHT_PAREN,   // )
    TOKEN_LEFT_BRACE,    // {
    TOKEN_RIGHT_BRACE,   // }
    TOKEN_COMMA,         // ,
    TOKEN_DOT,           // .
    TOKEN_MINUS,         // -
    TOKEN_PLUS,          // +
    TOKEN_SEMICOLON,     // ;
    TOKEN_SLASH,         // /
    TOKEN_STAR,          // *

    // 单字符或双字符符号
    TOKEN_BANG,          // !
    TOKEN_BANG_EQUAL,    // !=
    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=

    // 字面量
    TOKEN_IDENTIFIER,    // 如: count, maxValue
    TOKEN_STRING,        // 如: "hello"
    TOKEN_NUMBER,        // 如: 42, 3.14

    // 关键字
    TOKEN_AND,           // and 或 &&
    TOKEN_CLASS,         // class
    TOKEN_ELSE,          // else
    TOKEN_FALSE,         // false
    TOKEN_FOR,           // for
    TOKEN_FUN,           // fun 或 function
    TOKEN_IF,            // if
    TOKEN_NIL,           // nil 或 null
    TOKEN_OR,            // or 或 ||
    TOKEN_PRINT,         // print 或 console.log
    TOKEN_RETURN,        // return
    TOKEN_SUPER,         // super
    TOKEN_THIS,          // this 或 self
    TOKEN_TRUE,          // true
    TOKEN_VAR,           // 数据类型
    TOKEN_WHILE,         // while

    // 特殊 Token
    TOKEN_ERROR,         // 词法错误
    TOKEN_EOF            // 文件结束符
} TokenType;

typedef struct{
    TokenType type;
    const char* start;
    int length;
    int line;//行
}Token;

void initScanner(const char* source);//文件名字
Token scanToken();









#endif




