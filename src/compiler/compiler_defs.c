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
	target->type = VAR;
    target->depth = -1;
    target->id = idName;
    ++compiler->localCount;
	return compiler->localCount-1;
}


static uint32_t addDummyLocal(Compiler* compiler, LocalType type){
    if(compiler->localCount + 1 > compiler->scopeCapacity){
        int oldCapacity = compiler->scopeCapacity;
		compiler->scopeCapacity = GROW_CAPACITY(oldCapacity);
		compiler->locals = GROW_ARRAY(compiler->locals, Local, oldCapacity, compiler->scopeCapacity);
    }

    Local* target = &(compiler->locals[compiler->localCount]);
	target->type = type;
    target->depth = compiler->scopeDepth + 1;
    ++compiler->localCount;	
	return compiler->localCount-1;
}

uint32_t addInstanceLocal(Compiler* compiler){
	return addDummyLocal(compiler, INST);
}

uint32_t addCounterLocal(Compiler* compiler){
	return addDummyLocal(compiler, CNT);
}

void freeCompiler(Compiler* compiler){
	freeMap(compiler->classes);
	FREE_ARRAY(Local, compiler->locals, compiler->scopeCapacity);
	FREE(Compiler, compiler);
}