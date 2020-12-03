#ifndef scute_compiler_defs_h
#define scute_compiler_defs_h

#include "common.h"
#include "scanner.h"
#include "hashmap.h"
#include "vm.h"
#include "package.h"
#include "obj_def.h"

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

} Parser;
 
extern Parser parser;

typedef enum {
    VAR,
    CNT,
    INST
} LocalType;

typedef struct{
    LocalType type;
    TK id;
    int depth;
    bool isCaptured; 
} Local;

typedef struct{
    bool isLocal;
    int index;
} Upvalue;

typedef struct Compiler{
    ObjChunk* compilingChunk;

    int scopeDepth;
    bool enclosed;

    TKType instanceType;
    bool returned;

    Local* locals;
    int localCapacity;
    int localCount;

    Upvalue* upvalues;
    int upvalueCapacity;

    ObjClosure* compilingClosure;

    TK* timestepVariable;

    struct Compiler* super;

    CompilePackage* result;

    bool animated;
    int animUpperBound;
    int animLowerBound;
    bool compilingAnimation;
    bool compilingParametric;
} Compiler;

typedef void (*ParseFn)(bool canAssign);

typedef struct{
    ParseFn prefix;
    ParseFn infix;
    PCType precedence;
} ParseRule;

uint32_t addLocal(Compiler* compiler, TK idName);
uint32_t addInstanceLocal(Compiler* compiler);
uint32_t addCounterLocal(Compiler* compiler);
uint32_t addUpvalue(Compiler* compiler, int index, bool isLocal);
void freeCompiler(Compiler* compiler);
#endif