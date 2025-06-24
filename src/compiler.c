/* ************************************************************************
> File Name:     compiler.c
> Author:        qlc
> 微信:          a2744561792
> Created Time:  四  6/12 15:37:13 2025
> Description:   
 ************************************************************************/
#include "compiler.h"
#include "scanner.h"
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "chunk.h"
#include <string.h>
typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;//防止出现级连效应
} Parser;//解析code

typedef struct {
    Token name;
    int depth;//局部变量的深度
    bool isCaptured;//是否被闭包捕获
} Local;
typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;
typedef enum {
    TYPE_FUNCTION,//普通函数 
    TYPE_SCRIPT //主函数
} FunctionType;
typedef struct Compiler{
    struct Compiler* enclosing;//上一个compiler
    ObjFunction* function;
    FunctionType type;
    Local locals[UINT8_COUNT];//局部变量
    Upvalue upvalues[UINT8_COUNT];
    int localCount;
    int scopeDepth;//局部作用域深度
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;
typedef void(*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;
static void call(bool canAssign);
static void dot(bool canAssign);
static void unary(bool canAssign);
static void binary(bool canAssign);
static void or_(bool canAssign);
static void and_(bool canAssign);
static void call(bool canAssign);
static void number(bool canAssign);
static void string(bool canAssign);
static void variable(bool canAssign);
static void grouping(bool canAssign);
static void literal(bool canAssign);
static void this_(bool canAssign);
static void super_(bool canAssign);
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},
  [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void statement();
static void declaration();
static Chunk* currentChunk() {
    return &current->function->chunk;
}

static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) 
    {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }
    Local* local = &current->locals[current->localCount++];
    local->depth = 0;//编译器隐式地要求栈槽0供虚拟机自己内部使用  槽0固定
    local->isCaptured = false;
    local->name.start = "";
    local->name.length = 0;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
    } 
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}


//当前token代码中的error
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}
//上一个token发生错误
static void error(const char* message) {
    errorAt(&parser.previous, message);
}
//将上一个token发到chunk中
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}
static void printStatement() {
    expression();//前面的运行完后 到这一步一定是一个值在栈上
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}
static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);//放栈上 然后取出 每一个表达式完都要保证栈的清空
}

static void emitReturn() {
    emitByte(OP_NIL);//隐式子返回
    emitByte(OP_RETURN);
}
static ObjFunction* endCompiler() {
    emitReturn();//没有显示返回的语句 加这个让其返回
    ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
        ? function->name->chars : "<script>");
    }
#endif
    current = current->enclosing;
    return function;
}
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}//遇到常量的时候
uint8_t makeConstant(Value value){
    int constant=addConstant(currentChunk(),value);
    if(constant>=UINT8_MAX){
        error("too many constant value up than MAX");
        return 0;
    }
    return constant;
}
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);//16字节跳转指令
    emitByte(0xff);
    return currentChunk()->count - 2;//回到跳转指令位置
}

static void patchJump(int offset) {
    //总共a个 b个是块语句之前的 a-b-2是块语句中的code数量 就是要跳过这么多
    //
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;//低8位
}
static void binary(bool canAssign) {
    //已经到了中缀表达式的下一个了
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:          emitByte(OP_ADD); break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
        case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
        default: return; 
  }
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}


static void call(bool canAssign){
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);

}
static void dot(bool canAssign){}
static void or_(bool canAssign){
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);
    patchJump(endJump);



}
static void and_(bool canAssign){
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);

}
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
static void string(bool canAssign) {
    //copy 需要重新分配内存
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}
//isLocal 是否在上一层
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;
    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }
    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}


static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);//查看这个变量在上一个的第几个变量位置 可以理解成是外层的第几个栈位置
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }
    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }
    return -1;
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);//局部变量中的索引
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }    
    if (canAssign&&match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);//local里的值
    }else {
        emitBytes(getOp, (uint8_t)arg);
    }

}
static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}
static void this_(bool canAssign){}
static void super_(bool canAssign){}


static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // 
    }
}

//以某个表达式级别开始向后扫描express
static void parsePrecedence(Precedence precedence) {
    advance();
    //printf("%d\n",parser.previous.type);
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
    return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}
static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }     
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;//这个只是声明
    local->isCaptured = false;
}
static void declareVariable() {
    if (current->scopeDepth == 0) return;
    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break; 
        }
        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }    
    addLocal(*name);
}
//局部变量只会放到local数组里
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();//声明变量
    if (current->scopeDepth > 0) return 0;
    //在局部作用域中，则退出函数。在运行时，不会通过名称查询局部变量。不需要将变量的名称放入常量表中，所以如果声明在局部作用域内，则返回一个假的表索引。
    return identifierConstant(&parser.previous);
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;
    while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();//这边弄完了后 栈上就是true或者false了
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();//跑完后 code到了快语句的下一个空code里
    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);

}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}


static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OP_POP);
}

static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}


static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);//define是全局性的
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");//名字在常量表中的索引
    printf("val\n");
    if (match(TOKEN_EQUAL)) {
        printf("express\n");
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON,
          "Expect ';' after variable declaration.");
    defineVariable(global);
}
static void forStatement() {
    //变量都应该在循环体内
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
    
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }
    int loopStart = currentChunk()->count;//表达式的下一个
    //条件子句
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);//if false 跳到body那里去 
    }
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }
    statement();
    emitLoop(loopStart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }    
    endScope();
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope(); 
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));//逗号
    }    
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();
    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
    //emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));//函数加到常量表中
    //value hasValue index hasValue index; define funname
    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }    
}


static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");//名字加到常量表中去
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}
static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();//隐式返回
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}
static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    }else if(match(TOKEN_LEFT_BRACE)){
        beginScope();
        block();
        endScope();
    }else if(match(TOKEN_IF)){
        ifStatement();
    }else if(match(TOKEN_WHILE)){
        whileStatement();
    }else if(match(TOKEN_FOR)){
        forStatement();
    }else if(match(TOKEN_FUN)){
        funDeclaration();
    }else if(match(TOKEN_RETURN)){
        returnStatement();
    }else{
        expressionStatement();
    }
}

//处理表达式  先前缀表达式
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);  
}


static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
            switch (parser.current.type) {
                case TOKEN_CLASS:
                case TOKEN_FUN:
                case TOKEN_VAR:
                case TOKEN_FOR:
                case TOKEN_IF:
                case TOKEN_WHILE:
                case TOKEN_PRINT:
                case TOKEN_RETURN:
                return;
                default:
                    ; 
            }   
            advance();
  }
}


static void declaration() {
    
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
}



//括号分组 碰到左括号之后 解析括号里的表达式
static void grouping(bool canAssign) {
    expression();//这个时候current已经是括号里的第一个token了
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
//需要考虑算术优先级  -a+b//先计算-a再计算+b
static void unary(bool canAssign) {
    
    TokenType operatorType = parser.previous.type;
    parsePrecedence(PREC_UNARY);
    // expression();//如果后面有同级的 或者比他优先级高的 会先处理前面的
    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;//负号 算数表达式 先有数字再有操作符
        default: return;
    }
}


ObjFunction* compile(const char* source){
    initScanner(source);
    parser.hadError = false;
    parser.panicMode = false;
    /* int line = -1; */
    /* for (;;) { */
    /*     Token token = scanToken(); */
    /*     if (token.line != line) { */
    /*         printf("%4d ", token.line); */
    /*         line = token.line; */
    /*     } else { */
    /*         printf("   | "); */
    /*     } */
    /*     printf("%2d '%.*s'\n", token.type, token.length, token.start); */ 
    /*     if (token.type == TOKEN_EOF) break; */
    /* } */
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);
    advance();//为什么有括号
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

