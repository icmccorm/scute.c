#ifndef scute_compiler_defs_h
#define scute_compiler_defs_h
#include "scanner.h"
#include "hashmap.h"

typedef struct {
    TK current;
    TK previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef struct{
    TK id;
    int depth;
} Local;

typedef struct {
    HashMap* globals;
    Local* locals;
    int localCount;
    int scopeDepth;
    int scopeCapacity;
} Compiler;


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

void addLocal(Compiler* compiler, TK idName);
void freeCompiler(Compiler* compiler);

#endif