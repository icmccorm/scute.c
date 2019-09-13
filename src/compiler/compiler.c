#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "value.h"
#include "object.h"

typedef struct {
    TK current;
    TK previous;
    int indent;
    bool hadError;
    bool panicMode;
} Parser;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void emitByte(uint8_t byte){
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void errorAt(TK* token, char* message){
    if(parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if(token->type == TK_EOF){
        fprintf(stderr, " at end");
    } else if (token->type == TK_ERROR){
        //Nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void errorAtCurrent(char* message){
    errorAt(&parser.current, message);
}

static void error(char* message){
    errorAt(&parser.previous, message);
}

static void advance() {
    parser.previous = parser.current;

    for(;;){
        parser.current = scanTK();
        if(parser.current.type != TK_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}

static void consume(TKType type, char* message){
    if(parser.current.type == type){
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void emitReturn(){
    emitByte(OP_RETURN);
}

static void endCompiler(){
    emitReturn();

    #ifdef DEBUG_PRINT_CODE
    #include "debug.h"
    if(!parser.hadError){
        printChunk(currentChunk(), "code");
    }
    #endif
}

static void emitBytes(uint8_t byte1, uint8_t byte2){
    emitByte(byte1);
    emitByte(byte2);
}

static void emitConstant(Value value){
    writeConstant(currentChunk(), value, parser.previous.line);
}

typedef enum{
	PC_NONE,
	PC_ASSIGN,
	PC_OR,
	PC_AND,
	PC_EQUALS, // == !=
	PC_COMPARE, // > < <= >=
	PC_TERM, // + -
	PC_FACTOR, // * / %
	PC_UNARY, // - -- ++ !
	PC_CALL, // . [] ()
	PC_PRIMARY

} PCType;

typedef void (*ParseFn)();

typedef struct{
    ParseFn prefix;
    ParseFn infix;
    PCType precedence;
} ParseRule;


static void binary();
static void unary();
static void grouping();
static void number();
static void literal();
static void function();
static void constant();
static void string();

ParseRule rules[] = {
{ NULL,     binary,     PC_TERM },    // TK_PLUS,
{ unary,    binary,     PC_TERM },    // TK_MINUS,
{ NULL,     binary,     PC_FACTOR },  // TK_TIMES,
{ NULL,     binary,     PC_FACTOR },  // TK_DIVIDE,
{ NULL,     binary,     PC_FACTOR },  // TK_MODULO,
{ NULL,     binary,     PC_EQUALS },  // TK_EQUALS,
{ NULL,	    binary,	    PC_EQUALS },  // TK_BANG_EQUALS,
{ NULL,	    binary,	    PC_COMPARE }, // TK_LESS_EQUALS,
{ NULL,	    binary,	    PC_COMPARE }, // TK_GREATER_EQUALS,
{ NULL,	    binary,	    PC_COMPARE }, // TK_LESS,
{ NULL,	    binary,	    PC_COMPARE }, // TK_GREATER,
{ NULL,     binary,     PC_ASSIGN },    // TK_ASSIGN,
{ NULL,     binary,     PC_ASSIGN },    // TK_INCR_ASSIGN,
{ NULL,     binary,     PC_ASSIGN },    // TK_DECR_ASSIGN,
{ unary,    NULL,       PC_UNARY },   // TK_BANG,
{ NULL,	    NULL,	    PC_UNARY },    // TK_INCR, 
{ NULL,	    NULL,	    PC_UNARY },    // TK_DECR,
{ NULL,	    NULL,	    PC_NONE },    // TK_COLON,
{ NULL,	    NULL,	    PC_NONE },    // TK_QUESTION,
{ NULL,	    NULL,	    PC_NONE },    // TK_EVAL_ASSIGN,
{ NULL,	    NULL,	    PC_NONE },    // TK_L_LIMIT, 
{ NULL,	    NULL,	    PC_NONE },    // TK_R_LIMIT,
{ literal,  NULL,       PC_NONE },    // TK_REAL,
{ literal,  NULL,       PC_NONE },    // TK_INTEGER,
{ literal,	NULL,	    PC_NONE },    // TK_TRUE,
{ literal,	NULL,	    PC_NONE },    // TK_FALSE,
{ literal,	NULL,	    PC_NONE },    // TK_NULL,
{ string,	NULL,	    PC_NONE },    // TK_STRING,
{ NULL,	    NULL,	    PC_ASSIGN },    // TK_ID,
{ NULL,	    NULL,	    PC_NONE },    // TK_FUNC,
{ NULL,	    NULL,	    PC_NONE },    // TK_AND,
{ NULL,	    NULL,	    PC_NONE },    // TK_OR,
{ NULL,	    NULL,	    PC_NONE },    // TK_PRE,
{ literal,	NULL,	    PC_NONE },    // TK_PI,
{ literal,	NULL,	    PC_NONE },    // TK_E,
{ literal,	NULL,	    PC_NONE },    // TK_TAU,
{ NULL,	    NULL,	    PC_NONE },    // TK_SEMI,
{ NULL,	    NULL,	    PC_NONE },    // TK_L_BRACE,
{ NULL,	    NULL,	    PC_NONE },    // TK_R_BRACE,
{ grouping, NULL,       PC_NONE },    // TK_L_PAREN,
{ NULL,	    NULL,	    PC_NONE },    // TK_R_PAREN, 
{ NULL,	    NULL,	    PC_NONE },    // TK_L_BRACK,
{ NULL,	    NULL,	    PC_NONE },    // TK_R_BRACK,
{ NULL,	    NULL,	    PC_NONE },    // TK_COMMA,
{ NULL,	    NULL,	    PC_NONE },    // TK_DEREF, 
{ NULL,	    NULL,	    PC_NONE },    // TK_TILDA, 
{ NULL,	    NULL,	    PC_NONE },    // TK_NEWLINE,
{ NULL,	    NULL,	    PC_NONE },    // TK_INDENT,
{ NULL,	    NULL,	    PC_NONE },    // TK_DO,
{ NULL,	    NULL,	    PC_NONE },    // TK_WHILE,
{ NULL,	    NULL,	    PC_NONE },    // TK_FOR,
{ NULL,	    NULL,	    PC_NONE },    // TK_IF,
{ NULL,	    NULL,	    PC_NONE },    // TK_ELSE,
{ NULL,	    NULL,	    PC_NONE },    // TK_RECT,
{ NULL,	    NULL,	    PC_NONE },    // TK_CIRC,
{ NULL,	    NULL,	    PC_NONE },    // TK_ELLIP,
{ NULL,	    NULL,	    PC_NONE },    // TK_LET,
{ function,	NULL,	    PC_PRIMARY },    // TK_PRINT,
{ NULL,	    NULL,	    PC_NONE },    // TK_DRAW,
{ NULL,	    NULL,	    PC_NONE },    // TK_TEXT,
{ NULL,	    NULL,	    PC_NONE },    // TK_T,
{ NULL,	    NULL,	    PC_NONE },    // TK_ERROR,
{ NULL,     NULL,       PC_NONE }     // TK_EOF
};

static ParseRule* getRule(TKType type){
    return &rules[type];
}

static void parsePrecedence(PCType precedence){
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if(prefixRule == NULL){
        error("Expect expression.");
        return;
    }

    prefixRule();

    while(precedence <= getRule(parser.current.type)->precedence){
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static void statement(){
    advance();
    ParseRule* rule = getRule(parser.previous.type);
    if(rule->precedence = PC_PRIMARY){
        ParseFn primary = rule->prefix;
        primary();
    }else{
        advance();
        while(getRule(parser.previous.type)->precedence < PC_PRIMARY){
            advance();
        }
    }
}

static void expression() {
    parsePrecedence(PC_ASSIGN);
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUM_VAL(value));
}

static void literal() {
    switch(parser.previous.type){
        case TK_FALSE:  emitByte(OP_FALSE); break;
        case TK_TRUE:   emitByte(OP_TRUE); break;
        case TK_NULL:   emitByte(OP_NULL); break;
        case TK_TAU:    emitByte(OP_TAU); break;
        case TK_PI:     emitByte(OP_PI); break;
        case TK_E:      emitByte(OP_E); break;
        case TK_INTEGER:
        case TK_REAL:
            number();
            break;
        default:
            return;
    }    
}

static void string(){
    emitConstant(OBJ_VAL(copyString(parser.previous.start, parser.previous.length)));
}

static void emitParams(int numParams, int minParams){
    consume(TK_L_PAREN, "Expected '('.");
    for(int i = 0; i<numParams; ++i){
        expression();
        if(i + 1 == minParams) {
            if(parser.current.type == TK_COMMA){
                advance();
                emitByte(OP_SEPARATOR);
            }else{
                break;
            }
        }else{
            consume(TK_COMMA, "Expected additional parameters.");
            emitByte(OP_SEPARATOR);
        }
    }
    consume(TK_R_PAREN, "Expected ')'.");
}

static void function(){
    switch(parser.previous.type){
        case TK_PRINT:
            emitParams(1, 1);
            emitByte(OP_PRINT);
            break;
    }
}

static void binary(){
    TKType op = parser.previous.type;

    ParseRule* rule = getRule(op);
    parsePrecedence((PCType) (rule->precedence + 1));

    switch(op){
        case TK_PLUS: 
            emitByte(OP_ADD); 
            break;
        case TK_MINUS: 
            emitByte(OP_SUBTRACT); 
            break;
        case TK_TIMES: 
            emitByte(OP_MULTIPLY); 
            break;
        case TK_DIVIDE: 
            emitByte(OP_DIVIDE); 
            break;
        case TK_MODULO: 
            emitByte(OP_MODULO); 
            break;
        case TK_LESS:
            emitByte(OP_LESS);
            break;
        case TK_GREATER:
            emitByte(OP_GREATER);
            break;
        case TK_LESS_EQUALS:
            emitByte(OP_LESS_EQUALS);
            break;
        case TK_GREATER_EQUALS:
            emitByte(OP_GREATER_EQUALS);
            break;
        case TK_EQUALS:
            emitByte(OP_EQUALS);
            break;
        case TK_BANG_EQUALS:
            emitBytes(OP_EQUALS, OP_NOT);
            break;
        default: 
            break;
    }
}

static void unary(){
    TKType op = parser.previous.type;

    parsePrecedence(PC_UNARY);

    switch(op){
        case TK_MINUS: emitByte(OP_NEGATE); break;
        case TK_BANG: emitByte(OP_NOT); break;
        default: return;
    }
}

static void grouping(){
    expression();
    consume(TK_R_PAREN, "Expect ')' after expression.");
}

bool compile(char* source, Chunk* chunk){
    initScanner(source);
    parser.hadError = false;
    parser.panicMode = false;

    compilingChunk = chunk;

    advance();
    while(parser.current.type != TK_EOF){
        statement();
    }

    consume(TK_EOF, "Expected end of expression.");
    endCompiler();
    return !parser.hadError;
}

static void printToken(TK token){
    printf("%4d ", token.line);
    switch(token.type){
        case TK_INDENT:
            printf("%2d '\\t' \n", token.type);
            break;
        case TK_NEWLINE:
            printf("%2d '\\n' \n", token.type);
            break;
        default:
            printf("%2d '%.*s' \n", token.type, token.length, token.start);
            break;
    }
}