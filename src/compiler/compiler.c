#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "value.h"
#include "obj.h"
#include "vm.h"
#include "output.h"

typedef struct {
    TK current;
    TK previous;
    int indent;
    bool hadError;
    bool panicMode;
} Parser;

typedef struct {
    HashMap* globals;
} Compiler;

Parser parser;
Compiler currentScope;
Chunk* compilingChunk;

void initCompiler(){

}


static Chunk* currentChunk() {
    return compilingChunk;
}

static void emitByte(uint8_t byte){
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void errorAt(TK* token, char* message){
    if(parser.panicMode) return;
    parser.panicMode = true;
    print(O_ERR, "[line %d] Error", token->line);

    if(token->type == TK_EOF){
        print(O_ERR, " at end");
    } else if (token->type == TK_ERROR){
        print(O_ERR, "[line %d] %.*s", token->line, token->length, token->start);
    } else {
        print(O_ERR, " at '%.*s'", token->length, token->start);
    }
    print(O_ERR, ": %s\n", message);
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

static bool check(TKType type){
    return parser.current.type == type;
}

static bool match(TKType type){
    if(!check(type)) return false;
    advance();
    return true;
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

static int makeConstant(Value val){
    return writeValueArray(&currentChunk()->constants, val);
}

static Value getTokenStringObj(){
    return OBJ_VAL(internString(parser.previous.start, parser.previous.length));
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

static void expression();
static void binary();
static void unary();
static void grouping();
static void number();
static void literal();
static void constant();
static void string();
static void assign();
static void shape();
static void id();

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
{ NULL,     binary,     PC_ASSIGN },  // TK_ASSIGN,
{ NULL,     binary,     PC_ASSIGN },  // TK_INCR_ASSIGN,
{ NULL,     binary,     PC_ASSIGN },  // TK_DECR_ASSIGN,
{ unary,    NULL,       PC_UNARY },   // TK_BANG,
{ NULL,	    NULL,	    PC_UNARY },   // TK_INCR, 
{ NULL,	    NULL,	    PC_UNARY },   // TK_DECR,
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
{ id,       NULL,	    PC_NONE },    // TK_ID,
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
{ shape,	NULL,	    PC_NONE },    // TK_RECT,
{ NULL,	    NULL,	    PC_NONE },    // TK_CIRC,
{ NULL,	    NULL,	    PC_NONE },    // TK_ELLIP,
{ NULL,	    NULL,	    PC_NONE },    // TK_LET,
{ NULL,	    NULL,	    PC_PRIMARY }, // TK_PRINT,
{ NULL,	    NULL,	    PC_NONE },    // TK_DRAW,
{ NULL,	    NULL,	    PC_NONE },    // TK_TEXT,
{ NULL,	    NULL,	    PC_NONE },    // TK_T,
{ NULL,	    NULL,	    PC_NONE },    // TK_ERROR,
{ NULL,     NULL,       PC_NONE }     // TK_EOF
};

static void printStatement();
static void expressionStatement();
static void drawStatement();
static void assignStatement();
static void synchronize();
static void emitParams();

static ParseRule* getRule(TKType type){
    return &rules[type];
}

static void parsePrecedence(PCType precedence){
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if(prefixRule == NULL){
        errorAtCurrent("Expect expression.");
        return;
    }

    prefixRule();

    while(precedence <= getRule(parser.current.type)->precedence){
        advance();
        ParseFn infix = getRule(parser.previous.type)->infix;
        infix();
    }
}



static void statement(){
    advance();
    switch(parser.previous.type){
        case TK_PRINT:
            printStatement();
            break;
        case TK_DRAW:
            drawStatement();
            break;
        case TK_LET:
            assignStatement();
            break;
        default:
            expressionStatement();
            break;
    }
    if(parser.panicMode){
        synchronize();
    }else{
        if(parser.current.type != TK_EOF) consume(TK_NEWLINE, "Expected end of line, '\\n'");
    }
}

static void synchronize(){
    parser.panicMode = false;
    while(parser.current.type != TK_EOF){
        if(parser.previous.type == TK_NEWLINE) return;
        switch(parser.current.type){
            case TK_PRINT:
            case TK_DRAW:
            case TK_LET:
                return;
            default: ;
        }
        advance();
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
    emitConstant(getTokenStringObj());
}

static void shape(){
    emitParams(4, 4);
    emitByte(OP_RECT);
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
        }
    }
    consume(TK_R_PAREN, "Expected ')'.");
}

static void printStatement(){
    expression();
    emitByte(OP_PRINT);
}

static void drawStatement(){
    emitParams(1, 1);
    emitByte(OP_DRAW);
}

static void expressionStatement(){
    expression();
    emitByte(OP_POP);
}

static Value parseIdentifier(char* message){
    consume(TK_ID, message);
    return getTokenStringObj();
}

static void assignStatement(){
    Value idString = parseIdentifier("Expected an identifier.");
    char* contents = AS_CSTRING(idString);
    if(match(TK_ASSIGN)){
        expression();
    }else{
       emitByte(OP_NULL);
    }

    emitConstant(idString);
    emitByte(OP_DEF_GLOBAL);
}

static void id(){
    Value idString = getTokenStringObj();
    emitConstant(idString);
    emitByte(OP_GET_GLOBAL);
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
    initCompiler();
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
    print(O_DEBUG, "%4d ", token.line);
    switch(token.type){
        case TK_INDENT:
            print(O_DEBUG, "%2d '\\t' \n", token.type);
            break;
        case TK_NEWLINE:
            print(O_DEBUG, "%2d '\\n' \n", token.type);
            break;
        default:
            print(O_DEBUG, "%2d '%.*s' \n", token.type, token.length, token.start);
            break;
    }
}