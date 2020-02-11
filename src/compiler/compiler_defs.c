#include <stdio.h>
#include "memory.h"
#include "compiler_defs.h"

uint32_t addLocal(Compiler* compiler, TK idName){
    if(compiler->localCount + 1 > compiler->scopeCapacity){
        int oldCapacity = compiler->scopeCapacity;
		compiler->scopeCapacity = GROW_CAPACITY(oldCapacity);
		compiler->locals = GROW_ARRAY(compiler->locals, Local, oldCapacity, compiler->scopeCapacity);
    }
    Local* target = &(compiler->locals[compiler->localCount]);
    target->depth = -1;
    target->id = idName;
    ++compiler->localCount;
	return compiler->localCount-1;
}

uint32_t addDummyLocal(Compiler* compiler){
	TK nullToken;
	nullToken.start = NULL;
	nullToken.line = -1;
	nullToken.length = -1;
	nullToken.type = -1;

    if(compiler->localCount + 1 > compiler->scopeCapacity){
        int oldCapacity = compiler->scopeCapacity;
		compiler->scopeCapacity = GROW_CAPACITY(oldCapacity);
		compiler->locals = GROW_ARRAY(compiler->locals, Local, oldCapacity, compiler->scopeCapacity);
    }
    Local* target = &(compiler->locals[compiler->localCount]);
    target->depth = compiler->scopeDepth;
	target->id = nullToken;
    ++compiler->localCount;	
	return compiler->localCount-1;
}

void freeCompiler(Compiler* compiler){
	freeMap(compiler->classes);
	FREE_ARRAY(Local, compiler->locals, compiler->scopeCapacity);
	FREE(Compiler, compiler);
}