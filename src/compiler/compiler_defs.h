#ifndef scute_compiler_defs_h
#define scute_compiler_defs_h

#include "common.h"
#include "scanner.h"
#include "hashmap.h"
#include "vm.h"
#include "package.h"

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

typedef struct {
    TK current;
    TK previous;
    TK lastID;

    TKType lastOperator;
    PCType lastOperatorPrecedence;
    uint32_t parenDepth;

    bool hadError;
    bool panicMode;

    char* codeStart;
    char* lastNewline; 

	int lineIndex;
	int currentLineValueIndex;

    Value* lastValueEmitted;
    Value* manipTarget;
    PCType manipPrecedence;
    int manipTargetCharIndex;
    int manipTargetLength;
    int manipTargetParenDepth;
} Parser;


typedef enum {
    VAR,
    CNT,
    INST
} LocalType;

typedef struct{
    LocalType type;
    TK id;
    int depth;
} Local;

extern Parser parser;

 struct sCompiler{
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
    bool returned;
};

typedef struct sCompiler Compiler;


typedef void (*ParseFn)(bool canAssign);

typedef struct{
    ParseFn prefix;
    ParseFn infix;
    PCType precedence;
} ParseRule;

uint32_t addLocal(Compiler* compiler, TK idName);
uint32_t addInstanceLocal(Compiler* compiler);
uint32_t addCounterLocal(Compiler* compiler);

void freeCompiler(Compiler* compiler);
#endif