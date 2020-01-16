#ifndef scute_compiler_defs_h
#define scute_compiler_defs_h

#include "common.h"
#include "scanner.h"
#include "hashmap.h"
#include "vm.h"
#include "package.h"

typedef struct {
    TK current;
    TK previous;
    TK lastID;
    bool hadError;
    bool panicMode;
    char* codeStart;
} Parser;

typedef struct{
    TK id;
    int depth;
} Local;

 struct sCompiler{
    HashMap* globals;
    Local* locals;
    int localCount;
    int scopeDepth;
    int scopeCapacity;
    bool enclosed;
    CompilePackage* result;
    TKType instanceType;
    struct sCompiler* super;
    ObjChunk* compilingChunk;
	HashMap* classes;
};

typedef struct sCompiler Compiler;

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

typedef void (*ParseFn)(bool canAssign);

typedef struct{
    ParseFn prefix;
    ParseFn infix;
    PCType precedence;
} ParseRule;

uint32_t addLocal(Compiler* compiler, TK idName);
uint32_t addDummyLocal(Compiler* compiler);
void freeCompiler(Compiler* compiler);
#endif